#include "memory_manage.h"
#include "prefetch_mine.h"
#include "config.h"
#include "lt_profile.h"
#include "rrip.h"


#define RECLAIM_BATCH 32
#define EVICT_BUFFER_SIZE 524288
static inline __u64 get_cycles(void)
{
        __u32 timehi, timelo;
        asm("rdtsc":"=a"(timelo),"=d"(timehi):);
        return (__u64)(((__u64)timehi)<<32 | (__u64)timelo);
}


#ifdef USING_BITMAP
struct evict_transfer_entry_struct evict_buffer_train[ EVICT_BUFFER_SIZE ] = {0};
#else
unsigned long evict_buffer_train[EVICT_BUFFER_SIZE] = {0};
#endif
volatile unsigned long eb_w_ptr = 0, eb_r_ptr = 0;

unsigned long store_to_eb_num = 0;
unsigned long page_num_in_list = 0;

#ifdef USING_BITMAP
void store_to_eb( struct evict_transfer_entry_struct tmp_entry )
{
	if(tmp_entry.ppn >= MAX_PPN)
                return ;

        store_to_eb_num ++;

        if( eb_w_ptr - eb_r_ptr > EVICT_BUFFER_SIZE ){
                printf("$$$$$ evict buffer overflow , len = %lu!!!!\n", eb_w_ptr - eb_r_ptr);
                return 0;
        }
        evict_buffer_train[ eb_w_ptr % EVICT_BUFFER_SIZE ].ppn = tmp_entry.ppn;
        evict_buffer_train[ eb_w_ptr % EVICT_BUFFER_SIZE ].value = tmp_entry.value;
        eb_w_ptr ++;
}
#else
void store_to_eb(unsigned long ppn, int inter_page)
{
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
#endif




unsigned long max_evicting_len = 0;
unsigned long max_evictor_len = 0;


//#define MIN_SIZE 512
#define MIN_SIZE 2500
//#define MIN_SIZE 2524

int mem_pressure_check_num[2] = {0};
unsigned long reclaim_num_copy_num[30] = {0};


unsigned long get_ppn_from_lru(){
	unsigned long ppn = 0;
	// get and delte lru tail, evict the pages in BATCH NUM
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
		ppn = cur_scan->ppn;
		list_del(&page_map_array[ cur_scan->ppn ].page_list);
		page_num_in_list --;
	}
	else if( cur_scan->ppn != 0 && !lt_check_page_charging(cur_scan->ppn)){
		reclaim_num_copy_num[2] ++;
		list_del(&page_map_array[ cur_scan->ppn ].page_list);
		page_num_in_list --;
	}
	return ppn;
}

void local_mem_pressure_check(){
	unsigned long cur_free_mem_size = memory_buffer_start_addr[2]; 
	unsigned long all_mem_size = memory_buffer_start_addr[0];
	int i = 0;
	unsigned long ppn = 0;
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

#ifndef USING_FIFO
		// send evict command
		// get and delte lru tail, evict the pages in BATCH NUM
		for(i = 0 ; i < RECLAIM_BATCH; i++){
#ifdef USING_LRU
			ppn = get_ppn_from_lru();
#endif
#ifdef USING_RRIP
			ppn = get_ppn_from_rrip(); 
#endif
			if(ppn == 0) continue;
			copy_ppn_to_evict_buffer( ppn , 0, 0, 0);
		}
#endif

#ifdef USING_FIFO
		evict_page_default(ltarg_max_evict_batch);
	        statics_log(1);
#endif

#if 0
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
#endif
	}
}

unsigned long tmp_rdtsc[4][2] = {0};
unsigned long record_insert_ret[30] = {0};
#define RESERVED_BITS 4


#define PAGE_ATTRITUTE_SCAN 0
#define PAGE_ATTRITUTE_RANDOM 1
#define PAGE_ATTRITUTE_STRIDE_L1 2
#define PAGE_ATTRITUTE_STRIDE_L2 3
#define PAGE_ATTRITUTE_STRIDE_L1_NOT 4
#define PAGE_ATTRITUTE_STRIDE_L2_NOT 5

unsigned long page_classify[20] = {0};
unsigned long call_classify_page_num = 0;
unsigned long st = 0, ed = 0;
unsigned long all_time = 0;


