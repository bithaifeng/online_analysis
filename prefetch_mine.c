#include "prefetch_mine.h"
#include "config.h"
#include "lt_profile.h"

map<unsigned long, unsigned char> ppn2train_buffer;


unsigned long hot_physical_number = 0;

struct LSD_vpn table_lsd[LSDSTREAM_SIZE] = {0};
map<unsigned long, int> ppn2LSD_vpn; 


LRUCache *lruCache = NULL;

void init_lruCache(int size){
	lruCache = new LRUCache(LruCacheSize);
}

void filter_lru(unsigned long p_addr, unsigned long tt){
	unsigned long ppn = p_addr >> 12;
	struct value_struct tmp_value = lruCache->get(ppn);
//	struct value_struct tmp_value = lruCache->set(ppn);
	if(tmp_value.num == 0 && tmp_value.timer == 0)
	{
		//new entry
//		tmp_value.timer = tt;
//		tmp_value.num = 1;
//		lruCache -> set(ppn,tmp_value);
	}
	tmp_value.num ++;
	if(tmp_value.num >= 8 ){
		if( tt - tmp_value.timer >= (1ULL << 22)  ){
			//send to prefetch or to train
			if(ppn2pid[ppn] != 0&& ppn2pid[ppn] != now_pid)
                                store_to_tb(ppn, tt);

		}
		tmp_value.num = 0;
		tmp_value.timer = tt;
	}
	lruCache -> set(ppn,tmp_value);

}




unsigned int bit_map[MAX_PPN] = {0};
unsigned char test_and_set_bit(unsigned long p_addr){
	int offset = (p_addr % 4096) / 64;
	int ppn = p_addr >> 12;
	unsigned int mask = 1 << offset;
	unsigned char ret = 0;
	ret = (bit_map[ppn]  >> offset) & 1;
	bit_map[ppn] = bit_map[ppn] | mask;
	return ret;
}

unsigned char get_ppn_times(unsigned int a)
{
	unsigned  char count = 0;
	while (a)
	{
		//a = a & (a-1);
		a = a >> 1;
		count++;
	}
 
	return count;
}


unsigned char checkPaddr(unsigned long p_addr, unsigned long tt){

	//set 1 to target PPN'bit
	if(1 == test_and_set_bit(p_addr) ){
		//skip
	}
	else{
		//fisrt meet
		//check whether this page should copy to training buffer
	}
	return 1;
}


int insert_entry(unsigned long ppn, unsigned long time);


#define MAX_HOT_NUM 32


unsigned long lt_printer = 0;
int filter_check(unsigned long p_addr, unsigned long tt){
//	insert_entry(p_addr >> 12, tt);	

	int ret = -1;
	unsigned long ppn = p_addr >> 12;
//	if(ppn2train_buffer.count(ppn) == 0){// also can use bloom filter
	new_ppn[ppn].num ++;

//	if( (new_ppn[ppn].num % 33 ) == 32){  //equals new_ppn[ppn].num >= 8
	if( (new_ppn[ppn].num  ) ==  MAX_HOT_NUM){  //equals new_ppn[ppn].num >= 8
//	if( (new_ppn[ppn].num % 2 ) == 1){  //equals new_ppn[ppn].num >= 8
		new_ppn[ppn].num = 0;
//		ppn2train_buffer[ppn] = 1;
#ifndef USETIMER
		store_to_tb(ppn, tt);
#endif
#ifdef USETIMER
		if(duration_all - new_ppn[ppn].timer >= (1ULL << 20)){ //2GHZ ~ 1 seconed
//		if(duration_all - new_ppn[ppn].timer >= (1ULL << 10)){ //2GHZ ~ 1 seconed // fot evict update
//		if(duration_all - new_ppn[ppn].timer >= (1ULL << 31)){ //2GHZ ~ 1 seconed
		// if(duration_all - new_ppn[ppn].timer >= ( 5000 )){ //2GHZ ~ 1 seconed
//		if(duration_all - new_ppn[ppn].timer >= (1ULL << 27)){ //2GHZ ~ 1 seconed
			new_ppn[ppn].timer = duration_all;
			unsigned long value_d = ppn2rpt[ ppn ];
	                unsigned long vpn_d = 0;
	                int pid_d = 0;
	                vpn_d = ( value_d  >> 16) & 0xffffffffff;
	                pid_d = value_d & 0xffff;
		hot_physical_number ++;
            
             	if(pid_d == 0 || pid_d == now_pid)
                 return 0;

            // if(ppn < (64*(1UL << 18)))

			    // statics_datatype(page_state_array[ppn].state);


                // if(page_state_array[ppn].state == lt_hmtt_page_unused && (lt_printer++)%10000 == 1){
                //     printf("lt: get pid=%d use a unuse page\n", pid_d);
                // }
#ifndef USING_FASTSWAP
		        if( lt_check_page_charging(ppn) )
#endif
			{
				store_to_pb(ppn, 1900);
//				store_to_tb(ppn, tt);
//				store_to_eb(ppn, tt);				
				;
			}
		}
		else{
			new_ppn[ppn].timer = duration_all;
		}
#endif
	}
//	else if( (new_ppn[ppn].num  ) >  MAX_HOT_NUM ){
//		new_ppn[ppn].num --;
//	}
/*
	}
	else{
		//hit , no operation
	}
*/
	return ret;
}


