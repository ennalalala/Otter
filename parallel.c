/* Copyright (C) 2005-2019 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

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

/* This file handles the (bare) PARALLEL construct.  */

#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)

#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <err.h>

#include "libgomp.h"
#include <limits.h>
#include <sys/time.h>

#ifdef CPUFREQ
#include <cpufreq-bindings/cpufreq-bindings.h>
#endif

#include <sys/ioctl.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <stdint.h>
#include <inttypes.h>

#define OMP_NUM_STATIC 100
int active_flag[5000000];
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

typedef struct gold_struct
{
  int a;
  int b;
  int x;
  int y;
  double f_a;
  double f_b;
  double f_x;
  double f_y;
} gold;
gold gold_info[1];

typedef struct fibo_struct
{
  int a;
  int b;
  int x;
  int y;
  double f_a;
  double f_b;
  double f_x;
  double f_y;
} fibo;
fibo fibo_info[OMP_NUM_STATIC][1];

struct perf_event_attr pea;
int fd1[5000000], fd2, fd3, fd4, fd5;
struct perf_event_attr pea1[500];
uint64_t id1[5000000], id2[5000000], id3, id4, id5[5000000];
uint64_t val1, val2, val3, val4, val5;
uint64_t instruction_num = 0;
uint64_t cycle_num = 0;
// uint64_t instruction_num_list[97];
// uint64_t tid_map[10000000];
#define BUF_LEN 4096
char buf[BUF_LEN];
struct read_format *rf = (struct read_format *)buf;

//Newton
#define LOOP_NUM 100
#define MAX 100
typedef struct stPoint
{
  double x;
  double y;
  double z;
} Point;
Point points[LOOP_NUM][MAX];
double form[LOOP_NUM][MAX];
Point points1[MAX];
double form1[MAX];
double totaltime[MAX] = {0};
double minposition = 0;
double mintime = INT64_MAX;
double downratio[MAX] = {0};

#define NPROC 96
#define NCTR 1
#define PERF_FORMAT_SCALE (PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING)

struct pfm_result
{
  long long ctr[NCTR];
};
int OMP_NUM = 70;
int last_iter_seq = 20;
struct timeval start[LOOP_NUM][OMP_NUM_STATIC], end[LOOP_NUM][OMP_NUM_STATIC];
int x = 0, y = -1;
int cost[LOOP_NUM][OMP_NUM_STATIC];

struct perf_event_attr hw[NPROC][NCTR];
long long before[NPROC][NCTR], after[NPROC][NCTR];
unsigned int fd[NPROC][NCTR];

int profile_flag = 0;
int profile_flag_individual[OMP_NUM_STATIC] = {0};
int threadnum_flag = 0;
int threadnum_flag_individual[OMP_NUM_STATIC] = {0};
int perf_flag = 0;
int reset_freq_flag = 0;
int position_flag = 0;
int threadnum_set = 0;
int threadnum_set_individual[OMP_NUM_STATIC] = {0};
int threadnum_set_times = 0;
int binary_strategy = 0;
int binary_gap = 1;
int binary_optimal = 0;
// change freq after thread num
#define FREQ_NUM 4
#define POSITION_NUM 4
int read_freq_num = 0;
int compute_freq_num = 0;
int read_backend_num = 0;
int read_backend_omp = 0;
int read_freq_flag = 0;
int read_backend_flag = 0;
double freq_real[FREQ_NUM];
uint64_t backend[POSITION_NUM], total[POSITION_NUM];
double position_read[POSITION_NUM];
int freq_set;
//#define OMP_PLACE_FIRST
#define OMP_Otter
#ifdef OMP_Otter
int position_set = 3;
#else
int position_set = 4;
#endif

#define INIT_PROFILE_TIMES 3
int PROFILE_TIMES = INIT_PROFILE_TIMES;
int iter_a, iter_b;
#ifdef OMP_Otter
//int iter_array[100] = {94, 48, 24};
int iter_array[100] = {94, 48, 30};
#elif defined OMP_Hoder
int iter_array[100] = {95, 56, 36};
#else
int iter_array[100] = {95, 3, 6};
#endif
int next_thread_num = 95;
#ifdef OMP_Aurora
//int next_thread_num_individual[OMP_NUM_STATIC] = {0};
//int next_thread_num_individual[OMP_NUM_STATIC] = {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3};
int next_thread_num_individual[OMP_NUM_STATIC] = {96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,
 96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96};
#elif defined OMP_Hoder
int next_thread_num_individual[OMP_NUM_STATIC] = {0};
#endif
int init_finish = 0;
int init_finish_flag = 0;
//int iter_array1[PROFILE_TIMES] = {95, 88, 80, 72, 60, 48, 24, 16, 8, 4};
int delta = 10;
int feature = 0;
int gold_return = 0;
double gold_value = 0;
int fibo_return[OMP_NUM_STATIC] = {0};
double fibo_value[OMP_NUM_STATIC] = {0};
int binary_return[OMP_NUM_STATIC] = {0};
double binary_value[OMP_NUM_STATIC] = {0};
double wtime_start;
double wtime_end;

unsigned long long work_iter_num;

