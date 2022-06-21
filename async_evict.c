#include "memory_manage.h"
#include "prefetch_mine.h"
#include "config.h"
#include "lt_profile.h"


#define RECLAIM_BATCH 32
#define EVICT_BUFFER_SIZE 524288
static inline __u64 get_cycles(void)
{
        __u32 timehi, timelo;
        asm("rdtsc":"=a"(timelo),"=d"(timehi):);
        return (__u64)(((__u64)timehi)<<32 | (__u64)timelo);
}



unsigned long evict_buffer_train[EVICT_BUFFER_SIZE] = {0};
volatile unsigned long eb_w_ptr = 0, eb_r_ptr = 0;

unsigned long store_to_eb_num = 0;
unsigned long page_num_in_list = 0;


void store_to_eb(unsigned long ppn, int inter_page){
        if(ppn >= MAX_PPN)
                return ;
        
        store_to_eb_num ++;
        
        if( eb_w_ptr - eb_r_ptr > EVICT_BUFFER_SIZE ){
                printf("$$$$$ evict buffer overflow , len = %lu!!!!\n", eb_w_ptr - eb_r_ptr);
		return 0;
        }
        evict_buffer_train[ eb_w_ptr % EVICT_BUFFER_SIZE ] = ppn;
	eb_w_ptr ++;
}

unsigned long max_evicting_len = 0;
unsigned long max_evictor_len = 0;


#define MIN_SIZE 512
//#define MIN_SIZE 2524

int mem_pressure_check_num[2] = {0};
unsigned long reclaim_num_copy_num[30] = {0};


void local_mem_pressure_check(){
	unsigned long cur_free_mem_size = memory_buffer_start_addr[2]; 
	unsigned long all_mem_size = memory_buffer_start_addr[0];
	int i = 0;
//	if( cur_free_mem_size < MIN_SIZE )  //may be should check last evict finish?
	if( memory_buffer_start_addr[2] < MIN_SIZE )  //may be should check last evict finish?
	{
		//check evictor's buffer first, if the buffer is close to full, return directly
		unsigned long tmp_bufferr = get_evict_buffer_size(0);
		if(tmp_bufferr > max_evictor_len) max_evictor_len = tmp_bufferr;
//		if( get_evict_buffer_size(0) >= 72 )
		if(tmp_bufferr >= 256)
		{
			mem_pressure_check_num[1] ++;
			if(mem_pressure_check_num[0] % 2000 == 1999)
				printf("evict_buffer_size = %lu, free_mem = %lu\n", tmp_bufferr, cur_free_mem_size);
			return ;
		}
		else{
			 mem_pressure_check_num[0] ++;
		}

		// send evict command
		// get and delte lru tail, evict the pages in BATCH NUM
		for(i = 0 ; i < RECLAIM_BATCH; i++){
			struct page_list *cur_scan = lru_tail.prev;
			reclaim_num_copy_num[0] ++;
			if(reclaim_num_copy_num[0] < 20)
				printf( " reclaim_num_copy_num[0] = %lu, ppn = %lu , page_state_array[ppn].state = %d\n ", 
			reclaim_num_copy_num[0],cur_scan->ppn,page_state_array[cur_scan->ppn].state );
			if( cur_scan->ppn != 0 && lt_check_page_charging(cur_scan->ppn)){
				reclaim_num_copy_num[1] ++;
				if(reclaim_num_copy_num[1] % 5000 == 4999){
					printf("I have reclaim %lu pages, [%lu: %lu] pages in list, %lu free pages observed from kernel\n", 
				reclaim_num_copy_num[1], page_num_in_list, get_lru_size(), memory_buffer_start_addr[2]);
				}
				copy_ppn_to_evict_buffer( cur_scan->ppn , 0, 0, 0);
				list_del(&page_map_array[ cur_scan->ppn ].page_list);
				page_num_in_list --;
			}
			else if( cur_scan->ppn != 0 && !lt_check_page_charging(cur_scan->ppn)){
				reclaim_num_copy_num[2] ++;
				list_del(&page_map_array[ cur_scan->ppn ].page_list);
                                page_num_in_list --;
			}
		}
	}
}


unsigned long tmp_rdtsc[4][2] = {0};
unsigned long record_insert_ret[30] = {0};


void async_evict_seek(){
	unsigned long st,ed;
	printf("$$$ init async_seek\n");
	int ret = -1;
	while(1){
		if(eb_r_ptr < eb_w_ptr){
			unsigned long tmp_tb_len = eb_w_ptr - eb_r_ptr;
			if(tmp_tb_len > max_evicting_len){
				max_evicting_len = tmp_tb_len;
			}
			if(tmp_tb_len > 4000){
				//maybe print some msg
				;
			}
			st = get_cycles();
			unsigned long tmp_ppn = evict_buffer_train[ eb_r_ptr % EVICT_BUFFER_SIZE ];
			eb_r_ptr ++;
			// update lru
			ret = hot_page_lru_control_return(tmp_ppn);
			ed = get_cycles();
			tmp_rdtsc[0][0] += (ed - st);
			tmp_rdtsc[0][1] ++;

			if(ret == -1){
				record_insert_ret[5] ++;
			}
			else if(ret == 2){
				// its new page
				page_num_in_list ++;
				record_insert_ret[ret] ++;
			}
			else record_insert_ret[ret] ++;


			st = get_cycles();
			local_mem_pressure_check();
 
			//check state	
			ed = get_cycles();
			tmp_rdtsc[1][0] += (ed - st);
                        tmp_rdtsc[1][1] ++;
		}
		st = get_cycles();
		local_mem_pressure_check();
		ed = get_cycles(); 
                tmp_rdtsc[1][0] += (ed - st);
                tmp_rdtsc[1][1] ++;
	}
}
pthread_t tid_async_evict;
cpu_set_t mask_cpu_async_evict;

void init_async_evict(){
	pthread_create(&tid_async_evict, NULL, async_evict_seek, NULL);
	CPU_ZERO(&mask_cpu_async_evict);
	CPU_SET(ASYNC_EVICT_CORE_ID , &mask_cpu_async_evict);
	pthread_setaffinity_np(tid_async_evict, sizeof(mask_cpu_async_evict), &mask_cpu_async_evict);
}


void print_async_msg(){
	int i = 0;
	for(i = 0; i < 2; i++){
		if(tmp_rdtsc[i][1] != 0)
			printf("id = %d, average consume = %.2lf cycle\n",i , (double)tmp_rdtsc[i][0] / (double)tmp_rdtsc[i][1] );
		else printf("id = %d, no use\n", i);
//	printf("hot_page_lru_control average consume %lu, check_state average consume = %lu\n",tmp_rdtsc[0][0]/tmp_rdtsc[0][1],	tmp_rdtsc[1][0]/ tmp_rdtsc[1][1] );
	}
	printf("max_evicting_len = %lu, w_ptr = %lu, r_ptr = %lu, max_evictor_len = %lu\n", max_evicting_len, eb_w_ptr, eb_r_ptr, max_evictor_len);
	for(i = 0 ; i < 2; i++)
		printf("id = %d, mem_pressure_check_num = %d\n", i, mem_pressure_check_num[i]);
	for(i = 0 ; i < 3; i++)
		printf("reclaim_num_copy_num[%d] = %lu\n", i , reclaim_num_copy_num[i]);
	for(i = 0 ; i < 30; i++){
		if( record_insert_ret[i] != 0 ){
			if(i == 5) printf("update in high frequency, ");
			printf("record_insert_ret[%d], num = %lu\n", i, record_insert_ret[i]);
		}
	}
}