void sortLSDPage(int inx){
	for (int i = 0; i < table_lsd[inx].size; i++) {
        for (int j = i; j < table_lsd[inx].size; j++) {
            if (table_lsd[inx].vpn[i] > table_lsd[inx].vpn[j]) {
                unsigned long tmp = table_lsd[inx].vpn[i];
                table_lsd[inx].vpn[i] = table_lsd[inx].vpn[j];
                table_lsd[inx].vpn[j] = tmp;

#ifdef LSDUSE_PPN
		unsigned long tmp_ppn = table_lsd[inx].ppn[i];
		table_lsd[inx].ppn[i] = table_lsd[inx].ppn[j];

                table_lsd[inx].ppn[j] = tmp_ppn;
#endif
            }
        }
    }
}


int print_t = 1;

//int default_inter_page = 1900; //hpl
int default_inter_page = 300;
int last_prefetch_buffer = 0;
int last_average_len = 0;
float hmtt_detect_range = 0.2;
float hmtt_change_range = 0.2;
float rdma_detect_range = 0.5;
float rdma_change_range = 0.2;

int last_prefetch_step = default_inter_page;
int now_prefetch_step = default_inter_page;

int max_inter_page = 2000;
int print_timess = 50;


//int INTER = 256;
int INTER = 64;
//int INTER = 99999999999;

int caculate_stride(int inx){
//	return 1;
	int tmp_strid[8] = {0};
	
	map<unsigned long, int > stride2num;
	int real_stride = 0;
	for(int i = 0 ; i < 7 ;i ++){
		tmp_strid[i] = table_lsd[inx].vpn[i + 1] - table_lsd[inx].vpn[i];
		if(stride2num.count(tmp_strid[i]) == 0 )
			stride2num[ tmp_strid[i]  ]  = 1;
		else
			stride2num[ tmp_strid[i]  ]  ++ ;
		if(stride2num[ tmp_strid[i]  ] >= 4)
			real_stride = tmp_strid[i];
	}
	return real_stride;
	return 0;
}

int all_stride[1024] = {0};
int all_stride_sum[1024] = {0};
int not_find_stride = 0;


int get_stride(int inx){
	//if stride < 0, transfer the stride to stride_origin + 512;
	// -256 < stride < 256
	int i =0;
	int stride_arr[512] = {0};
	int tmp_stride = 0;
	int max_num = 0;
	int max_num_stride = 0;
	for(i = 0 ; i < LONGSTRIDE_STRIDEN - 1; i++){
		tmp_stride = table_lsd[inx].stride_array[ i ];
		if(tmp_stride < 0)
			tmp_stride += (768);
		stride_arr[tmp_stride] ++;
		if(max_num < stride_arr[tmp_stride]){
			max_num = stride_arr[tmp_stride];
			max_num_stride = tmp_stride;
		}
	}
	all_stride[max_num_stride] ++;
	if(max_num_stride > 256)
		max_num_stride -= 768;
	return max_num_stride;
}

int stride_sum = 0;


int get_dominate_stride( int inx, int new_stride ){
	int i , j;
	int record_score_stride[128] = {0};
	int max_ret_real = 0;
	int max_record_score = -1;
	int ret = 0;
	int tmp_ret = 0;
	for(i = 0; i < LONGSTRIDE_STRIDEN - 1; i++){
		tmp_ret = table_lsd[inx].stride_array[i];
		if( table_lsd[inx].stride_array[i] < 0){
			tmp_ret += 128;
		}
		record_score_stride[tmp_ret] ++;
		if( record_score_stride[tmp_ret] > max_record_score){
			max_record_score = record_score_stride[tmp_ret];
			max_ret_real = tmp_ret;
			ret = tmp_ret;
		}
	}
	if(ret > 64)
		ret -= 128;
	if( record_score_stride[tmp_ret] >= (LONGSTRIDE_STRIDEN - 1) / 2 ){
		return ret;
	}
	else{
		not_find_stride  = not_find_stride + 1;
		return 0;
	}
}