uint64_t perf_value[1000];

// cpufreq
#define NCORES 4
uint32_t CORE_IDS[] = {23, 47, 71, 95};
uint32_t available_freqs[100];
int setspeed_fds[NCORES];
int setminfreq_fds[NCORES];

// price
double price = 0.0526;
double totalprice[NPROC] = {0};

// index
double lastindex;
double curindex;

// jump first loop
int jump_first_flag = 0;

// core time
double core_time_start = 0;
double core_time_end = 0;
double core_time_total = 0;
int core_time_end_num = 10;
int core_time_output_flag = 0;


/* Determine the number of threads to be launched for a PARALLEL construct.
   This algorithm is explicitly described in OpenMP 3.0 section 2.4.1.
   SPECIFIED is a combination of the NUM_THREADS clause and the IF clause.
   If the IF clause is false, SPECIFIED is forced to 1.  When NUM_THREADS
   is not present, SPECIFIED is 0.  */

unsigned
gomp_resolve_num_threads(unsigned specified, unsigned count)
{
  struct gomp_thread *thr = gomp_thread();
  struct gomp_task_icv *icv;
  unsigned threads_requested, max_num_threads, num_threads;
  unsigned long busy;
  struct gomp_thread_pool *pool;

  icv = gomp_icv(false);

  if (specified == 1)
    return 1;
  else if (thr->ts.active_level >= 1 && !icv->nest_var)
    return 1;
  else if (thr->ts.active_level >= gomp_max_active_levels_var)
    return 1;

  //printf("icv : %ld\n", icv->nthreads_var);
  /* If NUM_THREADS not specified, use nthreads_var.  */
  if (specified == 0)
    threads_requested = icv->nthreads_var;
  else
    threads_requested = specified;

  max_num_threads = threads_requested;

  /* If dynamic threads are enabled, bound the number of threads
     that we launch.  */
  if (icv->dyn_var)
  {
    unsigned dyn = gomp_dynamic_max_threads();
    if (dyn < max_num_threads)
      max_num_threads = dyn;

    /* Optimization for parallel sections.  */
    if (count && count < max_num_threads)
      max_num_threads = count;
  }

  /* UINT_MAX stands for infinity.  */
  if (__builtin_expect(icv->thread_limit_var == UINT_MAX, 1) || max_num_threads == 1)
    return max_num_threads;

  printf("resolve continue\n");

  /* The threads_busy counter lives in thread_pool, if there
     isn't a thread_pool yet, there must be just one thread
     in the contention group.  If thr->team is NULL, this isn't
     nested parallel, so there is just one thread in the
     contention group as well, no need to handle it atomically.  */
  pool = thr->thread_pool;
  if (thr->ts.team == NULL || pool == NULL)
  {
    num_threads = max_num_threads;
    if (num_threads > icv->thread_limit_var)
      num_threads = icv->thread_limit_var;
    if (pool)
      pool->threads_busy = num_threads;
    return num_threads;
  }

#ifdef HAVE_SYNC_BUILTINS
  do
  {
    busy = pool->threads_busy;
    num_threads = max_num_threads;
    if (icv->thread_limit_var - busy + 1 < num_threads)
      num_threads = icv->thread_limit_var - busy + 1;
  } while (__sync_val_compare_and_swap(&pool->threads_busy,
                                       busy, busy + num_threads - 1) != busy);
#else
  gomp_mutex_lock(&gomp_managed_threads_lock);
  num_threads = max_num_threads;
  busy = pool->threads_busy;
  if (icv->thread_limit_var - busy + 1 < num_threads)
    num_threads = icv->thread_limit_var - busy + 1;
  pool->threads_busy += num_threads - 1;
  gomp_mutex_unlock(&gomp_managed_threads_lock);
#endif

  return num_threads;
}

void GOMP_parallel_start(void (*fn)(void *), void *data, unsigned num_threads)
{
  num_threads = gomp_resolve_num_threads(num_threads, 0);
  printf("GOMP_parallel_start num_threads: %d\n", num_threads);
  gomp_team_start(fn, data, num_threads, 0, gomp_new_team(num_threads),
                  NULL);
}

void GOMP_parallel_end(void)
{
  struct gomp_task_icv *icv = gomp_icv(false);
  if (__builtin_expect(icv->thread_limit_var != UINT_MAX, 0))
  {
    struct gomp_thread *thr = gomp_thread();
    struct gomp_team *team = thr->ts.team;
    unsigned int nthreads = team ? team->nthreads : 1;
    printf("GOMP_parallel_end: nthreads = %d\n", nthreads);
    gomp_team_end();
    printf("GOMP_parallel_end: gomp_team_end\n");
    if (nthreads > 1)
    {
      /* If not nested, there is just one thread in the
	     contention group left, no need for atomicity.  */
      if (thr->ts.team == NULL)
        thr->thread_pool->threads_busy = 1;
      else
      {
#ifdef HAVE_SYNC_BUILTINS
        __sync_fetch_and_add(&thr->thread_pool->threads_busy,
                             1UL - nthreads);
#else
        gomp_mutex_lock(&gomp_managed_threads_lock);
        thr->thread_pool->threads_busy -= nthreads - 1;
        gomp_mutex_unlock(&gomp_managed_threads_lock);
#endif
      }
    }
  }
  else
    gomp_team_end();
}
ialias(GOMP_parallel_end)

    // newton interpolation
    void newton_cal(int n, int loop)
{
  int i = 0, j = 0;
  for (i = 0; i <= n; i++)
  {
    form[loop][i] = points[loop][i].y;
  }
  for (i = 0; i < n; i++)
  {
    for (j = n; j > i; j--)
    {

      form[loop][j] = (form[loop][j] - form[loop][j - 1]) / (points[loop][j].x - points[loop][j - 1 - i].x);
    }
  }
}

