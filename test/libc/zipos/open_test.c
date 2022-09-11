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
#include "libc/mem/mem.h"
#include "libc/runtime/gc.h"
#include "libc/runtime/runtime.h"
#include "libc/str/str.h"
#include "libc/sysv/consts/o.h"
#include "libc/testlib/hyperion.h"
#include "libc/testlib/testlib.h"
#include "libc/thread/spawn.h"
#include "libc/zipos/zipos.h"

STATIC_YOINK("zip_uri_support");
STATIC_YOINK("libc/testlib/hyperion.txt");
STATIC_YOINK("inflate");
STATIC_YOINK("inflateInit2");
STATIC_YOINK("inflateEnd");

int Worker(void *arg, int tid) {
  int i, fd;
  char *data;
  for (i = 0; i < 20; ++i) {
    ASSERT_NE(-1, (fd = open("/zip/libc/testlib/hyperion.txt", O_RDONLY)));
    data = malloc(kHyperionSize);
    ASSERT_EQ(kHyperionSize, read(fd, data, kHyperionSize));
    ASSERT_EQ(0, memcmp(data, kHyperion, kHyperionSize));
    ASSERT_SYS(0, 0, close(fd));
    free(data);
  }
  return 0;
}

TEST(zipos, test) {
  int i, n = 16;
  struct spawn *t = _gc(malloc(sizeof(struct spawn) * n));
  for (i = 0; i < n; ++i) ASSERT_SYS(0, 0, _spawn(Worker, 0, t + i));
  for (i = 0; i < n; ++i) EXPECT_SYS(0, 0, _join(t + i));
  /* __print_maps(); */
}
