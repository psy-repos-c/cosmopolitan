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
#include "libc/calls/calls.h"
#include "libc/calls/sigbits.h"
#include "libc/calls/struct/sigaction.h"
#include "libc/errno.h"
#include "libc/macros.internal.h"
#include "libc/mem/io.h"
#include "libc/mem/mem.h"
#include "libc/runtime/runtime.h"
#include "libc/sysv/consts/o.h"
#include "libc/sysv/consts/sig.h"
#include "libc/testlib/testlib.h"

STATIC_YOINK("zip_uri_support");
STATIC_YOINK("plinko.com");
STATIC_YOINK("library.lisp");
STATIC_YOINK("library_test.lisp");
STATIC_YOINK("binarytrees.lisp");
STATIC_YOINK("algebra.lisp");
STATIC_YOINK("algebra_test.lisp");
STATIC_YOINK("infix.lisp");
STATIC_YOINK("ok.lisp");

static const char *const kSauces[] = {
    "/zip/library.lisp",       //
    "/zip/library_test.lisp",  //
    "/zip/binarytrees.lisp",   //
    "/zip/algebra.lisp",       //
    "/zip/algebra_test.lisp",  //
    "/zip/infix.lisp",         //
    "/zip/ok.lisp",            //
};

char testlib_enable_tmp_setup_teardown_once;

void SetUpOnce(void) {
  int fdin, fdout;
  ASSERT_NE(-1, mkdir("bin", 0755));
  ASSERT_NE(-1, (fdin = open("/zip/plinko.com", O_RDONLY)));
  ASSERT_NE(-1, (fdout = creat("bin/plinko.com", 0755)));
  ASSERT_NE(-1, _copyfd(fdin, fdout, -1));
  EXPECT_EQ(0, close(fdout));
  EXPECT_EQ(0, close(fdin));
}

TEST(plinko, worksOrPrintsNiceError) {
  ssize_t rc;
  char buf[16], drain[64];
  sigset_t chldmask, savemask;
  int i, pid, fdin, wstatus, pfds[2][2];
  struct sigaction ignore, saveint, savequit, savepipe;
  bzero(buf, sizeof(buf));
  ignore.sa_flags = 0;
  ignore.sa_handler = SIG_IGN;
  EXPECT_EQ(0, sigemptyset(&ignore.sa_mask));
  EXPECT_EQ(0, sigaction(SIGINT, &ignore, &saveint));
  EXPECT_EQ(0, sigaction(SIGQUIT, &ignore, &savequit));
  EXPECT_EQ(0, sigaction(SIGPIPE, &ignore, &savepipe));
  EXPECT_EQ(0, sigemptyset(&chldmask));
  EXPECT_EQ(0, sigaddset(&chldmask, SIGCHLD));
  EXPECT_EQ(0, sigprocmask(SIG_BLOCK, &chldmask, &savemask));
  ASSERT_NE(-1, pipe2(pfds[0], O_CLOEXEC));
  ASSERT_NE(-1, pipe2(pfds[1], O_CLOEXEC));
  ASSERT_NE(-1, (pid = vfork()));
  if (!pid) {
    close(0), dup(pfds[0][0]);
    close(1), dup(pfds[1][1]);
    close(2), dup(pfds[1][1]);
    sigaction(SIGINT, &saveint, 0);
    sigaction(SIGQUIT, &savequit, 0);
    sigaction(SIGQUIT, &savepipe, 0);
    sigprocmask(SIG_SETMASK, &savemask, 0);
    execve("bin/plinko.com", (char *const[]){"bin/plinko.com", 0},
           (char *const[]){0});
    _exit(127);
  }
  EXPECT_NE(-1, close(pfds[0][0]));
  EXPECT_NE(-1, close(pfds[1][1]));
  for (i = 0; i < ARRAYLEN(kSauces); ++i) {
    EXPECT_NE(-1, (fdin = open(kSauces[i], O_RDONLY)));
    rc = _copyfd(fdin, pfds[0][1], -1);
    if (rc == -1) EXPECT_EQ(EPIPE, errno);
    EXPECT_NE(-1, close(fdin));
  }
  EXPECT_NE(-1, close(pfds[0][1]));
  EXPECT_NE(-1, read(pfds[1][0], buf, sizeof(buf) - 1));
  while (read(pfds[1][0], drain, sizeof(drain)) > 0) donothing;
  EXPECT_NE(-1, close(pfds[1][0]));
  EXPECT_NE(-1, waitpid(pid, &wstatus, 0));
  EXPECT_TRUE(WIFEXITED(wstatus));
  if (!startswith(buf, "error: ")) {
    EXPECT_STREQ("OKCOMPUTER\n", buf);
    EXPECT_EQ(0, WEXITSTATUS(wstatus));
  } else {
    EXPECT_EQ(1, WEXITSTATUS(wstatus));
  }
  EXPECT_EQ(0, sigaction(SIGINT, &saveint, 0));
  EXPECT_EQ(0, sigaction(SIGQUIT, &savequit, 0));
  EXPECT_EQ(0, sigaction(SIGPIPE, &savepipe, 0));
  EXPECT_EQ(0, sigprocmask(SIG_SETMASK, &savemask, 0));
}