double newton_parse(int x, int n, int loop)
{
  int i = 0;
  double tmp = 1.0;
  double newton = form[loop][0];
  for (i = 0; i < n; i++)
  {

    tmp = tmp * (x - points[loop][i].x);       
    newton = newton + tmp * form[loop][i + 1]; 
  }
  return newton;
}
void newton_cal1(int n)
{
  int i = 0, j = 0;
  for (i = 0; i <= n; i++)
  {
    form1[i] = points1[i].y;
  }
  for (i = 0; i < n; i++)
  {
    for (j = n; j > i; j--)
    {

      form1[j] = (form1[j] - form1[j - 1]) / (points1[j].x - points1[j - 1 - i].x);
    }
  }
}

double newton_parse1(int x, int n)
{
  int i = 0;
  double tmp = 1.0;
  double newton = form1[0];
  for (i = 0; i < n; i++)
  {

    tmp = tmp * (x - points1[i].x);      
    newton = newton + tmp * form1[i + 1]; 
  }
  return newton;
}

//#define OMP_PERF
int test_num = 6;
int test_count = 0;
#define OMP_CORETIME
int64_t addr_map[10000];
int64_t addr_len = 0;

int addr_exist[10000] = {0};
int addr_pos = 0;
int find_cycle_flag = 0;
int64_t round_start = 0;
int otter_round = 20;

void find_cycle() {
  int i = addr_pos - 2;
  int j = addr_pos - 1;
  int pos = 0;
  while (i >= 0) {
    if (addr_exist[i] == addr_exist[addr_pos - 1]) {
      break;
    }
    i--;
  }
  pos = i;
  while (i >= 0 && j > pos) {
    if (addr_exist[i] == addr_exist[j]) {
      i--;
      j--;
    } else {
      return;
    } 
  }
  if (j == pos) {
    find_cycle_flag = 1;
  }

}

void GOMP_parallel_origin(void (*fn)(void *), void *data, unsigned num_threads,
                     unsigned int flags)
{
  num_threads = gomp_resolve_num_threads(num_threads, 0);

  gomp_team_start(fn, data, num_threads, flags, gomp_new_team(num_threads),
                  NULL);
  fn(data);
  ialias_call(GOMP_parallel_end)();
}

int gold_search()
{
  if (gold_info[0].f_x > gold_info[0].f_y)
  {
    if (gold_info[0].b - gold_info[0].x <= delta)
    {
      return gold_info[0].y;
    }
    else
    {
      gold_info[0].a = gold_info[0].x;
      gold_info[0].x = gold_info[0].y;
      gold_info[0].f_x = gold_info[0].f_y;
      gold_info[0].y = gold_info[0].a + 0.618 * (gold_info[0].b - gold_info[0].a);
      return -1;
    }
  }
  else
  {
    if (gold_info[0].y - gold_info[0].a <= delta)
    {
      return gold_info[0].x;
    }
    else
    {
      gold_info[0].b = gold_info[0].y;
      gold_info[0].y = gold_info[0].x;
      gold_info[0].f_y = gold_info[0].f_x;
      gold_info[0].x = gold_info[0].a + 0.382 * (gold_info[0].b - gold_info[0].a);
      return -2;
    }
  }
}

double fibo_array[7] = {1, 1, 2, 3, 5, 8, 13};
int fibo_num = 7;

