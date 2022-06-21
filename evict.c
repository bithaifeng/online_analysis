#include "/home/lhf/lhf/hmtt_kernel_control/hmtt_kernel.h"
#include "prefetch_mine.h"
#include "page_table.h"
#include "/home/lhf/receiver_driver/cfg_content.h"

#include "config.h"

#include "lt_profile.h"
#include "memory_manage.h"


#define EVICT_PTHREAD_NUM 1



int evict_ff;

cpu_set_t mask_evict;


struct evict_task evict_task_set[ EVICT_PTHREAD_NUM ];

unsigned long evict_buffer_write_ptr, evict_buffer_read_ptr;
struct evict_buffer_struct evict_buffer[ASYNC_BUFFER_LEN] = {0};

pthread_t     	tid_evict[ EVICT_PTHREAD_NUM ];

void lt_mem_pressure_check(){

#ifdef use_userspace_alg
    // scan_ptr = lru_tail.prev;
    
    struct page_list *cur_scan=scan_ptr;
    unsigned long cur_free_mem_size = memory_buffer_start_addr[2]; 
    unsigned long all_mem_size = memory_buffer_start_addr[0]; 

    unsigned int retry_cost = MIN_PAGE_NUM*2;

    long mem_building =all_mem_size - get_lru_size();
    unsigned long ret = 0;
do_retry:
    //find and evict batch pages
    if( (cur_free_mem_size < MIN_PAGE_NUM) && (cur_scan != NULL) && (cur_scan != &lru_head) ){
        // pretect the lru building process
        mem_building =all_mem_size - get_lru_size();
        if(mem_building > mem_protect_bound){
            return;
        }

        if( cur_scan->ppn != 0 && lt_check_page_charging(cur_scan->ppn)){
            ret = evict_page(cur_scan->ppn, 0,0,0);
            // buffer full just give up
            statics_log(1);
        }
        
        // cur_scan = nptr;
        // get next access pointer
        cur_scan = push_scan();
        cur_free_mem_size = memory_buffer_start_addr[2];
    }
    

#endif

#ifdef use_kernel_fifo_alg
        if( memory_buffer_start_addr[2] < MIN_PAGE_NUM){
                // &&(memory_buffer_start_addr[0] - lt_mem_using - memory_buffer_start_addr[2] < MIN_PAGE_NUM)){
            evict_page_default(ltarg_max_evict_batch);
            statics_log(1);
        }
#endif

}

void evict_pthread( void *arg ){
	struct evict_task* task = (struct evict_task*)arg;
//	task->real_tid = pthread_self();
	printf("<*> async evict pthread init successful, id = %d\n", task->tid);
	int id = task->tid;
	unsigned long i = 0;
	int evict_num = 0;
	unsigned long page_num;
	unsigned int pid;
	char flags;
	int scan_number = 0;
	int pages_wait_to_reclaim = 0;
    
    unsigned long ret = 0;

	while(1){
		// for(i = evict_task_set[id].evict_buffer_read_ptr; i < evict_task_set[id].evict_buffer_write_ptr; i++){
        while( evict_task_set[id].evict_buffer_read_ptr < evict_task_set[id].evict_buffer_write_ptr){
			if(evict_task_set[id].evict_buffer_read_ptr < 15){
				printf("write_ptr = %lu, read_ptr = %lu\n", 
				evict_task_set[id].evict_buffer_write_ptr, evict_task_set[id].evict_buffer_read_ptr);
			}

			page_num = evict_task_set[id].evict_buffer[ (evict_task_set[id].evict_buffer_read_ptr) % ASYNC_BUFFER_LEN ].ppn;
			pid = evict_task_set[id].evict_buffer[ (evict_task_set[id].evict_buffer_read_ptr) % ASYNC_BUFFER_LEN ].pid;
			flags = evict_task_set[id].evict_buffer[ (evict_task_set[id].evict_buffer_read_ptr) % ASYNC_BUFFER_LEN ].flag;

			evict_task_set[id].evict_buffer_read_ptr ++;
			evict_num ++;
			if(evict_num % 10000 == 1 || evict_num < 20)
				printf("evict_num = %d\n", evict_num);
			

		        statics_log(0);
		        ret = evict_page(page_num, pid, flags, 0);
	}

        lt_mem_pressure_check();

	}
}


void init_evict_thread(){
	CPU_ZERO(&mask_evict);
	for(int i = 0 ; i < EVICT_PTHREAD_NUM; i ++){
		CPU_SET(EVICT_CORE_ID + i, &mask_evict);
		evict_task_set[i].tid = i;	
		evict_task_set[i].evict_buffer_write_ptr = 0;	
		evict_task_set[i].evict_buffer_read_ptr = 0;	
	}


	
	for(int i = 0 ; i < EVICT_PTHREAD_NUM; i ++){
		pthread_create(&tid_evict[i], NULL, evict_pthread, (void*)(&evict_task_set[i]  )  );
		pthread_setaffinity_np(tid_evict[i], sizeof(mask_evict), &mask_evict);
	}
}


unsigned long use_evict_copy_times = 0;

unsigned long max_evict_buffer_len = 0;
unsigned long max_evict_stream_buffer_len = 0;

