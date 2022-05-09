#ifndef LTLS_INTER
#define LTLS_INTER

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <iostream>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>     /* getopt() */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* uintmax_t */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */

#include <omp.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
 #include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "config.h"

#define lt_queue_max_size ( (1UL << 18))

#define max_latency_len 1000000

#define lt_1g_size (1UL << 18)

// total pfn size, use to inter with hmtt 
#define lt_pfn_size ( 64 * lt_1g_size)


// static const char *lt_init_buff_filename = "/dev/lt_init_data";
// static const char *lt_page_state_filename = "/dev/lt_page_state";
// static const char *lt_arg_state_filename = "/dev/lt_arg_state";

#define lt_hmtt_total_size 0
#define lt_hmtt_used_size 1
#define lt_hmtt_free_size 2

#define lt_hmtt_page_using 0
#define lt_hmtt_page_free  1


/*
struct hmtt_page_state
{
    // in fact it is in with cpu's list
    unsigned int cpu;

    // page state
    // 0 means page is no use or just in cache
    // 1 means page is using or said mapped
    unsigned int state;
};
*/

struct ltarg_page
{
    // identify the page mode.
    char    flags;

    // also present the page result
    char    result;

    // flags=0, pid will be meaningless
    unsigned int    pid;
    // flags=0 means ppn
    // flags=1 means vpn
    unsigned long   page_num;
};


#define init_buffer_size (( lt_pfn_size + 3) * sizeof(unsigned long))
#define page_state_size (( lt_pfn_size + 3) * sizeof(struct hmtt_page_state))
#define ltarg_buffer_size ( ltarg_max_evict_batch * ltarg_max_evictor_size * sizeof(struct ltarg_page))


#define lt_fetch_idx 0
#define lt_evict_idx 1

// Mhz
#define lt_cpu_frequence 2200 
#define lt_max_err_code 10


#define __u64 unsigned long long 
#define __u32 unsigned int 



struct lt_queue_work{
    unsigned int flags;
    unsigned int pid;
    unsigned long page_num;
};
struct lt_queue{    
    // atomic use __sync_fetch_and_add

    // the last using idx in the queue;
    unsigned long __queue_real_begin;
    // the older not use entry idx
    unsigned long beg;
    // the newest not used entry idx
    unsigned long end;

    // this evictor last used entry idx. update when the evictor finished its job
    // use to update the __queue_real_begin with the  minimum one here.
    unsigned long local_last[ltarg_max_evictor_size];

    // do not wait for the overflow message. just drop it.
    unsigned long drop_full_msg_counter;
    
    struct lt_queue_work qwork[lt_queue_max_size];
};

struct lt_evictor_arg{
    unsigned int tid;

};

#define qwork_idx(x) (((x)%lt_queue_max_size))


int init_ltls_inter(void);
int end_ltls_inter(void);

int get_ltls_statics(void);

int make_reclaim_page(unsigned int evictor_idx, unsigned int len);
int make_fetch_page(unsigned int pid, unsigned long vpn);

int make_reclaim_lazy_async(unsigned int pid, unsigned long vpn);

int reset_statics(void);

int lt_queue_size(void);


#define min(a,b) (((a)>(b))?(b):(a))
#endif