int fibo_search(int fibo_index, int omp_index) {
  if (fibo_info[omp_index][0].f_x > fibo_info[omp_index][0].f_y)
  {
    if (fibo_info[omp_index][0].b - fibo_info[omp_index][0].x <= delta || fibo_index < 1)
    {
      fibo_info[omp_index][0].y = (fibo_info[omp_index][0].y % 2) ? fibo_info[omp_index][0].y + 1 : fibo_info[omp_index][0].y;
      return fibo_info[omp_index][0].y;
    }
    else
    {
      fibo_info[omp_index][0].a = fibo_info[omp_index][0].x;
      fibo_info[omp_index][0].x = fibo_info[omp_index][0].y;
      fibo_info[omp_index][0].f_x = fibo_info[omp_index][0].f_y;
      fibo_info[omp_index][0].y = fibo_info[omp_index][0].a + fibo_array[fibo_index - 1] / fibo_array[fibo_index] * (fibo_info[omp_index][0].b - fibo_info[omp_index][0].a);
      fibo_info[omp_index][0].y = (fibo_info[omp_index][0].y % 2) ? fibo_info[omp_index][0].y + 1 : fibo_info[omp_index][0].y;
      return -1;
    }
  }
  else
  {
    if (fibo_info[omp_index][0].y - fibo_info[omp_index][0].a <= delta || fibo_index < 2)
    {
      fibo_info[omp_index][0].x = (fibo_info[omp_index][0].x % 2) ? fibo_info[omp_index][0].x + 1 : fibo_info[omp_index][0].x;
      return fibo_info[omp_index][0].x;
    }
    else
    {
      fibo_info[omp_index][0].b = fibo_info[omp_index][0].y;
      fibo_info[omp_index][0].y = fibo_info[omp_index][0].x;
      fibo_info[omp_index][0].f_y = fibo_info[omp_index][0].f_x;
      fibo_info[omp_index][0].x = fibo_info[omp_index][0].a + fibo_array[fibo_index - 2] / fibo_array[fibo_index] * (fibo_info[omp_index][0].b - fibo_info[omp_index][0].a);
      fibo_info[omp_index][0].x = (fibo_info[omp_index][0].x % 2) ? fibo_info[omp_index][0].x + 1 : fibo_info[omp_index][0].x;
      return -2;
    }
  }
}

enum {
  Exponential,
  Search,
  End
};

int step[OMP_NUM_STATIC] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
int lastNT[OMP_NUM_STATIC] = {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3};
int bestNT[OMP_NUM_STATIC] = {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3};
int currentNT[OMP_NUM_STATIC] = {6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6};
int state[OMP_NUM_STATIC] = {0};
int tempNT[OMP_NUM_STATIC] = {0};
double bestMetricMsmt[OMP_NUM_STATIC], metricMsmt[OMP_NUM_STATIC];

int binary_search(int index) {
  switch (state[index])
  {
  case Exponential:
    step[index] = lastNT[index] / 2;
    if (metricMsmt[index] <= bestMetricMsmt[index]) {
      bestMetricMsmt[index] = metricMsmt[index];
      bestNT[index] = currentNT[index];
      if (currentNT[index] * 2 <= NPROC) {
        lastNT[index] = currentNT[index];
        currentNT[index] = bestNT[index] * 2;
      } else {
        currentNT[index] -= step[index];
        state[index] = Search;
      }
    } else {
      // if (bestNT[index] >= NPROC / 2) {
      //   currentNT[index] -= step[index];
      // } else {
      //   currentNT[index] += step[index];
      // }
      currentNT[index] -= step[index];
      state[index] = Search;
    }
    return -1;
  
  case Search:
    step[index] = step[index] / 2;
    if (bestNT[index] > currentNT[index]) {
      tempNT[index] = currentNT[index] + step[index];
    } else {
      tempNT[index] = currentNT[index] - step[index];
    }

    if (metricMsmt[index] <= bestMetricMsmt[index]) {
      bestNT[index] = currentNT[index];
      bestMetricMsmt[index] = metricMsmt[index];
    }
    if (step[index] == 1 || step[index] == 0) {
      state[index] = End;
      return 0;
    }
    currentNT[index] = tempNT[index];
    return -2;

  default:
    break;
  }
  return -3;
}

double difference(double a, double b)
{
  if (a > b) {
    return a - b;
  } else {
    return b - a;
  }
}

int difference_int(int a, int b)
{
  if (a > b) {
    return a - b;
  } else {
    return b - a;
  }
}

double golden_sum;

double ratio, gold_temp;
int enter_flag = 0;
int turn_flag = 0;    //从放置到数量的切换
int repeat_flag = 0; //每次重复一轮

int fix_thread = 0;
int fix_pos[96] = {0};



#define OMP_MODIFY
#ifdef OMP_MODIFY