//#define scan_window_size 2
int get_stride_fast(int inx, int new_stride){
	int ret = 0;
	int last_stride = table_lsd[inx].stride_array[ LONGSTRIDE_STRIDEN - 2 ];
	int i = 0;
	int j = 0;
	int record_score_stride[128] = {0};
	int max_record_score = -1;
	int last_score_index = -1;
	int max_ret_real = 0;

        int max_score_index = -1, max_score_last_index = -1;
	

#ifdef scan_window_size
//	for(i = Stride_Array_len - (scan_window_size) ; i >= 0; i-- ){
	for(i = Stride_Array_len - (2 * scan_window_size - 1) ; i >= 0; i-- ){
		//check scan windows
		int find_pattern = 1;
		for(j = 0 ; j < scan_window_size; j ++){
			if(j == scan_window_size - 1){
				if( table_lsd[inx].stride_array[i + j] != new_stride){
					find_pattern = -1;
					break;
				}
				else{
					//find next stride
					ret = table_lsd[inx].stride_array[i + j + 1];
				}

			}
			if( table_lsd[inx].stride_array[i + j] != table_lsd[inx].stride_array[ Stride_Array_len - scan_window_size + i + 1] ){
				find_pattern = -11;
				break;
			}
		}
		if(find_pattern == 1){
			stride_sum = 0;
			for(j = i; j < Stride_Array_len - scan_window_size; j++)
                        {
                                stride_sum += table_lsd[inx].stride_array[j];
                        }			
		}

	}
#endif

#ifndef scan_window_size
	for(i = LONGSTRIDE_STRIDEN - 3 ; i >= 0; i --){
		if( table_lsd[inx].stride_array[i] == last_stride && table_lsd[inx].stride_array[i + 1] == new_stride )
		{
			//find stride
			if(i != LONGSTRIDE_STRIDEN - 3)
				ret = table_lsd[inx].stride_array[i + 2];
			else
				ret = table_lsd[inx].stride_array[i + 1];
			int tmp_ret = ret;
			if(ret < 0)
				tmp_ret += 128;
			record_score_stride[tmp_ret] ++;
			if( record_score_stride[tmp_ret] > max_record_score ){
				max_score_index = i;	
				max_ret_real = tmp_ret;
				max_score_last_index = last_score_index;
				max_record_score = record_score_stride[tmp_ret];
			}
			last_score_index = i;
//			if(  )

			//find stride_sum
//			stride_sum = table_lsd[inx].stride_array[i] + table_lsd[inx].stride_array[i + 1];

#if 0 
			stride_sum = 0;
			for(j = i; j < LONGSTRIDE_STRIDEN - 2; j++)
			{
				stride_sum += table_lsd[inx].stride_array[j];
			}

			break;
#endif
		}
	}
	if( max_record_score != -1 ){
		//find and caculate stride sum
		stride_sum = 0;
		if( max_score_last_index == -1 ){
			for(j = max_score_index ; j < LONGSTRIDE_STRIDEN - 2; j ++){
				stride_sum += table_lsd[inx].stride_array[j];
			}
		}
		else{
			for( j = max_score_index ; j < max_score_last_index ; j++ ){
				stride_sum += table_lsd[inx].stride_array[j];
			}
		}
	}
	if(max_ret_real > 64 ){
		ret = max_ret_real - 128;
	}

#endif
	if(ret == 0){
		not_find_stride  = not_find_stride + 1;
		if(not_find_stride % 5000 == 0){
			printf("not_find_stride = %d, pid = %d {", not_find_stride, table_lsd[inx].pid);
			for(i = 0 ; i < LONGSTRIDE_STRIDEN - 1; i++)
				printf( "%d,", table_lsd[inx].stride_array[i] );
			printf("%d }\n", new_stride);
//			printf();
		}

	}
	all_stride[ret] ++;
	all_stride_sum[ stride_sum ] ++;
	return ret;
}

int index_to_stride[64][128] = {0};
int index_to_stride_len[64] = {0};

