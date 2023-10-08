/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2022 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include "libc/assert.h"
#include "libc/calls/cp.internal.h"
#include "libc/calls/struct/timespec.internal.h"
#include "libc/dce.h"
#include "libc/errno.h"
#include "libc/intrin/describeflags.internal.h"
#include "libc/intrin/strace.internal.h"
#include "libc/intrin/weaken.h"
#include "libc/nexgen32e/yield.h"
#include "libc/runtime/clktck.h"
#include "libc/str/str.h"
#include "libc/sysv/consts/clock.h"
#include "libc/sysv/consts/timer.h"
#include "libc/sysv/errfuns.h"
#include "libc/thread/thread.h"

static errno_t sys_clock_nanosleep(int clock, int flags,
                                   const struct timespec *req,
                                   struct timespec *rem) {
  int e, rc;
  BEGIN_CANCELATION_POINT;
  e = errno;
  if (IsLinux() || IsFreebsd() || IsNetbsd()) {
    rc = __sys_clock_nanosleep(clock, flags, req, rem);
  } else if (IsXnu()) {
    rc = sys_clock_nanosleep_xnu(clock, flags, req, rem);
  } else if (IsOpenbsd()) {
    rc = sys_clock_nanosleep_openbsd(clock, flags, req, rem);
  } else if (IsWindows()) {
    rc = sys_clock_nanosleep_nt(clock, flags, req, rem);
  } else {
    rc = enosys();
  }
  if (rc == -1) {
    rc = errno;
    errno = e;
  }
  END_CANCELATION_POINT;
#if 0
  STRACE("sys_clock_nanosleep(%s, %s, %s, [%s]) → %d% m",
         DescribeClockName(clock), DescribeSleepFlags(flags),
         DescribeTimespec(0, req), DescribeTimespec(rc, rem), rc);
#endif
  return rc;
}

// determine how many nanoseconds it takes before clock_nanosleep()
// starts sleeping with 90 percent accuracy; in other words when we
// ask it to sleep 1 second, it (a) must NEVER sleep for less time,
// and (b) does not sleep for longer than 1.1 seconds of time. what
// ever is below that, thanks but no thanks, we'll just spin yield,
static struct timespec GetNanosleepThreshold(void) {
  return timespec_fromnanos(1000000000 / CLK_TCK);
}

static errno_t CheckCancel(void) {
  if (_weaken(pthread_testcancel_np)) {
    return _weaken(pthread_testcancel_np)();
  } else {
    return 0;
  }
}

static errno_t SpinNanosleep(int clock, int flags, const struct timespec *req,
                             struct timespec *rem) {
  errno_t rc;
  struct timespec now, start, elapsed;
  if ((rc = CheckCancel())) {
    if (rc == EINTR && !flags && rem) {
      *rem = *req;
    }
    return rc;
  }
  unassert(!clock_gettime(CLOCK_REALTIME, &start));
  for (;;) {
    spin_yield();
    unassert(!clock_gettime(CLOCK_REALTIME, &now));
    if (flags & TIMER_ABSTIME) {
      if (timespec_cmp(now, *req) >= 0) {
        return 0;
      }
      if ((rc = CheckCancel())) {
        return rc;
      }
    } else {
      if (timespec_cmp(now, start) < 0) continue;
      elapsed = timespec_sub(now, start);
      if ((rc = CheckCancel())) {
        if (rc == EINTR && rem) {
          if (timespec_cmp(elapsed, *req) >= 0) {
            bzero(rem, sizeof(*rem));
          } else {
            *rem = elapsed;
          }
        }
        return rc;
      }
      if (timespec_cmp(elapsed, *req) >= 0) {
        return 0;
      }
    }
  }
}

// clock_gettime() takes a few nanoseconds but sys_clock_nanosleep()
// is incapable of sleeping for less than a millisecond on platforms
// such as windows and it's not much prettior on unix systems either
static bool ShouldUseSpinNanosleep(int clock, int flags,
                                   const struct timespec *req) {
  errno_t e;
  struct timespec now;
  if (clock != CLOCK_REALTIME &&          //
      clock != CLOCK_REALTIME_PRECISE &&  //
      clock != CLOCK_MONOTONIC &&         //
      clock != CLOCK_MONOTONIC_RAW &&     //
      clock != CLOCK_MONOTONIC_PRECISE) {
    return false;
  }
  if (!flags) {
    return timespec_cmp(*req, GetNanosleepThreshold()) < 0;
  }
  e = errno;
  if (clock_gettime(clock, &now)) {
    // punt to the nanosleep system call
    errno = e;
    return false;
  }
  return timespec_cmp(*req, now) < 0 ||
         timespec_cmp(timespec_sub(*req, now), GetNanosleepThreshold()) < 0;
}