void GOMP_parallel(void (*fn)(void *), void *data, unsigned num_threads,
                   unsigned int flags)
{
  int e_flag = 0;
  int i = 0;

  if (!find_cycle_flag) {
    for (i = 0; i < addr_len; i++) {
      if (addr_map[i] == (int64_t)(*fn))
      {
        find_cycle();
        addr_exist[addr_pos++] = addr_map[i];
        if (find_cycle_flag == 1) {
          printf("find cycle, num = %d\n", num_threads);
          round_start = addr_map[i];
        }
        e_flag = 1;
        break;
      }
    }
    if (e_flag == 0) {
      addr_map[addr_len] = (int64_t)(*fn);
      addr_len++;
    }
  }

  if (find_cycle_flag) {
    num_threads = 95;
    if (round_start == (int64_t)(*fn)) {
      otter_round++;
    }
  }


  int j = 0;
  pid_t tid = gettid();

  num_threads = gomp_resolve_num_threads(num_threads, 0);


  if (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag == 0 && jump_first_flag == 0)
  {
    // jump iteration
    if (otter_round == last_iter_seq + 1)
    {
      jump_first_flag = 1;
#ifdef OMP_Hoder
      profile_flag = 1;
#elif defined OMP_Otter
#ifndef OMP_PLACE_FIRST
      profile_flag = 1;
#else
      read_backend_flag = 1;
      position_flag = 1;
#endif
#else
      for (i = 0; i < OMP_NUM_STATIC; i++) {
        profile_flag_individual[i] = 1;
      }
#endif
    }
    else
    {
      num_threads = NPROC;
    }
  }

#ifdef OMP_Otter
  if (position_flag == 1)
  {
    enter_flag = 0;
    if (otter_round == last_iter_seq + 2)
    {
      OMP_NUM = y + 1;
      last_iter_seq++;
      enter_flag = 1;
    }

    if (y == OMP_NUM - 1 && enter_flag == 1) {
      if (read_backend_omp == OMP_NUM)
        {
          position_read[read_backend_num] = (double)backend[read_backend_num];
          printf("backend: cycle: %ld, ins: %ld, %f\n", backend[read_backend_num], total[read_backend_num], position_read[read_backend_num]);
          read_backend_num++;
          read_backend_omp = 0;
        }
    }

    if (read_backend_num == POSITION_NUM) {
      read_backend_flag = 0;
      position_flag = 2;

      if (POSITION_NUM > 1 && position_read[1] > position_read[3])
      {
        position_set = 4;
      }
      printf("position = %d\n", position_set);

      read_backend_num = -1;
#ifdef OMP_PLACE_FIRST
      profile_flag = 1;
      turn_flag = 1;
      x = -1;
#endif
    }
  }
#endif


#ifndef OMP_Aurora
  if (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag == 1)
#else
  if (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag_individual[y] == 1)
#endif
  {
    if (turn_flag != 1) {
      enter_flag = 0;
    }

    if (otter_round == last_iter_seq + 2)
    {
      OMP_NUM = y + 1;
      last_iter_seq++;
      enter_flag = 1;
    }

    if (init_finish == 0 && enter_flag == 1 && turn_flag != 1)
    {

      for (j = 0; j < OMP_NUM; j++)
      {
        points[OMP_NUM][x].y += points[j][x].y;
        points[OMP_NUM][x].z += points[j][x].z;

      }

      if (points[OMP_NUM][x].z == 0)
      {
        points[OMP_NUM][x].z = 1;
      }

#ifdef OMP_Aurora
      init_finish_flag = 0;
      printf("enter binary\n");
      for( j = 0; j < OMP_NUM; j++) {
        if (state[j] != End) {
          init_finish_flag = 1;
          binary_value[j] = points[j][x].y;
          if (x < INIT_PROFILE_TIMES - 1)
          {
            next_thread_num = iter_array[x + 1];
          }
          if (x == 1) {
            bestMetricMsmt[j] = binary_value[j];
            metricMsmt[j] = binary_value[j];
          } else if (x > 1) {
            metricMsmt[j] = binary_value[j];
            binary_return[j] = binary_search(j);
            if (currentNT[j] == 96) {
              currentNT[j] = 95;
            }
            if (binary_return[j] < 0) {
              PROFILE_TIMES++;
              next_thread_num_individual[j] = currentNT[j];
            } else {
              next_thread_num_individual[j] = bestNT[j];
              threadnum_set_individual[j] = next_thread_num_individual[j];
              threadnum_flag_individual[j] = 1;
              profile_flag_individual[j] = 0;
              feature = 1;
            }
          }
          printf("seq %d: next thread: %d\n", j, (next_thread_num_individual[j] == 0) ? next_thread_num : next_thread_num_individual[j]);
        }
      }
      if (init_finish_flag == 0) {
        init_finish = 1;
      }

#endif

#ifdef OMP_Hoder
      for (j = 0; j < OMP_NUM; j++) {
        fibo_value[j] = points[j][x].y;
        if (x == 1)
        {
          fibo_info[j][0].f_y = fibo_value[j];
        }
        else if (x == 2)
        {
          fibo_info[j][0].f_x = fibo_value[j];
        }
        if (x < INIT_PROFILE_TIMES - 1)
        {
          next_thread_num = iter_array[x + 1];
        }
        else
        {
          if (fibo_return[j] == -1)
          {
            fibo_info[j][0].f_y = fibo_value[j];
          }
          else if (fibo_return[j] == -2)
          {
            fibo_info[j][0].f_x = fibo_value[j];
          }
          fibo_return[j] = fibo_search(fibo_num - x, j);
          if (fibo_return[j] == -1)
          {
            PROFILE_TIMES++;
            next_thread_num_individual[j] = fibo_info[j][0].y;
          }
          else if (fibo_return[j] == -2)
          {
            PROFILE_TIMES++;
            next_thread_num_individual[j] = fibo_info[j][0].x;
          }
          else
          {
            next_thread_num_individual[j] = fibo_return[j];
            init_finish = 1;
            threadnum_set_individual[j] = next_thread_num_individual[j];
            threadnum_flag = 1;
            profile_flag = 0;
            feature = 1;
          }
        }
        printf("seq %d: next thread: %d return = %d\n", j, (next_thread_num_individual[j] == 0) ? next_thread_num : next_thread_num_individual[j], fibo_return[j]);
      }

#endif

#ifdef OMP_Otter
      gold_value = points[OMP_NUM][x].y;
      points1[x].x = points[0][x].x;
      points1[x].y = gold_value;
      printf("gold_value = %f, %f, %f, x = %d\n", points[OMP_NUM][x].y, points[OMP_NUM][x].z, gold_value, x);
      if (next_thread_num == 95) {
        mintime = gold_value;
      }
      if (x == 1)
      {
        gold_info[0].f_y = gold_value;
      }
      else if (x == 2)
      {
        gold_info[0].f_x = gold_value;
        if (next_thread_num == 71) {
          gold_info[0].x = 48;
          gold_info[0].y = 71;
          gold_temp = gold_info[0].f_y;
          gold_info[0].f_y = gold_info[0].f_x;
          gold_info[0].f_x = gold_temp;
        }
      }
      if (x < INIT_PROFILE_TIMES - 1)
      {
        next_thread_num = iter_array[x + 1];
      }

      if (points[OMP_NUM][x].y > points[OMP_NUM][x-1].y && x == 1)
      {
        ratio = difference(points[OMP_NUM][x].y, points[OMP_NUM][0].y) / points[OMP_NUM][0].y;
        next_thread_num = (iter_array[x] + iter_array[x - 1]) / 2;
        iter_array[x + 1] = next_thread_num;

      } else if(points[OMP_NUM][x].y < points[OMP_NUM][x-1].y && points[OMP_NUM][x].y > points[OMP_NUM][x-2].y && x == 2) {
        init_finish = 1;
      }

      else if (x >= INIT_PROFILE_TIMES - 1)
      {

        double gold_start = omp_get_wtime();
        if (gold_return == -1)
        {
          gold_info[0].f_y = gold_value;
        }
        else if (gold_return == -2)
        {
          gold_info[0].f_x = gold_value;
        }
        gold_return = gold_search();
        if (gold_return == -1)
        {
          PROFILE_TIMES++;
          next_thread_num = gold_info[0].y;
        }
        else if (gold_return == -2)
        {
          PROFILE_TIMES++;
          next_thread_num = gold_info[0].x;
        }
        else
        {
          next_thread_num = gold_return;
          init_finish = 1;
          threadnum_set = next_thread_num;
          threadnum_flag = 1;
          profile_flag = 0;
          feature = 1;
        }
        double gold_end = omp_get_wtime();
        golden_sum = (gold_end - gold_start) * 1000;
        printf("golden: %f\n", golden_sum);
        iter_array[x + 1] = next_thread_num;
      }
#endif

#ifdef OMP_Otter
      printf("next thread: %d\n", next_thread_num);
#endif

  #ifdef OMP_Otter
      if (x + 1 == PROFILE_TIMES && y == OMP_NUM - 1 && feature == 0)
      {

          double newton_start = omp_get_wtime();
          newton_cal1(x);

          printf("final: %f, mintime: %f\n", points[OMP_NUM][x].y, mintime);

            for (i = NPROC - 2; i > 1; i--)
            {

              downratio[i] = (newton_parse1(i, x) - mintime) / mintime;

              if (downratio[i] > 0.1)
              {
                threadnum_set = i+1;
                threadnum_flag = 1;
                profile_flag = 0;
                printf("change threadnum to %d, down_ratio = %f\n", threadnum_set, downratio[i+1]);
                break;
              }
            }
          double newton_end = omp_get_wtime();
          printf("newton: %f\n", newton_end - newton_start);


      }
  #endif

    }


  }



  if (num_threads < NPROC && jump_first_flag == 1) {
    y++;
    if (y == OMP_NUM && enter_flag == 1)
    {
      y = 0;
      x++;
      turn_flag = 0;
    }
  }

#ifndef OMP_Aurora
  if (threadnum_flag == 1)
#else
  if (y >= 0 && threadnum_flag_individual[y] == 1)
#endif
  {
#ifdef OMP_Otter
    num_threads = threadnum_set;
#else
    if (next_thread_num_individual[y] != 0) {
      num_threads = next_thread_num_individual[y];
    }
#endif

#ifdef OMP_Otter
#ifndef OMP_PLACE_FIRST
    if (position_flag == 0)
    {
      read_backend_flag = 1;
      position_flag = 1;
    }
#endif
#endif

    flags = position_set;

  }


  if (perf_flag == 0)
  {
#ifdef OMP_Otter
    memset(&pea, 0, sizeof(struct perf_event_attr));
    pea.type = 8;
    pea.size = sizeof(struct perf_event_attr);
    pea.config = 0x0008; // instruction
    pea.disabled = 1;
    pea.exclude_kernel = 1;
    pea.exclude_hv = 1;
    pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
    fd1[tid] = syscall(__NR_perf_event_open, &pea, tid, -1, -1, 0);
    ioctl(fd1[tid], PERF_EVENT_IOC_ID, &id1[tid]);



    memset(&pea, 0, sizeof(struct perf_event_attr));
    pea.type = 8;
    pea.size = sizeof(struct perf_event_attr);
    pea.config = 0x0011; // cycle
    //pea.config = PERF_COUNT_HW_CACHE_MISSES;
    pea.disabled = 1;
    pea.exclude_kernel = 1;
    pea.exclude_hv = 1;
    pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
    fd5 = syscall(__NR_perf_event_open, &pea, tid, -1, -1 /*!!!*/, 0);
    ioctl(fd5, PERF_EVENT_IOC_ID, &id5[tid]);


#endif

#ifdef OMP_Otter
    gold_info[0].a = 1;
    gold_info[0].b = 95;
    gold_info[0].x = 30;
    gold_info[0].y = 48;
#endif

#ifdef OMP_Hoder
    for (i = 0; i < OMP_NUM_STATIC; i++) {
      fibo_info[i][0].a = 2;
      fibo_info[i][0].b = 94;
      fibo_info[i][0].x = 36;
      fibo_info[i][0].y = 56;
    }
#endif

    perf_flag = 1;
  }

#ifndef OMP_Aurora
  if (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag == 1)
#else
  if (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag_individual[y] == 1)
#endif
  {

#ifdef OMP_Otter
    num_threads = next_thread_num;
#else
    if (x < INIT_PROFILE_TIMES) {
      num_threads = next_thread_num;
    } else {
      if (next_thread_num_individual[y] != 0) {
        num_threads = next_thread_num_individual[y];
      }
    }
#endif
#ifdef OMP_Otter
    flags = position_set;
#endif


  }

#ifdef OMP_Otter
  if (read_backend_flag == 1 && read_backend_num < POSITION_NUM)
  {
#ifdef OMP_PLACE_FIRST
    num_threads = NPROC / 2;
#endif
    if (read_backend_num < POSITION_NUM / 2)
    {
      flags = 3;
    }
    else
    {
      flags = 4;
    }
  }
#endif

#ifndef CORETIME_OUTPUT
  if (jump_first_flag == 1 && otter_round != core_time_end_num) {
    core_time_start = omp_get_wtime();
  }
#endif

  gomp_team_start(fn, data, num_threads, flags, gomp_new_team(num_threads),
                  NULL);

#ifndef OMP_Aurora
  if (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag == 1)
#else
  if (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag_individual[y] == 1)
#endif
  {
#ifdef OMP_Otter

    memset(buf, 0, sizeof(char) * BUF_LEN);

    ioctl(fd1[tid], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(fd1[tid], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);

    ioctl(fd5, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(fd5, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);

#else
    wtime_start = omp_get_wtime();
#endif
  }


  if (read_backend_flag == 1 && read_backend_num < POSITION_NUM)
  {
    memset(buf, 0, sizeof(char) * BUF_LEN);

    ioctl(fd1[tid], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(fd1[tid], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    ioctl(fd5, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(fd5, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
  }

#ifdef OMP_Otter
  if (profile_flag == 1 || read_backend_flag == 1) {
    active_flag[tid] = 1;
  }
#endif
  fn(data);
#ifdef OMP_Otter
  if (profile_flag == 1 || read_backend_flag == 1) {
    active_flag[tid] = 0;
  }
#endif


#ifdef OMP_Otter
  if ((read_backend_flag == 1 && read_backend_num < POSITION_NUM) || (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag == 1))
  {
    ioctl(fd1[tid], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    memset(buf, 0, sizeof(char) * BUF_LEN);
    read(fd1[tid], buf, sizeof(buf));
    for (i = 0; i < rf->nr; i++)
    {
      if (rf->values[i].id == id1[tid]) {
        val1 = rf->values[i].value;
        break;
      }

    }

		__sync_fetch_and_add(&instruction_num, val1);


    ialias_call(GOMP_parallel_end)();
    ioctl(fd5, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
  }
  else
  {
    ialias_call(GOMP_parallel_end)();
  }
#else
  ialias_call(GOMP_parallel_end)();
  wtime_end = omp_get_wtime();
#endif

#ifndef CORETIME_OUTPUT
  if (jump_first_flag == 1 && otter_round != core_time_end_num) {
    core_time_end = omp_get_wtime();
    core_time_total += (core_time_end - core_time_start) * (double)num_threads;
  }

  if (otter_round == core_time_end_num && core_time_output_flag == 0 && jump_first_flag == 1) {
    core_time_output_flag = 1;
    printf("- - - core_time: %f ---\n", core_time_total);
  }
#endif

  if (read_backend_flag == 1 && read_backend_num < POSITION_NUM)
  {
    memset(buf, 0, sizeof(char) * BUF_LEN);
    read(fd5, buf, sizeof(buf));
    for (i = 0; i < rf->nr; i++)
    {
      if (rf->values[i].id == id5[tid])
      {
        val5 = rf->values[i].value;
        break;
      }
    }
    cycle_num = val5;
    backend[read_backend_num] += cycle_num;
    total[read_backend_num] += instruction_num;
    instruction_num = 0;
    cycle_num = 0;
    read_backend_omp++;

  }


#ifndef OMP_Aurora
  if (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag == 1)
#else
  if (num_threads < NPROC && num_threads != 1 && num_threads != 0 && profile_flag_individual[y] == 1)
#endif
  {

#ifdef OMP_Otter
    memset(buf, 0, sizeof(char) * BUF_LEN);
    read(fd5, buf, sizeof(buf));
    for (i = 0; i < rf->nr; i++)
    {
      if (rf->values[i].id == id5[tid])
      {
        val5 = rf->values[i].value;
        break;
      }
    }
    cycle_num = val5;
#endif

#ifdef OMP_Otter
    points[y][x].x = (double)num_threads;
    points[y][x].y = (double)cycle_num;

    points[y][x].z = (double)instruction_num;
    instruction_num = 0;
    cycle_num = 0;
#else
    points[y][x].y = wtime_end - wtime_start;
#endif
  }

}
#endif
unsigned
GOMP_parallel_reductions(void (*fn)(void *), void *data,
                         unsigned num_threads, unsigned int flags)
{
  struct gomp_taskgroup *taskgroup;
  num_threads = gomp_resolve_num_threads(num_threads, 0);
  uintptr_t *rdata = *(uintptr_t **)data;
  taskgroup = gomp_parallel_reduction_register(rdata, num_threads);
  gomp_team_start(fn, data, num_threads, flags, gomp_new_team(num_threads),
                  taskgroup);
  fn(data);
  ialias_call(GOMP_parallel_end)();
  gomp_sem_destroy(&taskgroup->taskgroup_sem);
  free(taskgroup);
  return num_threads;
}

bool GOMP_cancellation_point(int which)
{
  if (!gomp_cancel_var)
    return false;

  struct gomp_thread *thr = gomp_thread();
  struct gomp_team *team = thr->ts.team;
  if (which & (GOMP_CANCEL_LOOP | GOMP_CANCEL_SECTIONS))
  {
    if (team == NULL)
      return false;
    return team->work_share_cancelled != 0;
  }
  else if (which & GOMP_CANCEL_TASKGROUP)
  {
    if (thr->task->taskgroup)
    {
      if (thr->task->taskgroup->cancelled)
        return true;
      if (thr->task->taskgroup->workshare && thr->task->taskgroup->prev && thr->task->taskgroup->prev->cancelled)
        return true;
    }
    /* FALLTHRU into the GOMP_CANCEL_PARALLEL case,
	 as #pragma omp cancel parallel also cancels all explicit
	 tasks.  */
  }
  if (team)
    return gomp_team_barrier_cancelled(&team->barrier);
  return false;
}
ialias(GOMP_cancellation_point)

    bool GOMP_cancel(int which, bool do_cancel)
{
  if (!gomp_cancel_var)
    return false;

  if (!do_cancel)
    return ialias_call(GOMP_cancellation_point)(which);

  struct gomp_thread *thr = gomp_thread();
  struct gomp_team *team = thr->ts.team;
  if (which & (GOMP_CANCEL_LOOP | GOMP_CANCEL_SECTIONS))
  {
    /* In orphaned worksharing region, all we want to cancel
	 is current thread.  */
    if (team != NULL)
      team->work_share_cancelled = 1;
    return true;
  }
  else if (which & GOMP_CANCEL_TASKGROUP)
  {
    if (thr->task->taskgroup)
    {
      struct gomp_taskgroup *taskgroup = thr->task->taskgroup;
      if (taskgroup->workshare && taskgroup->prev)
        taskgroup = taskgroup->prev;
      if (!taskgroup->cancelled)
      {
        gomp_mutex_lock(&team->task_lock);
        taskgroup->cancelled = true;
        gomp_mutex_unlock(&team->task_lock);
      }
    }
    return true;
  }
  team->team_cancelled = 1;
  gomp_team_barrier_cancel(team);
  return true;
}

/* The public OpenMP API for thread and team related inquiries.  */

int omp_get_num_threads(void)
{
  struct gomp_team *team = gomp_thread()->ts.team;
  return team ? team->nthreads : 1;
}

int omp_get_thread_num(void)
{
  return gomp_thread()->ts.team_id;
}

/* This wasn't right for OpenMP 2.5.  Active region used to be non-zero
   when the IF clause doesn't evaluate to false, starting with OpenMP 3.0
   it is non-zero with more than one thread in the team.  */

int omp_in_parallel(void)
{
  return gomp_thread()->ts.active_level > 0;
}

int omp_get_level(void)
{
  return gomp_thread()->ts.level;
}

int omp_get_ancestor_thread_num(int level)
{
  struct gomp_team_state *ts = &gomp_thread()->ts;
  if (level < 0 || level > ts->level)
    return -1;
  for (level = ts->level - level; level > 0; --level)
    ts = &ts->team->prev_ts;
  return ts->team_id;
}

int omp_get_team_size(int level)
{
  struct gomp_team_state *ts = &gomp_thread()->ts;
  if (level < 0 || level > ts->level)
    return -1;
  for (level = ts->level - level; level > 0; --level)
    ts = &ts->team->prev_ts;
  if (ts->team == NULL)
    return 1;
  else
    return ts->team->nthreads;
}

int omp_get_active_level(void)
{
  return gomp_thread()->ts.active_level;
}

ialias(omp_get_num_threads)
    ialias(omp_get_thread_num)
        ialias(omp_in_parallel)
            ialias(omp_get_level)
                ialias(omp_get_ancestor_thread_num)
                    ialias(omp_get_team_size)
                        ialias(omp_get_active_level)
