#ifndef HMTT_CONFIG_H
#define HMTT_CONFIG_H


 #define PREFETCH_ON

// #define exit_prefetch_when_nomem



//#define EVICT_ON
//#define USING_LRU
//#define USING_RRIP
//#define USING_FIFO



#include "memory_manage.h"
#include "evict.h"
#include "async_evict.h"
#include "store_file.h"

#include <queue>



#define STORE_FILE
#define USING_FASTSWAP

// #define use_stream_evict 1

//#define use_kernel_fifo_alg
// #define use_userspace_alg



#define max_prefetcher_count 3

#define PREFETCH_SEEK_CORE 16
#define PREFETCH_CORE_ID 7

//#define PREFETCH_CORE_ID 17
#define ASYNC_EVICT_CORE_ID 18


#define EVICT_CORE_ID 19
//#define EVICT_CORE_ID 18
#define EVICT_PTHREAD_NUM 1

#define ltarg_max_evictor_size 4
#define ltarg_max_evict_batch 32
// pref

static const char *lt_access_filename = "/dev/lt_accesspage_state";

#define MIN_PAGE_NUM 3000



// page state
#define lt_hmtt_page_unused  (6)
#define lt_hmtt_page_free  0
#define lt_hmtt_page_using 1
// just leave the list, will change later
// currently just remove from memlist, but not free state
#define lt_hmtt_page_unstable 3

// #define lt_check_page_charging(ppn) ( page_state_array[ppn].state & 1)
#define lt_check_page_charging(ppn) ( page_state_array[ppn].state == lt_hmtt_page_using)
#define lt_check_page_inuse(ppn) ( page_state_array[ppn].state  & 1)


// will not evict from userlru if userlru size is small than bound
// #define mem_protect_bound 10000

// for cg
#define mem_protect_bound (2*MIN_PAGE_NUM)

// pressure evict

#ifdef use_userspace_alg


#define use_userspace_lru_alg

// #define use_userspace_fifo_alg
#endif


// err code

// ppn cound not cover to page
#define lt_err_pagenotfound 1
// page is not used
// or page is not mapped
// give up to reclaim it.
#define lt_err_pagenotmapped 2

// cound not find process with this pid
#define lt_err_piderr 3

// this process is not belong to target
#define lt_err_pidoutofmanagement 4

// this address is not belong to target
#define lt_err_vpnoutofmanagement 5

// pte is assigned yet
#define lt_err_ptenotassigned 6

// pte is not present
#define lt_err_ptenotpresent 7

// pte is present
#define lt_err_ptepresent 8

// pte is changed by others
#define lt_err_otherschangeit 9

// no mem when try to prefetch
#define lt_err_nomem 10




#endif