void record_stride(int inx, int new_stride){
	index_to_stride_len[inx] ++;
	if( new_stride < 0 ){
		if( new_stride > -64 ){
			index_to_stride[ inx ][new_stride + 128] ++;
		}
		else index_to_stride[inx][127] ++;
	}
	else{
		if(new_stride < 64){
			index_to_stride[ inx ][new_stride] ++;
		}
		else
			index_to_stride[inx][64] ++;
	}
}

void clear_record_stride( int inx){
	int i = 0;
	for(i = 0; i < 128; i++){
		index_to_stride[ inx ][i] = 0;
	}
	index_to_stride_len[inx] = 0;
}

void print_record_stride(){
	int i ,j;
	for(i = 0; i < 64;i ++){
		if( index_to_stride_len[i] > 4000 ){
			printf("id = %d, len = %d { ", i, index_to_stride_len[i]);
			for(j = 0; j < 128; j++){
				if( index_to_stride[i][j] > 1000 )
				printf("[ %d, %d], ", j, index_to_stride[i][j] );
			}
			printf("}\n");
		}
	}

}



//#define PRINT_LSD_TIME
#define LSD_TIME 50000
unsigned long lsd_time[10] = {0};
unsigned long lsd_use_count[10] = {0};
unsigned long lsd_sum_time[10] = {0};


static inline __u64 get_cycles(void)
{
        __u32 timehi, timelo;
        asm("rdtsc":"=a"(timelo),"=d"(timehi):);
        return (__u64)(((__u64)timehi)<<32 | (__u64)timelo);
}



