/* Copyright (C) 2008-2019 Free Software Foundation, Inc.
   Contributed by Jakub Jelinek <jakub@redhat.com>.

   This file is part of the GNU Offloading and Multi Processing Library
   (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* This is a Linux specific implementation of a mutex synchronization
   mechanism for libgomp.  This type is private to the library.  This
   implementation uses atomic instructions and the futex syscall.  */

#ifndef GOMP_WAIT_H
#define GOMP_WAIT_H 1

#include "libgomp.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)


#define FUTEX_WAIT	0
#define FUTEX_WAKE	1
#define FUTEX_PRIVATE_FLAG	128

#ifdef HAVE_ATTRIBUTE_VISIBILITY
# pragma GCC visibility push(hidden)
#endif

extern int active_flag[5000000];
extern int fd1[5000000];
extern uint64_t id1[5000000], id2[5000000];
extern int gomp_futex_wait, gomp_futex_wake;
extern uint64_t instruction_num;
extern uint64_t cycle_num;
extern int profile_flag;
extern int read_backend_flag;

#include <futex.h>

static inline int do_spin (int *addr, int val)
{
  unsigned long long i, count = gomp_spin_count_var;

  if (__builtin_expect (__atomic_load_n (&gomp_managed_threads,
                                         MEMMODEL_RELAXED)
                        > gomp_available_cpus, 0))
    count = gomp_throttled_spin_count_var;
  for (i = 0; i < count; i++)
    if (__builtin_expect (__atomic_load_n (addr, MEMMODEL_RELAXED) != val, 0))
      return 0;
    else
      cpu_relax ();
  return 1;
}

static inline void do_wait (int *addr, int val)
{
  int spin_result;
	uint64_t val1_t = 0;
  pid_t tid = gettid();

	// perf event
	struct read_format
	{
		uint64_t nr;
		struct
		{
			uint64_t value;
			uint64_t id;
		} values[];
	};

  #define BUF_LEN 40960
	char buf1[BUF_LEN];
	struct read_format *rf1 = (struct read_format *)buf1;

	int i = 0;

  int OMP_WAIT = 0;

  if (active_flag[tid] == 1) {
    OMP_WAIT = 1;
  }

  if (OMP_WAIT == 1) {
    ioctl(fd1[tid], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    memset(buf1, 0, sizeof(char) * BUF_LEN);
    read(fd1[tid], buf1, sizeof(buf1));
    for (i = 0; i < rf1->nr; i++)
    {
      if (rf1->values[i].id == id1[tid]) {
        val1_t = rf1->values[i].value;
        break;
      }
    }

    //printf("%d: do_wait end: %ld, %ld\n", tid, val1_t, val2_t);
    if (profile_flag == 1 || read_backend_flag == 1)
		{
		  __sync_fetch_and_add(&instruction_num, val1_t);
		}
  }

  spin_result = do_spin (addr, val);
  if (spin_result)
    futex_wait (addr, val);

  if (OMP_WAIT == 1) {
    ioctl(fd1[tid], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(fd1[tid], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
  }

}

#ifdef HAVE_ATTRIBUTE_VISIBILITY
# pragma GCC visibility pop
#endif

#endif /* GOMP_WAIT_H */
