#include <stdio.h>
#include <perfmon/pfmlib.h>
#include <perfmon/pfmlib_perf_event.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)

#define NPROC 96
#define NCTR 1
#define PERF_FORMAT_SCALE (PERF_FORMAT_TOTAL_TIME_ENABLED|PERF_FORMAT_TOTAL_TIME_RUNNING)

struct pfm_result {
	long long ctr[NCTR];
};

extern void cilk_pfm_init(int tid, int nproc);

extern void do_read_before(int i);
extern void do_read_after(int i);

extern void numastat_info(int id, long long *values);
extern void read_groups(int id, long long *values);
extern void read_pfm_before(int id);
extern void read_pfm_after(int id, struct pfm_result *result);
//extern void calculate_pfm(int id);