void InsertNewLSD_vpn(unsigned long ppn, unsigned long vpn, unsigned long now_time, int real_pid){
//int InsertNewLSD_vpn(unsigned long ppn, unsigned long vpn, unsigned long now_time, int real_pid){
//	max_inter_page = using_inter_page;
//	default_inter_page = using_inter_page;	


	int i;
	int inx = -1;
	unsigned long minStride = 1ULL << 30;
	int evict_inx = -1;
	int zero_inx = -1;

#ifdef PRINT_LSD_TIME
	if( lsd_use_count[1] % LSD_TIME == ( LSD_TIME - 1) ){
		printf("{*Insert train*, part1 = %lu, part2 = %lu, part3 = %lu}\n", lsd_sum_time[1] / (LSD_TIME - 1), lsd_sum_time[2] / lsd_use_count[2] ,  lsd_sum_time[3] / lsd_use_count[3] );
		printf("**** part2 check vpns = %lu, compute strirde = %lu, evict oldest = %lu\n", lsd_sum_time[4]/ lsd_use_count[4], lsd_sum_time[5] / lsd_use_count[5] , lsd_sum_time[6] / lsd_use_count[6]);
			
		lsd_use_count[2] = 0;
		lsd_use_count[3] = 0;
		lsd_sum_time[2] = 0;
		lsd_sum_time[1] = 0;
		lsd_sum_time[3] = 0;
	}
#endif



#ifdef PRINT_LSD_TIME
	lsd_time[0] = get_cycles();
#endif
	//  if(print_timess > 0){
    //              printf("find old id = %d, pid = %d, vpn = %lu\n", inx, real_pid, vpn);
    //              print_timess --;
    //     }


	unsigned long last_time = now_time;
	for(i = 0; i < LSDSTREAM_SIZE; i ++){
		if (table_lsd[i].current_time < last_time) {
			last_time = table_lsd[i].current_time;
			evict_inx = i;
		}
		if (table_lsd[i].valid > 0 && real_pid == table_lsd[i].pid){
/*
			if (table_lsd[i].size == 0){
				if(zero_inx != -1){
					zero_inx = i;
				}
				continue;
			}
*/
//			if(table_lsd[i].vpn2value.count(vpn) != 0)
//				return ;//continue;

			unsigned long tmp = delta(vpn, table_lsd[i].vpn[table_lsd[i].size - 1]);
//                        unsigned long tmp1 = delta(vpn, table_lsd[i].vpn[ 0 ]  );
//                        if(tmp < minStride && (tmp < INTER || tmp1 < INTER )){
            if(tmp < minStride && (tmp < INTER )){
				inx = i;
				minStride = tmp;
				// if(print_timess > 0){
                //     printf("find old id = %d, pid = %d, vpn = %lu\n", inx, real_pid, vpn);
                //     print_timess --;
                // }

				break;
			}
		}
	}

#ifdef PRINT_LSD_TIME
	lsd_time[1] = get_cycles();
	lsd_sum_time[1] += ( lsd_time[1] - lsd_time[0] );
	lsd_use_count[1] ++;

#endif
	if (inx >= 0) {
		//insert 

		for(i = 0; i < table_lsd[inx].size;i ++){
			if(table_lsd[inx].vpn[i] == vpn)
				return 0;
		}
		
#ifdef PRINT_LSD_TIME
        lsd_time[4] = get_cycles();
        lsd_sum_time[4] += ( lsd_time[4] - lsd_time[1] );
        lsd_use_count[4] ++;

#endif

		if (table_lsd[inx].size >= STRIDEN) {
			if(print_timess > 0){
				printf("insert 8th id = %d, pid = %d, vpn = %lu\n", inx, real_pid, vpn);
				print_timess --;
			}
//			ppn2train_buffer.erase( table_lsd[inx].ppn[0] );
//			table_lsd[i].vpn2value.erase( table_lsd[inx].vpn[0] );
			// prefetch 
			int stride = (int)(table_lsd[inx].vpn[table_lsd[inx].size - 1] - table_lsd[inx].vpn[ 0 ]) / 7 ; 
//			stride = caculate_stride(inx);
//			stride = get_stride(inx);
//			stride = get_stride_fast(inx, vpn - table_lsd[inx].vpn[table_lsd[inx].size - 1] );
			stride = get_dominate_stride(inx, vpn - table_lsd[inx].vpn[table_lsd[inx].size - 1] );
			record_stride(inx, vpn - table_lsd[inx].vpn[table_lsd[inx].size - 1]);
//			stride = 1;
//			if(stride >= 100 || stride <= -100)
//				stride = 0;

#if 0
			if(stride == 0){
				if(table_lsd[inx].vpn[ 0 ] < table_lsd[inx].vpn[table_lsd[inx].size - 1]) 
					stride += 1;
				else stride -= 1;
			}
			if(stride < -100 || stride > 100){
				if(table_lsd[inx].vpn[ 0 ] < table_lsd[inx].vpn[table_lsd[inx].size - 1]) 
                                        stride = 1;
                                else stride = -1;
			}

#endif 
			if( table_lsd[inx].stride == 0)  
				table_lsd[inx].stride = stride;
//			table_lsd[inx].stride =   caculate_stride(inx);

/*			if( delta(stride , table_lsd[inx].stride) >= 5){
				table_lsd[inx].stride = stride;
			}
*/

#ifdef USE_TIMER
			unsigned long inter_time = (table_lsd[inx].timer[table_lsd[inx].size - 1] - table_lsd[inx].timer[ 0 ] ) / 7 ;
#endif




			if( print_t % 30000 == 9){
				

#ifdef USE_TIMER
//				printf("pid = %d, inter_times = %lu cycles  \n",  table_lsd[inx].pid, inter_time  );	
#endif
//				printf("t0 = %lu \n",  table_lsd[inx].timer[0]);
//				printf("t3 = %lu \n",  table_lsd[inx].timer[3]);
//				printf("t5 = %lu \n",  table_lsd[inx].timer[5]);
//				printf("t7 = %lu \n",  table_lsd[inx].timer[7]);
#ifdef USE_TIMER
//				printf("stride = %lu,prefetch_len = %ld\n",  stride , PREFETCH_TIME / inter_time);
#endif
//				printf("prefetch thread len = %d \n ",prefetcher_task_set[ 0 ].ptr_w - prefetcher_task_set[ 0 ].ptr_r );
//				printf("train buffer  len = %d \n ",  tb_w_ptr - tb_r_ptr );
			}
			if(print_t % 10000 == 1000){//  && false){
				if(stride == 0){
//					if(table_lsd[inx].vpn[ 0 ] < table_lsd[inx].vpn[table_lsd[inx].size - 1]) 
						stride += 1;
//					else stride -= 1;
				}
				if( table_lsd[inx].stride == 0)  
					table_lsd[inx].stride = stride;
				if( delta(stride , table_lsd[inx].stride) >= 5){
					table_lsd[inx].stride = stride;
				}

				//update date prefetch len 
				now_prefetch_step = last_prefetch_step;
				unsigned long ac_counter_inter = 0, latency_all_inter = 0;
				if(last_latency_all == 0){
					latency_all_inter = latency_all;
					ac_counter_inter = ac_counter;
				}
				else{
					latency_all_inter = latency_all - last_latency_all;
					ac_counter_inter = ac_counter - last_ac_counter;
				}
				if(ac_counter_inter != 0){
					//tune prefetch step
					double tmp_latency_average = (double) latency_all_inter / (double)ac_counter_inter;
					if(tmp_latency_average >= 20000000 && tmp_latency_average <= 40000000){
					}
					else if( tmp_latency_average < 20000000 ){
						now_prefetch_step = now_prefetch_step * 1.1;
					}
					else if( tmp_latency_average > 40000000 ){
						now_prefetch_step = now_prefetch_step * 0.9;

					}
					if(now_prefetch_step < default_inter_page){
                                 	       now_prefetch_step = default_inter_page;
	                                }
        	                        if(now_prefetch_step >= max_inter_page){
                	                        now_prefetch_step = max_inter_page;
                        	        }
//					printf("tmp_latency_average=%.3lf\n",tmp_latency_average);
					

				}
#if 0  //use hmtt buffer and prefetch buffer to tune prefetch_step
				int now_prefetch_buffer = (( prefetcher_task_set[ 0 ].ptr_w - prefetcher_task_set[ 0 ].ptr_r )  + prefetcher_task_set[ 1 ].ptr_w - prefetcher_task_set[ 1 ].ptr_r) / 2  ;
				if(now_prefetch_buffer == 0)
					now_prefetch_buffer = 1;	

				
				int now_average_len = all_has_read;
				if(times_use != 0)
					now_average_len = all_has_read / times_use;
				if(last_average_len == 0){
					if(times_use != 0)
						last_average_len = all_has_read / times_use;	
					else
						last_average_len = now_average_len;
					printf(" last_average_len = %d\n", last_average_len);
					all_has_read = 0;
					times_use = 0;
				}
				else{
					//
//					if(now_average_len * 1.15 < last_average_len){
					if(now_average_len * (1 + hmtt_detect_range) < last_average_len){
						// decrease prefetch len
					//	now_prefetch_len = last_prefetch_len * 0.9;

//						now_prefetch_len = last_prefetch_len * (1 - hmtt_change_range);
					}
					else if(now_average_len > last_average_len * (1 + hmtt_detect_range)){
						// increase prefetch len
//						now_prefetch_len = last_prefetch_len * 1.1;

//						now_prefetch_len = last_prefetch_len * (1 + hmtt_change_range);
					}
					last_average_len = now_average_len;
					all_has_read = 0;
                                        times_use = 0;					
				}
				if(last_prefetch_buffer == 0){
					last_prefetch_buffer = now_prefetch_buffer;
				}
				else{
/*
					if( now_prefetch_buffer  * (1 + rdma_detect_range) < last_prefetch_buffer ){
						// decrease prefetch len
//						now_prefetch_len = last_prefetch_len * 0.9;
						now_prefetch_len = last_prefetch_len * (1 - rdma_change_range);
					}
					else if (  now_prefetch_buffer > last_prefetch_buffer * (1 + rdma_detect_range)){
						// increase prefetch len
//						now_prefetch_len = last_prefetch_len * 1.1;
						now_prefetch_len = last_prefetch_len * (1 + rdma_change_range);
					}
*/
					if(delta( now_prefetch_buffer, last_prefetch_buffer ) >= 500 ){
						if(now_prefetch_buffer > last_prefetch_buffer){
							//increase prefetch len
							now_prefetch_step = last_prefetch_step * (1 + rdma_change_range);
						}
						else{
							now_prefetch_step = last_prefetch_step * (1 - rdma_change_range);
						}
					}
					last_prefetch_buffer = now_prefetch_buffer;
					
				}
				if(now_prefetch_step < default_inter_page){
					now_prefetch_step = default_inter_page;
				}
				if(now_prefetch_step >= max_inter_page){
					now_prefetch_step = max_inter_page;
				}
#endif
				
//				printf("table_lsd[inx].stride = %d\n", table_lsd[inx].stride);
//				printf("len change from %d to %d\n", last_prefetch_len ,now_prefetch_len);
//				printf("now_average_len = %d, now_prefetch_buffer = %d\n  ",  now_average_len, now_prefetch_buffer);
			}
			print_t ++;
#ifdef PREFETCH_ON
				// stride test
//				if(stride != 0)
//				store_to_pb(ppn, now_prefetch_step * stride_sum + stride); // useful
				if(stride == 0) stride = 1;
				store_to_pb(ppn, now_prefetch_step * stride); // for stream-based
#endif

#ifdef EVICT_ON
				
#ifdef use_stream_evict
            // if( memory_buffer_start_addr[2] < MIN_PAGE_NUM*2)
				copy_ppn_to_evict_buffer(vpn - 1000 , real_pid ,  1, 0);
//				copy_ppn_to_evict_stream_buffer(vpn - 1000 , real_pid ,  1, 0);
#endif

#endif



//				store_to_pb(ppn, now_prefetch_step * 5); // useful
			if(now_prefetch_step <= last_prefetch_step){
//				store_to_pb(ppn, now_prefetch_step * table_lsd[inx].stride);
//				store_to_pb(ppn, now_prefetch_step * stride); // useful

//				store_to_pb(ppn, now_prefetch_step * 1);
			}
			else{
				for(int len = last_prefetch_step; len <= now_prefetch_step; len ++){
//					store_to_pb(ppn, len * table_lsd[inx].stride);
//					store_to_pb(ppn, len * stride);  //useful
//					store_to_pb(ppn, len * 1);
				}
			}
			last_prefetch_step = now_prefetch_step;

#ifdef PRINT_LSD_TIME
		        lsd_time[5] = get_cycles();
	        	lsd_sum_time[5] += ( lsd_time[5] - lsd_time[4] );
		        lsd_use_count[5] ++;
#endif



			for (int i = 0; i < table_lsd[inx].size - 1; i++) {
#ifdef LSDUSE_PPN
		                table_lsd[inx].ppn[i] = table_lsd[inx].ppn[i+1];
#endif
				table_lsd[inx].vpn[i] = table_lsd[inx].vpn[i+1];

#ifdef USE_TIMER
				table_lsd[inx].timer[i] = table_lsd[inx].timer[i + 1];
#endif

#ifdef USE_STRIDE
				table_lsd[inx].stride_array[i] = table_lsd[inx].stride_array[i + 1];
#endif

            		}
		        table_lsd[inx].size--;
#ifdef PRINT_LSD_TIME
                        lsd_time[6] = get_cycles();
                        lsd_sum_time[6] += ( lsd_time[6] - lsd_time[5] );
                        lsd_use_count[6] ++;
#endif



		}
#ifdef LSDUSE_PPN
		table_lsd[inx].ppn[ table_lsd[inx].size ] = ppn;
#endif
		table_lsd[inx].vpn[ table_lsd[inx].size ] = vpn;
//		table_lsd[inx].vpn2value[vpn] = 1;

#ifdef USE_STRIDE
		if( table_lsd[inx].size > 0 ){
			int tmp_stride = table_lsd[inx].vpn[ table_lsd[inx].size ] - table_lsd[inx].vpn[ table_lsd[inx].size - 1];
			if(tmp_stride <= 256 && tmp_stride > -256){
				;
			}
			else tmp_stride = 0;	
			table_lsd[inx].stride_array[ table_lsd[inx].size - 1 ] = tmp_stride;
		}
#endif

#ifdef USE_TIMER
		table_lsd[inx].timer[ table_lsd[inx].size ] = now_time;
#endif

		table_lsd[inx].size ++;
		table_lsd[inx].current_time = now_time;
//		sortLSDPage(inx);
		 if(table_lsd[inx].vpn[ 0 ]   < table_lsd[inx].start_vpn)
                        table_lsd[inx].start_vpn =  table_lsd[inx].vpn[ 0 ];
#ifdef PRINT_LSD_TIME
        lsd_time[2] = get_cycles();
        lsd_sum_time[2] += ( lsd_time[2] - lsd_time[1] );
        lsd_use_count[2] ++;
#endif


	}
	else{
		//new 
/*
		for (int i = 0; i < LSDSTREAM_SIZE; i++) {
	        	if (table_lsd[i].size == 0) {
		                inx = i;
                		break;
			}
        	}

*/
//		if(zero_inx != 0){
//			inx = zero_inx;
//		}		
//		else inx = evict_inx;
		inx = evict_inx;
		if (inx < 0) {
            // stream evict
			inx = -1;
            		unsigned long early = now_time;
		        for (int i = 0; i < LSDSTREAM_SIZE; i++) {
                		if (table_lsd[i].current_time < early) {
                    			early = table_lsd[i].current_time;
                    			inx = i;
                		}
            		}
        	}
//		for(int i = 0 ; i < table_lsd[inx].size; i ++){
//			table_lsd[inx].vpn2value.erase( table_lsd[inx].vpn[i] );
//		}
//		printf(" insert new\n ");
		if(print_timess > 0){
                        printf("insert new id = %d, pid = %d, vpn = %lu\n", inx, real_pid, vpn);
	                print_timess --;
                }

//		clear_record_stride(inx);


		table_lsd[inx].size = 1;
		table_lsd[inx].pid = real_pid;
		table_lsd[inx].vpn[0] = vpn;
		table_lsd[inx].valid = 1;
#ifdef LSDUSE_PPN
		table_lsd[inx].ppn[0] = ppn;
#endif
		table_lsd[inx].current_time = now_time;
		table_lsd[inx].stride = 0;

#ifdef USE_TIMER
                table_lsd[inx].timer[ 0 ] = now_time;
#endif

#ifdef PRINT_LSD_TIME
        lsd_time[3] = get_cycles();
        lsd_sum_time[3] += ( lsd_time[3] - lsd_time[1] );
        lsd_use_count[3] ++;

#endif
	}
}