/**
 * Sleeps for particular amount of time.
 *
 * Here's how you could sleep for one second:
 *
 *     clock_nanosleep(0, 0, &(struct timespec){1}, 0);
 *
 * Your sleep will be interrupted automatically if you do something like
 * press ctrl-c during the wait. That's an `EINTR` error and it lets you
 * immediately react to status changes. This is always the case, even if
 * you're using `SA_RESTART` since this is a `@norestart` system call.
 *
 *     void OnCtrlC(int sig) {} // EINTR only happens after delivery
 *     signal(SIGINT, OnCtrlC); // do delivery rather than kill proc
 *     printf("save me from sleeping forever by pressing ctrl-c\n");
 *     clock_nanosleep(0, 0, &(struct timespec){INT_MAX}, 0);
 *     printf("you're my hero\n");
 *
 * If you want to perform an uninterruptible sleep without having to use
 * sigprocmask() to block all signals then this function provides a good
 * solution to that problem. For example:
 *
 *     struct timespec rel, now, abs;
 *     clock_gettime(CLOCK_REALTIME, &now);
 *     rel = timespec_frommillis(100);
 *     abs = timespec_add(now, rel);
 *     while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &abs, 0));
 *
 * will accurately spin on `EINTR` errors. That way you're not impeding
 * signal delivery and you're not loosing precision on the wait timeout.
 * This function has first-class support on Linux, FreeBSD, and NetBSD;
 * on OpenBSD it's good; on XNU it's bad; and on Windows it's ugly.
 *
 * @param clock should be `CLOCK_REALTIME` and you may consult the docs
 *     of your preferred platforms to see what other clocks might work
 * @param flags can be 0 for relative and `TIMER_ABSTIME` for absolute
 * @param req can be a relative or absolute time, depending on `flags`
 * @param rem shall be updated with the remainder of unslept time when
 *     (1) it's non-null; (2) `flags` is 0; and (3) -1 w/ `EINTR` is
 *     returned; if this function returns 0 then `rem` is undefined;
 *     if flags is `TIMER_ABSTIME` then `rem` is ignored
 * @return 0 on success, or errno on error
 * @raise EINTR when a signal got delivered while we were waiting
 * @raise ECANCELED if thread was cancelled in masked mode
 * @raise ENOTSUP if `clock` is known but we can't use it here
 * @raise EFAULT if `req` or null or bad memory was passed
 * @raise EINVAL if `clock` is unknown to current platform
 * @raise EINVAL if `flags` has an unrecognized value
 * @raise EINVAL if `req->tv_nsec ∉ [0,1000000000)`
 * @raise ENOSYS on bare metal
 * @cancelationpoint
 * @returnserrno
 * @norestart
 */
errno_t clock_nanosleep(int clock, int flags, const struct timespec *req,
                        struct timespec *rem) {
  int rc;
  // threads on win32 stacks call this so we can't asan check *ts
  LOCKTRACE("clock_nanosleep(%s, %s, %s) → ...", DescribeClockName(clock),
            DescribeSleepFlags(flags), DescribeTimespec(0, req));
  if (IsMetal()) {
    rc = ENOSYS;
  } else if (clock == 127 ||              //
             (flags & ~TIMER_ABSTIME) ||  //
             req->tv_sec < 0 ||           //
             !(0 <= req->tv_nsec && req->tv_nsec <= 999999999)) {
    rc = EINVAL;
  } else if (ShouldUseSpinNanosleep(clock, flags, req)) {
    rc = SpinNanosleep(clock, flags, req, rem);
  } else {
    rc = sys_clock_nanosleep(clock, flags, req, rem);
  }
  TIMETRACE("clock_nanosleep(%s, %s, %s, [%s]) → %s", DescribeClockName(clock),
            DescribeSleepFlags(flags), DescribeTimespec(0, req),
            DescribeTimespec(rc, rem), DescribeErrno(rc));
  return rc;
}