// flag = 0 for ppn, 1 for vpn
unsigned long copy_ppn_to_evict_buffer(unsigned long ppn, int pid, unsigned char flag, int id){
	// check first
	if( evict_task_set[id].evict_buffer_write_ptr - evict_task_set[id].evict_buffer_read_ptr >=  ASYNC_BUFFER_LEN-1){
		// printf("evict buffer overflows!! write_ptr = 0x%lx, read_prt = 0x%lx\n", evict_task_set[id].evict_buffer_write_ptr, evict_task_set[id].evict_buffer_read_ptr);
        	return evict_task_set[id].evict_buffer_write_ptr - evict_task_set[id].evict_buffer_read_ptr;
	}
	evict_task_set[id].evict_buffer[ (evict_task_set[id].evict_buffer_write_ptr) % ASYNC_BUFFER_LEN ].ppn = ppn;
	evict_task_set[id].evict_buffer[ (evict_task_set[id].evict_buffer_write_ptr) % ASYNC_BUFFER_LEN ].pid = pid;
	evict_task_set[id].evict_buffer[ (evict_task_set[id].evict_buffer_write_ptr) % ASYNC_BUFFER_LEN ].flag = flag;
	
//	printf("write_ptr = 0x%lx, read_ptr = 0x%lx, buffer_address = 0x%lx\n", *evict_buffer_write_ptr, *evict_buffer_read_ptr,  &evict_buffer_start_addr[ (*evict_buffer_write_ptr) % ASYNC_BUFFER_LEN ]);
//	printf("pagenum = 0x%lx, pid = %d, flag = %d\n", evict_buffer_start_addr[ (*evict_buffer_write_ptr) % ASYNC_BUFFER_LEN ].ppn, pid, flag);

	evict_task_set[id].evict_buffer_write_ptr += 1;

	if( evict_task_set[id].evict_buffer_write_ptr - evict_task_set[id].evict_buffer_read_ptr > max_evict_buffer_len )
		max_evict_buffer_len = evict_task_set[id].evict_buffer_write_ptr - evict_task_set[id].evict_buffer_read_ptr;


    return evict_task_set[id].evict_buffer_write_ptr - evict_task_set[id].evict_buffer_read_ptr;
}


unsigned long get_evict_buffer_size(int id){
    return evict_task_set[id].evict_buffer_write_ptr - evict_task_set[id].evict_buffer_read_ptr;
}

/** stream copy **/
unsigned long copy_ppn_to_evict_stream_buffer(unsigned long ppn, int pid, unsigned char flag, int id){
	// check first
        if( evict_task_set[id].evict_stream_buffer_write_ptr - evict_task_set[id].evict_stream_buffer_read_ptr >=  ASYNC_BUFFER_LEN-1){
                // printf("evict buffer overflows!! write_ptr = 0x%lx, read_prt = 0x%lx\n", evict_task_set[id].evict_buffer_write_ptr, evict_task_set[id].evict_buffer_read_ptr);        
		return evict_task_set[id].evict_stream_buffer_write_ptr - evict_task_set[id].evict_stream_buffer_read_ptr;
        }
        evict_task_set[id].evict_stream_buffer[ (evict_task_set[id].evict_stream_buffer_write_ptr) % ASYNC_BUFFER_LEN ].ppn = ppn;
        evict_task_set[id].evict_stream_buffer[ (evict_task_set[id].evict_stream_buffer_write_ptr) % ASYNC_BUFFER_LEN ].pid = pid;
        evict_task_set[id].evict_stream_buffer[ (evict_task_set[id].evict_stream_buffer_write_ptr) % ASYNC_BUFFER_LEN ].flag = flag;

//      printf("write_ptr = 0x%lx, read_ptr = 0x%lx, buffer_address = 0x%lx\n", *evict_buffer_write_ptr, *evict_buffer_read_ptr,  &evict_buffer_start_addr[ (*evict_buffer_write_ptr) % ASYNC_BUFFER_LEN ]);
//      printf("pagenum = 0x%lx, pid = %d, flag = %d\n", evict_buffer_start_addr[ (*evict_buffer_write_ptr) % ASYNC_BUFFER_LEN ].ppn, pid, flag);

        evict_task_set[id].evict_stream_buffer_write_ptr += 1;

        if( evict_task_set[id].evict_stream_buffer_write_ptr - evict_task_set[id].evict_stream_buffer_read_ptr > max_evict_stream_buffer_len )
                max_evict_stream_buffer_len = evict_task_set[id].evict_stream_buffer_write_ptr - evict_task_set[id].evict_stream_buffer_read_ptr;


    return evict_task_set[id].evict_stream_buffer_write_ptr - evict_task_set[id].evict_stream_buffer_read_ptr;
}

void print_msg_evict(){
	printf("max_evict_buffer_len = %lu, evict_task_set[0].evict_buffer_write_ptr = %lu, evict_task_set[id].evict_buffer_read_ptr = %lu \n", max_evict_buffer_len, evict_task_set[0].evict_buffer_write_ptr, evict_task_set[0].evict_buffer_read_ptr );
	
}




