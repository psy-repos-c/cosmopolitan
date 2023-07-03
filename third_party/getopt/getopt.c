/*	$NetBSD: getopt.c,v 1.26 2003/08/07 16:43:40 agc Exp $	*/
/*
 * Copyright (c) 1987, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)getopt.c	8.3 (Berkeley) 4/27/95
 * $FreeBSD: src/lib/libc/stdlib/getopt.c,v 1.8 2007/01/09 00:28:10 imp Exp $
 * $DragonFly: src/lib/libc/stdlib/getopt.c,v 1.7 2005/11/20 12:37:48 swildner
 */
#include "libc/calls/calls.h"
#include "libc/runtime/runtime.h"
#include "libc/str/str.h"
#include "third_party/getopt/getopt.internal.h"

asm(".ident\t\"\\n\
getopt (BSD-3)\\n\
Copyright 1987, 1993, 1994 The Regents of the University of California\"");
asm(".include \"libc/disclaimer.inc\"");

#define BADCH  (int)'?'
#define BADARG (int)':'

/**
 * If error message should be printed.
 * @see getopt()
 */
int opterr;

/**
 * Index into parent argv vector.
 * @see getopt()
 */
int optind;

/**
 * Character checked for validity.
 * @see getopt()
 */
int optopt;

/**
 * Reset getopt.
 * @see getopt()
 */
int optreset;

/**
 * Argument associated with option.
 * @see getopt()
 */
char *optarg;

char *getopt_place;
static char kGetoptEmsg[1];

static void getopt_print_badch(const char *s) {
  char b1[512];
  char b2[8] = " -- ";
  b1[0] = 0;
  if (program_invocation_name) {
    strlcat(b1, program_invocation_name, sizeof(b1));
    strlcat(b1, ": ", sizeof(b1));
  }
  strlcat(b1, s, sizeof(b1));
  b2[4] = optopt;
  b2[5] = '\n';
  b2[6] = 0;
  strlcat(b1, b2, sizeof(b1));
  write(2, b1, strlen(b1));
}

/**
 * Parses argc/argv argument vector, e.g.
 *
 *     while ((opt = getopt(argc, argv, "hvx:")) != -1) {
 *       switch (opt) {
 *         case 'x':
 *           x = atoi(optarg);
 *           break;
 *         case 'v':
 *           ++verbose;
 *           break;
 *         case 'h':
 *           PrintUsage(EXIT_SUCCESS, stdout);
 *         default:
 *           PrintUsage(EX_USAGE, stderr);
 *       }
 *     }
 *
 * @see optind
 * @see optarg
 */
int getopt(int nargc, char *const nargv[], const char *ostr) {
  char *oli; /* option letter list index */
  static bool once;
  if (!once) {
    opterr = 1;
    optind = 1;
    getopt_place = kGetoptEmsg;
    once = true;
  }
  /*
   * Some programs like cvs expect optind = 0 to trigger
   * a reset of getopt.
   */
  if (optind == 0) optind = 1;
  if (optreset || *getopt_place == 0) { /* update scanning pointer */
    optreset = 0;
    getopt_place = nargv[optind];
    if (optind >= nargc || *getopt_place++ != '-') {
      /* Argument is absent or is not an option */
      getopt_place = kGetoptEmsg;
      return -1;
    }
    optopt = *getopt_place++;
    if (optopt == '-' && *getopt_place == 0) {
      /* "--" => end of options */
      ++optind;
      getopt_place = kGetoptEmsg;
      return -1;
    }
    if (optopt == 0) {
      /* Solitary '-', treat as a '-' option
         if the program (eg su) is looking for it. */
      getopt_place = kGetoptEmsg;
      if (strchr(ostr, '-') == NULL) return -1;
      optopt = '-';
    }
  } else {
    optopt = *getopt_place++;
  }
  /* See if option letter is one the caller wanted... */
  if (optopt == ':' || (oli = strchr(ostr, optopt)) == NULL) {
    if (*getopt_place == 0) ++optind;
    if (opterr && *ostr != ':') getopt_print_badch("illegal option");
    return BADCH;
  }
  /* Does this option need an argument? */
  if (oli[1] != ':') {
    /* don't need argument */
    optarg = NULL;
    if (*getopt_place == 0) ++optind;
  } else {
    /* Option-argument is either the rest of this argument or the
       entire next argument. */
    if (*getopt_place) {
      optarg = getopt_place;
    } else if (nargc > ++optind) {
      optarg = nargv[optind];
    } else {
      /* option-argument absent */
      getopt_place = kGetoptEmsg;
      if (*ostr == ':') return BADARG;
      if (opterr) getopt_print_badch("option requires an argument");
      return BADCH;
    }
    getopt_place = kGetoptEmsg;
    ++optind;
  }
  return optopt; /* return option letter */
}