int classify_page( struct evict_transfer_entry_struct wait_tmp){
	int ret = 0;

	if(call_classify_page_num == 0) st = get_cycles();
	st = get_cycles();
	unsigned long value = wait_tmp.value;
	call_classify_page_num ++;
	if(call_classify_page_num % 500000 == 0){
//		ed = get_cycles();
//		printf("call_classify_page_num = %lu, value = 0x%lx, average_cycle = %.4lf\n", call_classify_page_num, wait_tmp.value, (double)(ed - st)/500000.0  );
		printf("call_classify_page_num = %lu, value = 0x%lx, average_cycle = %.4lf\n", call_classify_page_num, wait_tmp.value, (double)(all_time)/500000.0  );
		all_time = 0;
//		st = get_cycles();

	}
	//check 
	int ladder[2][64] = {0};
	int len_01[2] = {0};
	value = value & ( ((1ULL << (64 - RESERVED_BITS)) ) - 1);
	int index = 0;
	int last_bit = value & 1;
	int now_bit = 0;
	int number_1[2] = {0};
	len_01[last_bit] ++;
	number_1[last_bit] ++;
	index ++; // start scan bit 1
	value = value >> 1;
	int max_ladder_len = 0;
	while(value != 0){
		now_bit = value & 1;
		number_1[now_bit] ++;
		if(now_bit == last_bit){
			len_01[last_bit] ++;
		}
		else{
			if(max_ladder_len < len_01[!now_bit] && last_bit == 1)
				max_ladder_len = len_01[!now_bit];
			//not equal
			ladder[ !now_bit  ][ len_01[!now_bit] ] ++;	// update corresponding N-ladder's number
			len_01[now_bit] ++;
			len_01[!now_bit] = 0;
			last_bit = now_bit;
		}
		value = value >> 1;
		index ++;
	}
	if( len_01[1] < (1ULL << RESERVED_BITS) - 1){
		if(index == 64 - RESERVED_BITS ){
			//meet last bits
			if(last_bit == 1){
				len_01[last_bit] += ( RESERVED_BITS );
				if(max_ladder_len < len_01[last_bit] )
					max_ladder_len = len_01[last_bit];

				ladder[last_bit][ len_01[last_bit] ] ++;
			}
		}
		else{
			ladder[1][ (1ULL << RESERVED_BITS) - 1 - len_01[1] ] ++;
			if(max_ladder_len < (1ULL << RESERVED_BITS) - 1 - len_01[1] )
				max_ladder_len = (1ULL << RESERVED_BITS) - 1 - len_01[1];
		}
	}
	if( max_ladder_len >= 7 ){
		page_classify[ PAGE_ATTRITUTE_SCAN ] ++;
		ret = PAGE_ATTRITUTE_SCAN;
	}
	else if( ladder[1][1] >= 7){
		if(ladder[0][1] >= 6 || ladder[0][2] >= 6 || ladder[0][3] >= 6 ){
			//stride page
			page_classify[ PAGE_ATTRITUTE_STRIDE_L1 ] ++;	
			ret = PAGE_ATTRITUTE_STRIDE_L1;
		}
		else{
			page_classify[ PAGE_ATTRITUTE_STRIDE_L1_NOT ] ++;	
			ret = PAGE_ATTRITUTE_STRIDE_L1_NOT;
		}
	}
	else if( ladder[1][2] >= 4 ){
		if(ladder[0][1] >= 3 || ladder[0][2] >= 3 || ladder[0][3] >= 3 ){
			// stride page
			page_classify[ PAGE_ATTRITUTE_STRIDE_L2 ] ++;
			ret = PAGE_ATTRITUTE_STRIDE_L2;
		}
		else {
			page_classify[ PAGE_ATTRITUTE_STRIDE_L2_NOT ] ++;
			ret = PAGE_ATTRITUTE_STRIDE_L2_NOT;
		}
	}
	else{
			page_classify[ PAGE_ATTRITUTE_RANDOM ] ++;
			ret = PAGE_ATTRITUTE_RANDOM;
	}
	ed = get_cycles();
	all_time += (ed - st);
	return ret;

}

struct evict_transfer_entry_struct tmp_evict_tes;

