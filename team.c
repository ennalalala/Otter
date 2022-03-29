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

/* This file handles the maintainence of threads in response to team
   creation and termination.  */

#define _GNU_SOURCE
#include "libgomp.h"
#include "pool.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)

#define OMP_MIGRATION
#define OMP_PERF
extern uint64_t instruction_num;
extern int profile_flag;
extern int read_backend_flag;
extern int active_flag[5000000];
extern int fd1[5000000];
extern struct perf_event_attr pea1[500];
extern uint64_t id1[5000000], id2[5000000];
extern int threadnum_flag;

extern int fix_thread;
extern int fix_pos[96];

double w_start, w_end;

// update
#ifdef LIBGOMP_USE_PTHREADS
pthread_attr_t gomp_thread_attr;

/* This key is for the thread destructor.  */
pthread_key_t gomp_thread_destructor;

/* This is the libgomp per-thread data structure.  */
#if defined HAVE_TLS || defined USE_EMUTLS
__thread struct gomp_thread gomp_tls_data;
#else
pthread_key_t gomp_tls_key;
#endif

/* This structure is used to communicate across pthread_create.  */

struct gomp_thread_start_data
{
	void (*fn)(void *);
	void *fn_data;
	struct gomp_team_state ts;
	struct gomp_task *task;
	struct gomp_thread_pool *thread_pool;
	unsigned int place;
	bool nested;
	pthread_t handle;
};

/* This function is a pthread_create entry point.  This contains the idle
   loop in which a thread waits to be called up to become part of a team.  */