#define ENTRYNUM 256

struct Ppn2entry{
        unsigned long ppn;
        int num;
        unsigned long time;
        char send_bit;
};
struct Ppn2entry ppn2numentry[ENTRYNUM] = {0};

int sram_entry_num = 64;

int set_num = 1;
int way_num = 64;
int sram_num = 8;
int max_pri = 19;

int  check_id(unsigned long ppn){
//	int real_set = ppn % set_num;
	if( max_pri -- > 0 ){
		printf("ppn = %lu , 0x%lx\n", ppn, ppn);
	}

//      for(int i =0 ;i < ENTRYNUM; i++){
	for(int i =0 ;i < sram_entry_num; i++){
//        for(int i = set_num * way_num ;i < set_num * way_num +  way_num ; i++){
//        for(int i = set_num * way_num ;i < set_num * way_num +  way_num ; i++){
                if( ppn2numentry[i].ppn == ppn  ){
                        return i;
                }
        }
        int ret = -1;
        return ret;
}

int found_oldest( unsigned long ppn ){
	
        unsigned long min_time = ppn2numentry[ 0 ].time;
        int id = 0;
	int set_num = sram_entry_num / 16;
        int real_set = 0;
        if(set_num == 1){
                real_set = 0;
        }
        else if(set_num == 2){
                real_set = ppn % 2;
        }
        else if(set_num == 4){
                real_set = ppn % 4;
        }

//      for( int i =0 ;i < ENTRYNUM; i++ ){
//      for( int i =0 ;i < sram_entry_num; i++ ){
        for( int i = real_set * way_num  ;i < real_set * way_num + way_num; i++ ){
                if( ppn2numentry[i].ppn == 0 )
                        return i;

                if( ppn2numentry[i].time < min_time ){
                        min_time = ppn2numentry[i].time;
                        id = i;
                }
        }
        return id;
}


