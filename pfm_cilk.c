#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>

#include <sched.h>
#include "pfm_cilk.h"

struct perf_event_attr hw[NPROC][NCTR];
long long before[NPROC][NCTR], after[NPROC][NCTR];
unsigned int fd[NPROC][NCTR];

static const char *counter[NCTR] = {
    "L1-DCACHE-LOAD-MISSES",
//	"cycles", "instructions",
//	"cycles", "PERF_COUNT_HW_CACHE_MISSES",
};

void do_setup(int id, int nproc)
{
	struct perf_event_attr attr;
	struct perf_event_attr attr2;
	int i = 0, ret;
	int group_fd;
	long long count;
//	pfm_perf_encode_arg_t arg;
	
	memset(&(hw[id][0]), 0, NCTR*sizeof(struct perf_event_attr));		
	

	for(i=0;i<NCTR;i++) {
		ret = pfm_get_perf_event_encoding(counter[i], PFM_PLM0 | PFM_PLM3, &(hw[id][i]), NULL, NULL);
		if(ret != PFM_SUCCESS)
			printf("%d cannot get encoding %s----\n", id, counter[i]);
		else {
//			hw[id][j].read_format = PERF_FORMAT_SCALE;
			if(i==0) {
				group_fd = -1;
				hw[id][i].read_format = PERF_FORMAT_GROUP;
			}
			else group_fd=fd[id][0];

			fd[id][i] = perf_event_open(&(hw[id][i]), 0, -1, group_fd, 0);	
			
			ioctl(fd[id][i], PERF_EVENT_IOC_ENABLE, 0);
		}
	}
			
}

void numastat_info(int id, long long *values) {
    //get the numastat info
    FILE *fp;
    char path[1035];
    char test[1035];

    /* Open the command for reading. */
    fp = popen("numastat", "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    int i = 0;
    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
        if (i == 5) {
            sscanf(path, "%s %ld %ld %ld %ld", test,
                   &values[0], &values[1],
                   &values[2], &values[3]);
        }
        if (i == 6){
            sscanf(path, "%s %ld %ld %ld %ld", test,
                   &values[4], &values[5],
                   &values[6], &values[7]);
            break;
        }
        memset(path, 0, sizeof(path));

        i++;
    }
    pclose(fp);
}


void read_groups(int id, long long *values) {
	int num_ctrs;
	int size;
	int ret;
	int i;

	long long values2[NCTR+1];	
	size = sizeof(long long) * (NCTR+1);

	ret = read(fd[id][0], values, size);	

	if(ret == -1)	printf("cannot read the counters\n");

//	else {
//		for(i=0;i<NCTR;i++)
//			printf("%lld \t", values[i]);
//		printf("\n");
//	}

}

void read_pfm_before(int id) {

	int i = 0;
	int ret;
	
	for(i=0;i<NCTR;i++)	{
		ioctl(fd[id][i], PERF_EVENT_IOC_ENABLE, 0);
//		printf("value before on %d is %lld\t", id, before[id][i]);
	}
//	printf("\n");
}

void read_pfm_after(int id, struct pfm_result *result) {
	int i = 0;
	int ret;
	long long count;
	
	for(i=0;i<NCTR;i++)	{
		count = 0;
		ioctl(fd[id][i], PERF_EVENT_IOC_DISABLE, 0);
		read(fd[id][i], &(result->ctr[i]), sizeof(long long));
		
//		ret = read(fd[id][i], &(after[id][i]), sizeof(long long));
//		printf("value after on %d is %lld\t", id, before[id][i]);
	}
//	printf("\n");
}

void calculate_pfm(int id) {
	int i = 0;
	
	for(i=0;i<NCTR;i++) {
		printf("value on %d is %lld\t", id, after[id][i]-before[id][i]);
	}
	printf("\n");
}

void cilk_pfm_init(int tid, int nproc) 
{
    //printf("lgq: cilkpfm_init: tid = %d, nproc = %d\n", tid, nproc);
	int ret;	//return value
	cpu_set_t mask;

	//pin each worker to an individual core
	CPU_ZERO(&mask);
	CPU_SET(tid, &mask);
	
	if(sched_setaffinity(0,sizeof(mask), &mask)) {
		printf("fails to pin workers to cores\n");
		exit(EXIT_FAILURE);
	}

	ret=pfm_initialize();
	
	if(ret != PFM_SUCCESS)
		printf("fail to initialize pfm\n");

	do_setup(tid, nproc);
}