static void *
gomp_thread_start(void *xdata)
{
	struct gomp_thread_start_data *data = xdata;
	struct gomp_thread *thr;
	struct gomp_thread_pool *pool;
	void (*local_fn)(void *);
	void *local_data;

#ifdef OMP_MIGRATION
	cpu_set_t mask;
#endif

#ifdef OMP_MIGRATION
	pid_t tid = gettid();
#elif defined OMP_PERF
	pid_t tid = gettid();
#endif

#ifdef OMP_PERF
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

	struct perf_event_attr pea1;
#ifndef OMP_PURE
	uint64_t val1_t = 0;
#endif

#define BUF_LEN 40960
	char buf1[BUF_LEN];
	struct read_format *rf1 = (struct read_format *)buf1;
	int i = 0;
#endif

#if defined HAVE_TLS || defined USE_EMUTLS
	thr = &gomp_tls_data;
#else
	struct gomp_thread local_thr;
	thr = &local_thr;
	pthread_setspecific(gomp_tls_key, thr);
#endif
	gomp_sem_init(&thr->release, 0);

	/* Extract what we need from data.  */
	local_fn = data->fn;
	local_data = data->fn_data;
	thr->thread_pool = data->thread_pool;
	thr->ts = data->ts;
	thr->task = data->task;
	thr->place = data->place;
#ifdef GOMP_NEEDS_THREAD_HANDLE
	thr->handle = data->handle;
#endif

	thr->ts.team->ordered_release[thr->ts.team_id] = &thr->release;

	/* Make thread pool local. */
	pool = thr->thread_pool;

#ifdef OMP_PERF
	memset(&pea1, 0, sizeof(struct perf_event_attr));
	pea1.type = 8;
	pea1.size = sizeof(struct perf_event_attr);
	pea1.config = 0x0008; // instruction
	pea1.disabled = 1;
	pea1.exclude_hv = 1;
	pea1.inherit = 1;
	//pea1.exclusive = 1;
	pea1.exclude_kernel = 1;
	pea1.exclude_idle = 0;
	// pea1.exclude_callchain_kernel = 1;
	// pea1.exclude_callchain_user = 1;
	// pea1.exclude_guest = 1;
	pea1.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
    //w_start = omp_get_wtime();
	fd1[tid] = syscall(__NR_perf_event_open, &pea1, tid, -1, -1, 0);
    //w_end = omp_get_wtime();
    //printf("syscal time: %f\n", w_end - w_start);
	ioctl(fd1[tid], PERF_EVENT_IOC_ID, &id1[tid]);

#endif
	if (data->nested)
	{
		struct gomp_team *team = thr->ts.team;
		struct gomp_task *task = thr->task;

		gomp_barrier_wait(&team->barrier);

		local_fn(local_data);
		gomp_team_barrier_wait_final(&team->barrier);
		gomp_finish_task(task);
		gomp_barrier_wait_last(&team->barrier);
	}
	else
	{
		pool->threads[thr->ts.team_id] = thr;

		gomp_simple_barrier_wait(&pool->threads_dock);
		do
		{
			struct gomp_team *team = thr->ts.team;
			struct gomp_task *task = thr->task;

#ifdef OMP_PERF
	if (profile_flag == 1 || read_backend_flag == 1) {

			memset(buf1, 0, sizeof(char) * BUF_LEN);
			ioctl(fd1[tid], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
			//printf("%ld: start local_fn\n", gettid());
			ioctl(fd1[tid], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);

			active_flag[tid] = 1;
	}
#endif
			local_fn(local_data);
#ifdef OMP_PERF
	if (profile_flag == 1 || read_backend_flag == 1) {
			active_flag[tid] = 0;

			ioctl(fd1[tid], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
			read(fd1[tid], buf1, sizeof(buf1));
			for (i = 0; i < rf1->nr; i++)
			{
				if (rf1->values[i].id == id1[tid])
				{
					val1_t = rf1->values[i].value;
					break;
				}
			}
			__sync_fetch_and_add(&instruction_num, val1_t);
	}
#endif

			gomp_team_barrier_wait_final(&team->barrier);
			gomp_finish_task(task);

#ifdef OMP_WAKEUP
		do {
#endif
			gomp_simple_barrier_wait(&pool->threads_dock);

			local_fn = thr->fn;
			local_data = thr->data;
			thr->fn = NULL;
#ifdef OMP_WAKEUP
		} while (!local_fn && !threadnum_flag);
#endif

#ifdef OMP_MIGRATION
			if (local_fn && thr->place != thr->old_place)
			{
				CPU_ZERO(&mask);
				CPU_SET(thr->place - 1, &mask);
				if (sched_setaffinity(tid, sizeof(cpu_set_t), &mask) == -1)
				{
					printf("sched_setaffinity failed\n");
				}

			}
#endif

		} while (local_fn);
	}

	gomp_sem_destroy(&thr->release);
	pthread_detach(pthread_self());
	thr->thread_pool = NULL;
	thr->task = NULL;
	return NULL;
}
#endif

static inline struct gomp_team *
get_last_team(unsigned nthreads)
{
	struct gomp_thread *thr = gomp_thread();
	if (thr->ts.team == NULL)
	{
		struct gomp_thread_pool *pool = gomp_get_thread_pool(thr, nthreads);
		struct gomp_team *last_team = pool->last_team;
		if (last_team != NULL && last_team->nthreads == nthreads)
		{
			pool->last_team = NULL;
			return last_team;
		}
	}
	return NULL;
}

/* Create a new team data structure.  */

struct gomp_team *
gomp_new_team(unsigned nthreads)
{
	struct gomp_team *team;
	int i;

	team = get_last_team(nthreads);
	if (team == NULL)
	{
		size_t extra = sizeof(team->ordered_release[0]) + sizeof(team->implicit_task[0]);
		team = gomp_malloc(sizeof(*team) + nthreads * extra);

#ifndef HAVE_SYNC_BUILTINS
		gomp_mutex_init(&team->work_share_list_free_lock);
#endif
		gomp_barrier_init(&team->barrier, nthreads);
		gomp_mutex_init(&team->task_lock);

		team->nthreads = nthreads;
	}

	team->work_share_chunk = 8;
#ifdef HAVE_SYNC_BUILTINS
	team->single_count = 0;
#endif
	team->work_shares_to_free = &team->work_shares[0];
	gomp_init_work_share(&team->work_shares[0], 0, nthreads);
	team->work_shares[0].next_alloc = NULL;
	team->work_share_list_free = NULL;
	team->work_share_list_alloc = &team->work_shares[1];
	for (i = 1; i < 7; i++)
		team->work_shares[i].next_free = &team->work_shares[i + 1];
	team->work_shares[i].next_free = NULL;

	gomp_sem_init(&team->master_release, 0);
	team->ordered_release = (void *)&team->implicit_task[nthreads];
	team->ordered_release[0] = &team->master_release;

	priority_queue_init(&team->task_queue);
	team->task_count = 0;
	team->task_queued_count = 0;
	team->task_running_count = 0;
	team->work_share_cancelled = 0;
	team->team_cancelled = 0;

	return team;
}

/* Free a team data structure.  */

static void
free_team(struct gomp_team *team)
{
#ifndef HAVE_SYNC_BUILTINS
	gomp_mutex_destroy(&team->work_share_list_free_lock);
#endif
	gomp_barrier_destroy(&team->barrier);
	gomp_mutex_destroy(&team->task_lock);
	priority_queue_free(&team->task_queue);
	free(team);
}

static void
gomp_free_pool_helper(void *thread_pool)
{
	struct gomp_thread *thr = gomp_thread();
	struct gomp_thread_pool *pool = (struct gomp_thread_pool *)thread_pool;
	gomp_simple_barrier_wait_last(&pool->threads_dock);
	gomp_sem_destroy(&thr->release);
	thr->thread_pool = NULL;
	thr->task = NULL;
#ifdef LIBGOMP_USE_PTHREADS
	pthread_detach(pthread_self());
	pthread_exit(NULL);
#elif defined(__nvptx__)
	asm("exit;");
#else
#error gomp_free_pool_helper must terminate the thread
#endif
}

/* Free a thread pool and release its threads. */

void gomp_free_thread(void *arg __attribute__((unused)))
{
	struct gomp_thread *thr = gomp_thread();
	struct gomp_thread_pool *pool = thr->thread_pool;
	if (pool)
	{
		if (pool->threads_used > 0)
		{
			int i;
			for (i = 1; i < pool->threads_used; i++)
			{
				struct gomp_thread *nthr = pool->threads[i];
				nthr->fn = gomp_free_pool_helper;
				nthr->data = pool;
			}
			/* This barrier undocks threads docked on pool->threads_dock.  */
			gomp_simple_barrier_wait(&pool->threads_dock);
			/* And this waits till all threads have called gomp_barrier_wait_last
	     in gomp_free_pool_helper.  */
			gomp_simple_barrier_wait(&pool->threads_dock);
			/* Now it is safe to destroy the barrier and free the pool.  */
			gomp_simple_barrier_destroy(&pool->threads_dock);

#ifdef HAVE_SYNC_BUILTINS
			__sync_fetch_and_add(&gomp_managed_threads,
								 1L - pool->threads_used);
#else
			gomp_mutex_lock(&gomp_managed_threads_lock);
			gomp_managed_threads -= pool->threads_used - 1L;
			gomp_mutex_unlock(&gomp_managed_threads_lock);
#endif
		}
		if (pool->last_team)
			free_team(pool->last_team);
#ifndef __nvptx__
		free(pool->threads);
		free(pool);
#endif
		thr->thread_pool = NULL;
	}
	if (thr->ts.level == 0 && __builtin_expect(thr->ts.team != NULL, 0))
		gomp_team_end();
	if (thr->task != NULL)
	{
		struct gomp_task *task = thr->task;
		gomp_end_task();
		free(task);
	}
}

/* Launch a team.  */

#ifdef LIBGOMP_USE_PTHREADS
void gomp_team_start(void (*fn)(void *), void *data, unsigned nthreads,
					 unsigned flags, struct gomp_team *team,
					 struct gomp_taskgroup *taskgroup)
{
	struct gomp_thread_start_data *start_data = NULL;
	struct gomp_thread *thr, *nthr;
	struct gomp_task *task;
	struct gomp_task_icv *icv;
	bool nested;
	struct gomp_thread_pool *pool;
	unsigned i, n, old_threads_used = 0;
	pthread_attr_t thread_attr, *attr;
	unsigned long nthreads_var;
	char bind, bind_var;
	unsigned int s = 0, rest = 0, p = 0, k = 0;
	unsigned int affinity_count = 0;
	struct gomp_thread **affinity_thr = NULL;
	bool force_display = false;

#ifdef OMP_MIGRATION
#define MAX_THR 100
	unsigned j = 0, l = 0;
	unsigned index = 0, index_temp;
	struct gomp_thread *nthr_1, *nthr_2;
	unsigned thr_array[100];
#endif

#ifdef OMP_WAKEUP
	unsigned j;
#endif

	thr = gomp_thread();
	nested = thr->ts.level;
	pool = thr->thread_pool;
	task = thr->task;
	icv = task ? &task->icv : &gomp_global_icv;
	if (__builtin_expect(gomp_places_list != NULL, 0) && thr->place == 0)
	{
		gomp_init_affinity();
		if (__builtin_expect(gomp_display_affinity_var, 0) && nthreads == 1)
			gomp_display_affinity_thread(gomp_thread_self(), &thr->ts,
										 thr->place);
	}

	/* Always save the previous state, even if this isn't a nested team.
     In particular, we should save any work share state from an outer
     orphaned work share construct.  */
	team->prev_ts = thr->ts;

	thr->ts.team = team;
	thr->ts.team_id = 0;
	++thr->ts.level;
	if (nthreads > 1)
		++thr->ts.active_level;
	thr->ts.work_share = &team->work_shares[0];
	thr->ts.last_work_share = NULL;
#ifdef HAVE_SYNC_BUILTINS
	thr->ts.single_count = 0;
#endif
	thr->ts.static_trip = 0;
	thr->task = &team->implicit_task[0];
#ifdef GOMP_NEEDS_THREAD_HANDLE
	thr->handle = pthread_self();
#endif
	nthreads_var = icv->nthreads_var;
	if (__builtin_expect(gomp_nthreads_var_list != NULL, 0) && thr->ts.level < gomp_nthreads_var_list_len)
		nthreads_var = gomp_nthreads_var_list[thr->ts.level];
	bind_var = icv->bind_var;
	#ifdef OMP_NEW
	if (flags != 0) {
		bind_var = flags & 7;
	}
	#else
	if (bind_var != omp_proc_bind_false && (flags & 7) != omp_proc_bind_false)
		bind_var = flags & 7;
	#endif
	bind = bind_var;
	if (__builtin_expect(gomp_bind_var_list != NULL, 0) && thr->ts.level < gomp_bind_var_list_len)
		bind_var = gomp_bind_var_list[thr->ts.level];
	gomp_init_task(thr->task, task, icv);
	thr->task->taskgroup = taskgroup;
	team->implicit_task[0].icv.nthreads_var = nthreads_var;
	team->implicit_task[0].icv.bind_var = bind_var;

	if (nthreads == 1)
		return;

	i = 1;

	if (__builtin_expect(gomp_places_list != NULL, 0))
	{
		/* Depending on chosen proc_bind model, set subpartition
	 for the master thread and initialize helper variables
	 P and optionally S, K and/or REST used by later place
	 computation for each additional thread.  */
		p = thr->place - 1;
		switch (bind)
		{
		case omp_proc_bind_true:
		case omp_proc_bind_close:
			if (nthreads > thr->ts.place_partition_len)
			{
				/* T > P.  S threads will be placed in each place,
		 and the final REM threads placed one by one
		 into the already occupied places.  */
				s = nthreads / thr->ts.place_partition_len;
				rest = nthreads % thr->ts.place_partition_len;
			}
			else
				s = 1;
			k = 1;
			break;
		case omp_proc_bind_master:
			/* Each thread will be bound to master's place.  */
			break;
		case omp_proc_bind_spread:
			if (nthreads <= thr->ts.place_partition_len)
			{
				/* T <= P.  Each subpartition will have in between s
		 and s+1 places (subpartitions starting at or
		 after rest will have s places, earlier s+1 places),
		 each thread will be bound to the first place in
		 its subpartition (except for the master thread
		 that can be bound to another place in its
		 subpartition).  */
				s = thr->ts.place_partition_len / nthreads;
				rest = thr->ts.place_partition_len % nthreads;
				rest = (s + 1) * rest + thr->ts.place_partition_off;
				if (p < rest)
				{
					p -= (p - thr->ts.place_partition_off) % (s + 1);
					thr->ts.place_partition_len = s + 1;
				}
				else
				{
					p -= (p - rest) % s;
					thr->ts.place_partition_len = s;
				}
				thr->ts.place_partition_off = p;
			}
			else
			{
				/* T > P.  Each subpartition will have just a single
		 place and we'll place between s and s+1
		 threads into each subpartition.  */
				s = nthreads / thr->ts.place_partition_len;
				rest = nthreads % thr->ts.place_partition_len;
				thr->ts.place_partition_off = p;
				thr->ts.place_partition_len = 1;
				k = 1;
			}
			break;
		}
	}
	else
		bind = omp_proc_bind_false;

	/* We only allow the reuse of idle threads for non-nested PARALLEL
     regions.  This appears to be implied by the semantics of
     threadprivate variables, but perhaps that's reading too much into
     things.  Certainly it does prevent any locking problems, since
     only the initial program thread will modify gomp_threads.  */
	if (!nested)
	{
		old_threads_used = pool->threads_used;

		if (nthreads <= old_threads_used)
			n = nthreads;
		else if (old_threads_used == 0)
		{
			n = 0;
			gomp_simple_barrier_init(&pool->threads_dock, nthreads);
		}
		else
		{
			n = old_threads_used;

			/* Increase the barrier threshold to make sure all new
	     threads arrive before the team is released.  */
			gomp_simple_barrier_reinit(&pool->threads_dock, nthreads);
		}

		/* Not true yet, but soon will be.  We're going to release all
	 threads from the dock, and those that aren't part of the
	 team will exit.  */
#ifdef OMP_WAKEUP
   if (threadnum_flag == 1) {
    pool->threads_used = nthreads;
   } else {
	pool->threads_used = 96;
   }
#else
		pool->threads_used = nthreads;
#endif

		/* If necessary, expand the size of the gomp_threads array.  It is
	 expected that changes in the number of threads are rare, thus we
	 make no effort to expand gomp_threads_size geometrically.  */
		if (nthreads >= pool->threads_size)
		{
			pool->threads_size = nthreads + 1;
			pool->threads = gomp_realloc(pool->threads,
										 pool->threads_size * sizeof(struct gomp_thread *));
			/* Add current (master) thread to threads[].  */
			pool->threads[0] = thr;
		}

#ifdef OMP_WAKEUP
		if (threadnum_flag != 1) {
			for (j = 1; j < n; j++) {
				pool->threads[j]->fn = NULL;
			}
		}

#endif

		/* Release existing idle threads.  */
		for (; i < n; ++i)
		{
			unsigned int place_partition_off = thr->ts.place_partition_off;
			unsigned int place_partition_len = thr->ts.place_partition_len;
			unsigned int place = 0;
#ifdef OMP_MIGRATION
			unsigned int old_place = 0;
#endif
//printf("bind = %d\n", bind);
			if (__builtin_expect(gomp_places_list != NULL, 0))
			{
				switch (bind)
				{
				case omp_proc_bind_true:
				case omp_proc_bind_close:
					if (k == s)
					{
						++p;
						if (p == (team->prev_ts.place_partition_off + team->prev_ts.place_partition_len))
							p = team->prev_ts.place_partition_off;
						k = 1;
						if (i == nthreads - rest)
							s = 1;
					}
					else
						++k;
					break;
				case omp_proc_bind_master:
					break;
				case omp_proc_bind_spread:
					if (k == 0)
					{
						/* T <= P.  */
						if (p < rest)
							p += s + 1;
						else
							p += s;
						if (p == (team->prev_ts.place_partition_off + team->prev_ts.place_partition_len))
							p = team->prev_ts.place_partition_off;
						place_partition_off = p;
						if (p < rest)
							place_partition_len = s + 1;
						else
							place_partition_len = s;
					}
					else
					{
						/* T > P.  */
						if (k == s)
						{
							++p;
							if (p == (team->prev_ts.place_partition_off + team->prev_ts.place_partition_len))
								p = team->prev_ts.place_partition_off;
							k = 1;
							if (i == nthreads - rest)
								s = 1;
						}
						else
							++k;
						place_partition_off = p;
						place_partition_len = 1;
					}
					break;
				}
#ifndef OMP_WAKEUP
				if (affinity_thr != NULL || (bind != omp_proc_bind_true && pool->threads[i]->place != p + 1) || pool->threads[i]->place <= place_partition_off || pool->threads[i]->place > (place_partition_off + place_partition_len))
				{
// simple modification
#ifndef OMP_MIGRATION
					unsigned int l;
					force_display = true;
					if (affinity_thr == NULL)
					{
						unsigned int j;

						if (team->prev_ts.place_partition_len > 64)
							affinity_thr = gomp_malloc(team->prev_ts.place_partition_len * sizeof(struct gomp_thread *));
						else
							affinity_thr = gomp_alloca(team->prev_ts.place_partition_len * sizeof(struct gomp_thread *));
						memset(affinity_thr, '\0',
							   team->prev_ts.place_partition_len * sizeof(struct gomp_thread *));
						for (j = i; j < old_threads_used; j++)
						{
							if (pool->threads[j]->place > team->prev_ts.place_partition_off && (pool->threads[j]->place <= (team->prev_ts.place_partition_off + team->prev_ts.place_partition_len)))
							{
								l = pool->threads[j]->place - 1 - team->prev_ts.place_partition_off;
								pool->threads[j]->data = affinity_thr[l];
								affinity_thr[l] = pool->threads[j];
							}
							pool->threads[j] = NULL;
						}
						if (nthreads > old_threads_used)
							memset(&pool->threads[old_threads_used],
								   '\0', ((nthreads - old_threads_used) * sizeof(struct gomp_thread *)));
						n = nthreads;
						affinity_count = old_threads_used - i;
					}
					if (affinity_count == 0)
						break;
					l = p;
					if (affinity_thr[l - team->prev_ts.place_partition_off] == NULL)
					{
						if (bind != omp_proc_bind_true)
							continue;
						for (l = place_partition_off;
							 l < place_partition_off + place_partition_len;
							 l++)
							if (affinity_thr[l - team->prev_ts.place_partition_off] != NULL)
								break;
						if (l == place_partition_off + place_partition_len)
							continue;
					}
					nthr = affinity_thr[l - team->prev_ts.place_partition_off];
					affinity_thr[l - team->prev_ts.place_partition_off] = (struct gomp_thread *)nthr->data;
					affinity_count--;
					pool->threads[i] = nthr;
#else
					nthr = pool->threads[i];
					old_place = nthr->place;
#endif
				}
				else
					nthr = pool->threads[i];
				place = p + 1;
#else
				place = p + 1;
				if (place > 0) {
					nthr = pool->threads[place-1];	
				} else {
					nthr = pool->threads[i];
				}
#endif
			}
			else
				nthr = pool->threads[i];

			nthr->ts.team = team;
			nthr->ts.work_share = &team->work_shares[0];
			nthr->ts.last_work_share = NULL;
			nthr->ts.team_id = i;
			nthr->ts.level = team->prev_ts.level + 1;
			nthr->ts.active_level = thr->ts.active_level;
			nthr->ts.place_partition_off = place_partition_off;
			nthr->ts.place_partition_len = place_partition_len;
#ifdef HAVE_SYNC_BUILTINS
			nthr->ts.single_count = 0;
#endif
			nthr->ts.static_trip = 0;
			nthr->task = &team->implicit_task[i];
			nthr->place = place;
#ifdef OMP_MIGRATION
			if (old_place == 0)
			{
				old_place = place;
			}
			nthr->old_place = old_place;
			if (place != old_place) {
				thr_array[index++] = i;
			}
#endif
			gomp_init_task(nthr->task, task, icv);
			team->implicit_task[i].icv.nthreads_var = nthreads_var;
			team->implicit_task[i].icv.bind_var = bind_var;
			nthr->task->taskgroup = taskgroup;
			nthr->fn = fn;
			nthr->data = data;
			team->ordered_release[i] = &nthr->release;
		}

#ifdef OMP_MIGRATION
		for (j = 0; j < index; ++j) {
			nthr_1 = pool->threads[thr_array[j]];
			if (nthr_1->place != nthr_1->old_place) {
				for (l = 0; l < index; l++) {
					nthr_2 = pool->threads[thr_array[l]];
					if (nthr_1->old_place == nthr_2->place) {
						index_temp = nthr_1->place;
						nthr_1->place = nthr_2->place;
						nthr_2->place = index_temp;
						break;
					}
				}
			}
		}
#endif

		if (__builtin_expect(affinity_thr != NULL, 0))
		{
			/* If AFFINITY_THR is non-NULL just because we had to
	     permute some threads in the pool, but we've managed
	     to find exactly as many old threads as we'd find
	     without affinity, we don't need to handle this
	     specially anymore.  */
			if (nthreads <= old_threads_used
					? (affinity_count == old_threads_used - nthreads)
					: (i == old_threads_used))
			{
				if (team->prev_ts.place_partition_len > 64)
					free(affinity_thr);
				affinity_thr = NULL;
				affinity_count = 0;
			}
			else
			{
				i = 1;
				/* We are going to compute the places/subpartitions
		 again from the beginning.  So, we need to reinitialize
		 vars modified by the switch (bind) above inside
		 of the loop, to the state they had after the initial
		 switch (bind).  */
				switch (bind)
				{
				case omp_proc_bind_true:
				case omp_proc_bind_close:
					if (nthreads > thr->ts.place_partition_len)
						/* T > P.  S has been changed, so needs
		       to be recomputed.  */
						s = nthreads / thr->ts.place_partition_len;
					k = 1;
					p = thr->place - 1;
					break;
				case omp_proc_bind_master:
					/* No vars have been changed.  */
					break;
				case omp_proc_bind_spread:
					p = thr->ts.place_partition_off;
					if (k != 0)
					{
						/* T > P.  */
						s = nthreads / team->prev_ts.place_partition_len;
						k = 1;
					}
					break;
				}

				/* Increase the barrier threshold to make sure all new
		 threads and all the threads we're going to let die
		 arrive before the team is released.  */
				if (affinity_count)
					gomp_simple_barrier_reinit(&pool->threads_dock,
											   nthreads + affinity_count);
			}
		}

		if (i == nthreads)
			goto do_release;
	}

	if (__builtin_expect(nthreads + affinity_count > old_threads_used, 0))
	{
		long diff = (long)(nthreads + affinity_count) - (long)old_threads_used;

		if (old_threads_used == 0)
			--diff;

#ifdef HAVE_SYNC_BUILTINS
		__sync_fetch_and_add(&gomp_managed_threads, diff);
#else
		gomp_mutex_lock(&gomp_managed_threads_lock);
		gomp_managed_threads += diff;
		gomp_mutex_unlock(&gomp_managed_threads_lock);
#endif
	}

	attr = &gomp_thread_attr;
	if (__builtin_expect(gomp_places_list != NULL, 0))
	{
		size_t stacksize;
		pthread_attr_init(&thread_attr);
		if (!pthread_attr_getstacksize(&gomp_thread_attr, &stacksize))
			pthread_attr_setstacksize(&thread_attr, stacksize);
		attr = &thread_attr;
	}

	start_data = gomp_alloca(sizeof(struct gomp_thread_start_data) * (nthreads - i));

	/* Launch new threads.  */
	for (; i < nthreads; ++i)
	{
		int err;

		start_data->ts.place_partition_off = thr->ts.place_partition_off;
		start_data->ts.place_partition_len = thr->ts.place_partition_len;
		start_data->place = 0;
		if (__builtin_expect(gomp_places_list != NULL, 0))
		{
			switch (bind)
			{
			case omp_proc_bind_true:
			case omp_proc_bind_close:
				if (k == s)
				{
					++p;
					if (p == (team->prev_ts.place_partition_off + team->prev_ts.place_partition_len))
						p = team->prev_ts.place_partition_off;
					k = 1;
					if (i == nthreads - rest)
						s = 1;
				}
				else
					++k;
				break;
			case omp_proc_bind_master:
				break;
			case omp_proc_bind_spread:
				if (k == 0)
				{
					/* T <= P.  */
					if (p < rest)
						p += s + 1;
					else
						p += s;
					if (p == (team->prev_ts.place_partition_off + team->prev_ts.place_partition_len))
						p = team->prev_ts.place_partition_off;
					start_data->ts.place_partition_off = p;
					if (p < rest)
						start_data->ts.place_partition_len = s + 1;
					else
						start_data->ts.place_partition_len = s;
				}
				else
				{
					/* T > P.  */
					if (k == s)
					{
						++p;
						if (p == (team->prev_ts.place_partition_off + team->prev_ts.place_partition_len))
							p = team->prev_ts.place_partition_off;
						k = 1;
						if (i == nthreads - rest)
							s = 1;
					}
					else
						++k;
					start_data->ts.place_partition_off = p;
					start_data->ts.place_partition_len = 1;
				}
				break;
			}
			start_data->place = p + 1;
			if (affinity_thr != NULL && pool->threads[i] != NULL)
				continue;
			gomp_init_thread_affinity(attr, p);
		}

		start_data->fn = fn;
		start_data->fn_data = data;
		start_data->ts.team = team;
		start_data->ts.work_share = &team->work_shares[0];
		start_data->ts.last_work_share = NULL;
		start_data->ts.team_id = i;
		start_data->ts.level = team->prev_ts.level + 1;
		start_data->ts.active_level = thr->ts.active_level;
#ifdef HAVE_SYNC_BUILTINS
		start_data->ts.single_count = 0;
#endif
		start_data->ts.static_trip = 0;
		start_data->task = &team->implicit_task[i];
		gomp_init_task(start_data->task, task, icv);
		team->implicit_task[i].icv.nthreads_var = nthreads_var;
		team->implicit_task[i].icv.bind_var = bind_var;
		start_data->task->taskgroup = taskgroup;
		start_data->thread_pool = pool;
		start_data->nested = nested;

		attr = gomp_adjust_thread_attr(attr, &thread_attr);
		err = pthread_create(&start_data->handle, attr, gomp_thread_start,
							 start_data);

		start_data++;
		if (err != 0)
			gomp_fatal("Thread creation failed: %s", strerror(err));
	}

	if (__builtin_expect(attr == &thread_attr, 0))
		pthread_attr_destroy(&thread_attr);

do_release:
	if (nested)
		gomp_barrier_wait(&team->barrier);
	else
		gomp_simple_barrier_wait(&pool->threads_dock);

	/* Decrease the barrier threshold to match the number of threads
     that should arrive back at the end of this team.  The extra
     threads should be exiting.  Note that we arrange for this test
     to never be true for nested teams.  If AFFINITY_COUNT is non-zero,
     the barrier as well as gomp_managed_threads was temporarily
     set to NTHREADS + AFFINITY_COUNT.  For NTHREADS < OLD_THREADS_COUNT,
     AFFINITY_COUNT if non-zero will be always at least
     OLD_THREADS_COUNT - NTHREADS.  */
	if (__builtin_expect(nthreads < old_threads_used, 0) || __builtin_expect(affinity_count, 0))
	{
		long diff = (long)nthreads - (long)old_threads_used;

		if (affinity_count)
			diff = -affinity_count;

#ifdef OMP_WAKEUP
	if (threadnum_flag != 1) {
		gomp_simple_barrier_reinit(&pool->threads_dock, 96);
	} else {
		gomp_simple_barrier_reinit(&pool->threads_dock, nthreads);
	}
#else
		gomp_simple_barrier_reinit(&pool->threads_dock, nthreads);
#endif

#ifdef HAVE_SYNC_BUILTINS
		__sync_fetch_and_add(&gomp_managed_threads, diff);
#else
		gomp_mutex_lock(&gomp_managed_threads_lock);
		gomp_managed_threads += diff;
		gomp_mutex_unlock(&gomp_managed_threads_lock);
#endif
	}
	if (__builtin_expect(gomp_display_affinity_var, 0))
	{
		if (nested || nthreads != old_threads_used || force_display)
		{
			gomp_display_affinity_thread(gomp_thread_self(), &thr->ts,
										 thr->place);
			if (nested)
			{
				start_data -= nthreads - 1;
				for (i = 1; i < nthreads; ++i)
				{
					gomp_display_affinity_thread(
#ifdef LIBGOMP_USE_PTHREADS
						start_data->handle,
#else
						gomp_thread_self(),
#endif
						&start_data->ts,
						start_data->place);
					start_data++;
				}
			}
			else
			{
				for (i = 1; i < nthreads; ++i)
				{
					gomp_thread_handle handle = gomp_thread_to_pthread_t(pool->threads[i]);
					gomp_display_affinity_thread(handle, &pool->threads[i]->ts,
												 pool->threads[i]->place);
				}
			}
		}
	}
	if (__builtin_expect(affinity_thr != NULL, 0) && team->prev_ts.place_partition_len > 64)
		free(affinity_thr);
}
#endif

/* Terminate the current team.  This is only to be called by the master
   thread.  We assume that we must wait for the other threads.  */

void gomp_team_end(void)
{
	struct gomp_thread *thr = gomp_thread();
	struct gomp_team *team = thr->ts.team;

	/* This barrier handles all pending explicit threads.
     As #pragma omp cancel parallel might get awaited count in
     team->barrier in a inconsistent state, we need to use a different
     counter here.  */
	gomp_team_barrier_wait_final(&team->barrier);
	if (__builtin_expect(team->team_cancelled, 0))
	{
		struct gomp_work_share *ws = team->work_shares_to_free;
		do
		{
			struct gomp_work_share *next_ws = gomp_ptrlock_get(&ws->next_ws);
			if (next_ws == NULL)
				gomp_ptrlock_set(&ws->next_ws, ws);
			gomp_fini_work_share(ws);
			ws = next_ws;
		} while (ws != NULL);
	}
	else
		gomp_fini_work_share(thr->ts.work_share);

	gomp_end_task();
	thr->ts = team->prev_ts;

	if (__builtin_expect(thr->ts.level != 0, 0))
	{
#ifdef HAVE_SYNC_BUILTINS
		__sync_fetch_and_add(&gomp_managed_threads, 1L - team->nthreads);
#else
		gomp_mutex_lock(&gomp_managed_threads_lock);
		gomp_managed_threads -= team->nthreads - 1L;
		gomp_mutex_unlock(&gomp_managed_threads_lock);
#endif
		/* This barrier has gomp_barrier_wait_last counterparts
	 and ensures the team can be safely destroyed.  */
		gomp_barrier_wait(&team->barrier);
	}

	if (__builtin_expect(team->work_shares[0].next_alloc != NULL, 0))
	{
		struct gomp_work_share *ws = team->work_shares[0].next_alloc;
		do
		{
			struct gomp_work_share *next_ws = ws->next_alloc;
			free(ws);
			ws = next_ws;
		} while (ws != NULL);
	}
	gomp_sem_destroy(&team->master_release);

	if (__builtin_expect(thr->ts.team != NULL, 0) || __builtin_expect(team->nthreads == 1, 0))
		free_team(team);
	else
	{
		struct gomp_thread_pool *pool = thr->thread_pool;
		if (pool->last_team)
			free_team(pool->last_team);
		pool->last_team = team;
		gomp_release_thread_pool(pool);
	}
}

#ifdef LIBGOMP_USE_PTHREADS

/* Constructors for this file.  */

static void __attribute__((constructor))
initialize_team(void)
{
#if !defined HAVE_TLS && !defined USE_EMUTLS
	static struct gomp_thread initial_thread_tls_data;

	pthread_key_create(&gomp_tls_key, NULL);
	pthread_setspecific(gomp_tls_key, &initial_thread_tls_data);
#endif

	if (pthread_key_create(&gomp_thread_destructor, gomp_free_thread) != 0)
		gomp_fatal("could not create thread pool destructor.");
}

static void __attribute__((destructor))
team_destructor(void)
{
	/* Without this dlclose on libgomp could lead to subsequent
     crashes.  */
	pthread_key_delete(gomp_thread_destructor);
}

/* Similar to gomp_free_pool_helper, but don't detach itself,
   gomp_pause_host will pthread_join those threads.  */

static void
gomp_pause_pool_helper(void *thread_pool)
{
	struct gomp_thread *thr = gomp_thread();
	struct gomp_thread_pool *pool = (struct gomp_thread_pool *)thread_pool;
	gomp_simple_barrier_wait_last(&pool->threads_dock);
	gomp_sem_destroy(&thr->release);
	thr->thread_pool = NULL;
	thr->task = NULL;
	pthread_exit(NULL);
}

/* Free a thread pool and release its threads.  Return non-zero on
   failure.  */

int gomp_pause_host(void)
{
	struct gomp_thread *thr = gomp_thread();
	struct gomp_thread_pool *pool = thr->thread_pool;
	if (thr->ts.level)
		return -1;
	if (pool)
	{
		if (pool->threads_used > 0)
		{
			int i;
			pthread_t *thrs = gomp_alloca(sizeof(pthread_t) * pool->threads_used);
			for (i = 1; i < pool->threads_used; i++)
			{
				struct gomp_thread *nthr = pool->threads[i];
				nthr->fn = gomp_pause_pool_helper;
				nthr->data = pool;
				thrs[i] = gomp_thread_to_pthread_t(nthr);
			}
			/* This barrier undocks threads docked on pool->threads_dock.  */
			gomp_simple_barrier_wait(&pool->threads_dock);
			/* And this waits till all threads have called gomp_barrier_wait_last
	     in gomp_pause_pool_helper.  */
			gomp_simple_barrier_wait(&pool->threads_dock);
			/* Now it is safe to destroy the barrier and free the pool.  */
			gomp_simple_barrier_destroy(&pool->threads_dock);

#ifdef HAVE_SYNC_BUILTINS
			__sync_fetch_and_add(&gomp_managed_threads,
								 1L - pool->threads_used);
#else
			gomp_mutex_lock(&gomp_managed_threads_lock);
			gomp_managed_threads -= pool->threads_used - 1L;
			gomp_mutex_unlock(&gomp_managed_threads_lock);
#endif
			for (i = 1; i < pool->threads_used; i++)
				pthread_join(thrs[i], NULL);
		}
		if (pool->last_team)
			free_team(pool->last_team);
#ifndef __nvptx__
		free(pool->threads);
		free(pool);
#endif
		thr->thread_pool = NULL;
	}
	return 0;
}
#endif

struct gomp_task_icv *
gomp_new_icv(void)
{
	struct gomp_thread *thr = gomp_thread();
	struct gomp_task *task = gomp_malloc(sizeof(struct gomp_task));
	gomp_init_task(task, NULL, &gomp_global_icv);
	thr->task = task;
#ifdef LIBGOMP_USE_PTHREADS
	pthread_setspecific(gomp_thread_destructor, thr);
#endif
	return &task->icv;
}
