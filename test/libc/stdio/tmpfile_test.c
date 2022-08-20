/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2021 Justine Alexandra Roberts Tunney                              │
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
#include "libc/calls/struct/dirent.h"
#include "libc/calls/struct/stat.h"
#include "libc/calls/syscall_support-sysv.internal.h"
#include "libc/dce.h"
#include "libc/runtime/gc.internal.h"
#include "libc/runtime/runtime.h"
#include "libc/stdio/stdio.h"
#include "libc/stdio/temp.h"
#include "libc/str/str.h"
#include "libc/sysv/consts/at.h"
#include "libc/testlib/testlib.h"
#include "libc/x/x.h"

char testlib_enable_tmp_setup_teardown;
char oldtmpdir[PATH_MAX];

bool IsDirectoryEmpty(const char *path) {
  DIR *d;
  bool res = true;
  struct dirent *e;
  ASSERT_NE(NULL, (d = opendir(path)));
  while ((e = readdir(d))) {
    if (!strcmp(e->d_name, ".")) continue;
    if (!strcmp(e->d_name, "..")) continue;
    res = false;
  }
  closedir(d);
  return res;
}

void SetUp(void) {
  strcpy(oldtmpdir, kTmpPath);
  strcpy(kTmpPath, ".");
}

void TearDown(void) {
  strcpy(kTmpPath, oldtmpdir);
}

TEST(tmpfile, test) {
  FILE *f;
  struct stat st;
  EXPECT_TRUE(IsDirectoryEmpty(kTmpPath));
  f = tmpfile();
  if (IsWindows()) {
    EXPECT_FALSE(IsDirectoryEmpty(kTmpPath));
  } else {
    EXPECT_TRUE(IsDirectoryEmpty(kTmpPath));
  }
  EXPECT_SYS(0, 0, fstat(fileno(f), &st));
  EXPECT_NE(010600, st.st_mode);
  EXPECT_NE(-1, fputc('t', f));
  EXPECT_NE(-1, fflush(f));
  rewind(f);
  EXPECT_EQ('t', fgetc(f));
  EXPECT_EQ(0, fclose(f));
  EXPECT_TRUE(IsDirectoryEmpty(kTmpPath));
}

TEST(tmpfile, renameToRealFile) {
  if (!IsLinux() || !__is_linux_2_6_23()) return;
  FILE *f;
  f = tmpfile();
  ASSERT_EQ(2, fputs("hi", f));
  ASSERT_SYS(0, 0,
             linkat(AT_FDCWD, gc(xasprintf("/proc/self/fd/%d", fileno(f))),
                    AT_FDCWD, "real", AT_SYMLINK_FOLLOW));
  ASSERT_EQ(0, fclose(f));
  ASSERT_NE(NULL, (f = fopen("real", "r")));
  ASSERT_EQ('h', fgetc(f));
  ASSERT_EQ('i', fgetc(f));
  ASSERT_EQ(0, fclose(f));
}