void async_evict_seek(){
	int i, j;
	unsigned long st,ed;
	printf("$$$ init async_seek\n");
	int ret = -1;
	while(1){


#ifdef USING_BITMAP
		if(eb_r_ptr < eb_w_ptr){
                        unsigned long tmp_tb_len = eb_w_ptr - eb_r_ptr;
                        if(tmp_tb_len > max_evicting_len){
                                max_evicting_len = tmp_tb_len;
                        }
			unsigned long tmpi;


			for(tmpi = 0;tmpi < tmp_tb_len; tmpi ++){
				tmp_evict_tes = evict_buffer_train[ eb_r_ptr % EVICT_BUFFER_SIZE ]; 

#if 1			
				//check the pid_first
				unsigned long value_s , vpn_s;
	                        int pid_s;
				if(tmp_evict_tes.ppn >= MAX_PPN)
					continue;
			
				if(tmp_evict_tes.ppn >= (32ULL << 18))
					continue;
//				printf(" ppn = 0x%lx\n" , tmp_evict_tes.ppn);
//	                        value_s =  ppn2rpt[ tmp_evict_tes.ppn ];
	                        value_s =  pgtable_transfer( tmp_evict_tes.ppn );//ppn2rpt[ tmp_evict_tes.ppn ];
	                        vpn_s = ( value_s  >> 16) & 0xffffffffff;
	                        pid_s =  value_s & 0xffff;
				if(pid_s != now_pid && pid_s != 0)
#endif
					classify_page(tmp_evict_tes);
	                        eb_r_ptr ++;
			}
                }
#endif


#ifdef USING_FIFO
		if(eb_r_ptr < eb_w_ptr){
			unsigned long tmp_tb_len = eb_w_ptr - eb_r_ptr;
			if(tmp_tb_len > max_evicting_len){
                                max_evicting_len = tmp_tb_len;
                        }
			eb_r_ptr  = eb_w_ptr;
		}
#endif
		
#ifndef USING_BITMAP
#ifndef USING_FIFO
		if(eb_r_ptr < eb_w_ptr)
		{
			unsigned long tmp_tb_len = eb_w_ptr - eb_r_ptr;
			if(tmp_tb_len > max_evicting_len){
				max_evicting_len = tmp_tb_len;
			}
			if(tmp_tb_len > 4000){
				//maybe print some msg
				;
			}
			for(i = 0; i < tmp_tb_len; i++)
			{

			st = get_cycles();
			unsigned long tmp_ppn = evict_buffer_train[ (eb_r_ptr + i ) % EVICT_BUFFER_SIZE ];
//			eb_r_ptr ++;
			// update lru
#ifdef USING_LRU
			ret = hot_page_lru_control_return(tmp_ppn);
#endif
#ifdef USING_RRIP
			ret = hot_page_rrip_control_return(tmp_ppn);
#endif
			ed = get_cycles();
			tmp_rdtsc[0][0] += (ed - st);
			tmp_rdtsc[0][1] ++;

			if(ret == -1){
				record_insert_ret[10] ++;
			}
			else if(ret == 2){
				// its new page
				page_num_in_list ++;
				record_insert_ret[ret] ++;
			}
			else record_insert_ret[ret] ++;

			}
			eb_r_ptr += ( tmp_tb_len );

			st = get_cycles();
			local_mem_pressure_check();
 
			//check state	
			ed = get_cycles();
			tmp_rdtsc[1][0] += (ed - st);
                        tmp_rdtsc[1][1] ++;
		}
#endif
#endif
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
#ifdef USING_RRIP
	init_rrip_structure();
#endif


}


void print_async_msg(){
	int i = 0;
	for(i = 0; i < 2; i++){
		if(tmp_rdtsc[i][1] != 0)
			printf("id = %d, average consume = %.2lf cycle\n",i , (double)tmp_rdtsc[i][0] / (double)tmp_rdtsc[i][1] );
		else printf("id = %d, no use\n", i);
//	printf("hot_page_lru_control average consume %lu, check_state average consume = %lu\n",tmp_rdtsc[0][0]/tmp_rdtsc[0][1],	tmp_rdtsc[1][0]/ tmp_rdtsc[1][1] );
	}
	printf("store_to_eb's max_len = %lu, w_ptr = %lu, r_ptr = %lu, max_evictor's queue len = %lu\n", max_evicting_len, eb_w_ptr, eb_r_ptr, max_evictor_len);
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
	for(i = 0; i < 6; i++){
		if(page_classify[i] != 0){
			printf("## page identify, i = %d, number = %lu\n", i, page_classify[i]);
		}
	}
#ifdef USING_RRIP
	printf_rrip_state();
#endif
	

}

