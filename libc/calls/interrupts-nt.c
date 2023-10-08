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
#include "libc/calls/internal.h"
#include "libc/calls/sig.internal.h"
#include "libc/intrin/atomic.h"
#include "libc/intrin/weaken.h"
#include "libc/sysv/errfuns.h"
#include "libc/thread/posixthread.internal.h"
#ifdef __x86_64__

textwindows int _check_cancel(void) {
  if (_weaken(_pthread_cancel_ack) &&  //
      _pthread_self() && !(_pthread_self()->pt_flags & PT_NOCANCEL) &&
      atomic_load_explicit(&_pthread_self()->pt_canceled,
                           memory_order_acquire)) {
    return _weaken(_pthread_cancel_ack)();
  }
  return 0;
}

textwindows int _check_signal(bool restartable) {
  int status;
  if (!_weaken(__sig_check)) return 0;
  if (!(status = _weaken(__sig_check)())) return 0;
  if (status == 2 && restartable) return 0;
  return eintr();
}

#endif /* __x86_64__ */