int insert_entry(unsigned long ppn, unsigned long time){
        int index = check_id(ppn);

        if(index == -1){//not found
                index = found_oldest(ppn);
//		if(max_pri -- > 0){
//			printf("not found, index = %d\n", index );
//		}
                ppn2numentry[ index ].ppn = ppn;
                ppn2numentry[ index ].num = 1;
                ppn2numentry[ index ].time = time;
                ppn2numentry[ index ].send_bit = 0;
        }
        else{
                //update entry
		if(max_pri -- > 0){
			printf("found, index = %d\n", index );
		}
                ppn2numentry[ index ].num ++;
                ppn2numentry[ index ].time = time;
                //check num > 8?
//              if(ppn2numentry[ index ].num >= MAX_NUM){
                if(ppn2numentry[ index ].num >= 3){
                        if(ppn2numentry[ index ].send_bit == 0){
                                ppn2numentry[ index ].send_bit = 1;
//                                max_find_page_num[1] ++;
//				store_to_tb( ppn , time);
                                return 1;
                        }
                }
        }
        return 0;
}


void print_stride(){
	int i = 0;;
	printf("*********  stirde  ***********\n");
	for(i = 0; i < 1024; i ++){
		if(all_stride[i] != 0 && all_stride[i] > 10){
			printf("stride = %d, num = %d\n", i , all_stride[i]);
		}
	}
	for(i = 0; i < 1024; i ++){
		if(all_stride_sum[i] != 0 && all_stride_sum[i] > 10){
			printf("stride_sum = %d, num = %d\n", i , all_stride_sum[i]);
		}
	}
//	printf("not_find_stride = %lu. \n", not_find_stride);
	printf("not_find_stride = %d \n", not_find_stride);
	print_record_stride();

	printf("\n\n");

}







