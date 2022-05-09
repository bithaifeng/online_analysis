#include "page_table.h"
#include "prefetch_mine.h"





struct analysis_pg_task analysis_pg_task_set[ MAX_ANALYSIS_NUM ];
pthread_t analysis_tid[ MAX_ANALYSIS_NUM ];
pthread_t tid_pg_seek;



void analysis_pthread_id(void *arg){
	struct analysis_pg_task* task = (struct analysis_pg_task*)arg;
	task->real_tid = pthread_self();
	int bug_flag = 0;
	unsigned long ppn,vpn;
        printf("< pg analysis >curent tpid = %u, id = %d\n", pthread_self(), task->tid);
	while(1){
		if(task->ptr_w > task->ptr_r){
			if(task->ptr_w - task->ptr_r > ANALYSIS_BUFFER_SIZE){
				if(bug_flag == 0){
                                        bug_flag = 1;
                                        printf("overflow pgtable pthread task->tid = %d, inter = %u, ptr_w = %u, ptr_r = %u  \n",  task->tid, task->ptr_w - task->ptr_r, task->ptr_w, task->ptr_r);
				}
			}
			unsigned long tmp_value = task->value[ task->ptr_r % ANALYSIS_BUFFER_SIZE ];
			int tmp_pid = task->pid[ task->ptr_r % ANALYSIS_BUFFER_SIZE ];
			char tmp_magic = task->magic[ task->ptr_r % ANALYSIS_BUFFER_SIZE ];
			ppn = tmp_value & 0xffffff;
                        vpn = (tmp_value >> 24) & 0xffffffffff;

			if(tmp_magic == set_page_table_magic){
				ppn2pid[ppn] = tmp_pid;
                                ppn2vpn[ppn] = vpn;
                                ppn2num[ppn] = 0;
				if(using_pid == tmp_pid && vpn  >= (moniter_base_address >> 12))
				{
#ifdef PRINT_MSG
					printf("update set vpn = %llu, vaddr = %llx, ppn = 0x%lx , pid = %d, id = %d\n",  ppn2vpn[ppn], (ppn2vpn[ppn] << 12) | (record.paddr & 0xfff), ppn  ,tmp_pid,  analysis_kt_num);
#endif
				}


			}
			else if( tmp_magic == free_page_table_get_clear || tmp_magic == free_page_table_get_clear_full || tmp_magic == free_page_table_magic ){
				ppn2pid[ppn] = 0;
                                ppn2vpn[ppn] = 0;
				ppn2num[ppn] = 0;
#ifdef MONITOER_PREFETCH
                                new_ppn[ppn].isprefetch=0;
                                new_ppn[ppn].prefetch_time=0;
#endif
			}
			else{
                                printf("Unknow maigc = %x\n", tmp_magic);
                        }
			task->ptr_r ++;
		}
	}
}


extern void analysis_kt_buffer_seek();

void init_page_table_pthread(){
	for(int i = 0; i < MAX_ANALYSIS_NUM; i ++){
		analysis_pg_task_set[i].tid = 0;
		analysis_pg_task_set[i].real_tid = 0;
		analysis_pg_task_set[i].ptr_w = 0;
		analysis_pg_task_set[i].ptr_r = 0;
		analysis_pg_task_set[i].task_len = 0;
	}

	pthread_create(&tid_pg_seek, NULL, analysis_kt_buffer_seek, NULL);
//        pthread_setaffinity_np(tid_pg_seek, sizeof(mask_cpu_2), &mask_cpu_2);
        pthread_setaffinity_np(tid_pg_seek, sizeof(mask_kt), &mask_kt);
	for(int i = 0; i < MAX_ANALYSIS_NUM; i ++){
                pthread_create(&analysis_tid[i], NULL, analysis_pthread_id , (void*)(&analysis_pg_task_set[i]));
//                pthread_setaffinity_np(analysis_tid[i], sizeof(mask_cpu_2), &mask_cpu_2);
                pthread_setaffinity_np(analysis_tid[i], sizeof(mask_kt), &mask_kt);
                printf("<analysis pthread> i = %d, create\n",i);
        }
	printf("start analysis kt pthread！\n");
}


void analysis_kt_buffer_seek(){
        unsigned int seq_no,r_w;
        unsigned long paddr;
        unsigned long long timer;
        unsigned int pid;
        unsigned long ppn;
        unsigned long vpn;
        uint64_t val;
        char magic;

        //monitor kt buffer
	int current_idx = 0;
	int first_flag = 0;

        while(1){
                if(finish_kt_analysis == 0)
                {
                        printf(" analysis_kt_buffer finish , kt_hmtt_reading_addr = 0 , kt_hmtt_writing_addr = 0\n");
                        break;
                }

                if(glb_start_analysis == 0)
                {
                        continue ;
                }
                if (strncmp(kt_ch_tmp, "$$$$$$$$$$$$$", 13) == 0) {
                        return ;
                }

		if(first_flag == 0){
			int tag_seq = (int)(*(char*)(kt_ch_tmp + 13));
	                magic =  (*(char*)(kt_ch_tmp));			
			pid = (*(int*)((kt_ch_tmp + 1)));
			val = (*(uint64_t*)(kt_ch_tmp + 5));
//			ppn = val & 0xffffff;
//			vpn = (val >> 24) & 0xffffffffff;
			unsigned long now_read_id = analysis_pg_task_set[current_idx].ptr_w % MAX_ANALYSIS_NUM;

			analysis_pg_task_set[current_idx].value[ now_read_id ] = val;
			analysis_pg_task_set[current_idx].magic[ now_read_id ] = magic;
			analysis_pg_task_set[current_idx].pid[ now_read_id ] = pid;
			analysis_pg_task_set[current_idx].ptr_w ++;

			current_idx ++;
			current_idx = current_idx % MAX_ANALYSIS_NUM;
			first_flag = 1;
		}
               	//check_and dispatch
		while(*dev_readptr == *dev_writeptr)
                {
                }
		if(*dev_readptr + 14 < buffer_size){
//			if(*dev_readptr < *dev_writeptr){
//                                if(max_kt_buffer_offst < ( *dev_writeptr - *dev_readptr ))
//                                        max_kt_buffer_offst = ( *dev_writeptr - *dev_readptr );
//                        }

//                        memcpy(kt_ch_tmp, p_kernel_trace_buf + *dev_readptr , 14);
			char * ch_ptr = NULL;
			ch_ptr = p_kernel_trace_buf + *dev_readptr;
			magic =  (*(char*)(ch_ptr));
			pid = (*(int*)((ch_ptr + 1)));
			val = (*(uint64_t*)(ch_ptr + 5));
			
			unsigned long now_read_id = analysis_pg_task_set[current_idx].ptr_w % ANALYSIS_BUFFER_SIZE;
			analysis_pg_task_set[current_idx].value[ now_read_id ] = val;
                        analysis_pg_task_set[current_idx].magic[ now_read_id ] = magic;
                        analysis_pg_task_set[current_idx].pid[ now_read_id ] = pid;
                        analysis_pg_task_set[current_idx].ptr_w ++;
			current_idx ++;
                        current_idx = current_idx % MAX_ANALYSIS_NUM;

                        *dev_readptr += 14;

		}
		else{
			unsigned long left = buffer_size - *dev_readptr;
                        memcpy(kt_ch_tmp, p_kernel_trace_buf + *dev_readptr, left);
                        memcpy(kt_ch_tmp + left, p_kernel_trace_buf, 14 - left);
			
			magic =  (*(char*)(kt_ch_tmp));
                        pid = (*(int*)((kt_ch_tmp + 1)));
                        val = (*(uint64_t*)(kt_ch_tmp + 5));

			unsigned long now_read_id = analysis_pg_task_set[current_idx].ptr_w % ANALYSIS_BUFFER_SIZE;

                        analysis_pg_task_set[current_idx].value[ now_read_id ] = val;
                        analysis_pg_task_set[current_idx].magic[ now_read_id ] = magic;
                        analysis_pg_task_set[current_idx].pid[ now_read_id ] = pid;
                        analysis_pg_task_set[current_idx].ptr_w ++;

                        current_idx ++;
                        current_idx = current_idx % MAX_ANALYSIS_NUM;

                        *dev_readptr = 14 - left;
		}
        }
}


void check_pte(char *ch_ptr){
	unsigned int seq_no,r_w;
        unsigned long paddr;
        unsigned long long timer;
        unsigned int pid;
        unsigned long ppn;
        unsigned long vpn;
        uint64_t val;
        char magic;

	int tag_seq = (int)(*(char*)(ch_ptr + 13));
	magic =  (*(char*)(ch_ptr));
	if(magic == set_page_table_magic){
		pid = (*(int*)((ch_ptr + 1)));
		val = (*(uint64_t*)(ch_ptr + 5));
		ppn = val & 0xffffff;
                vpn = (val >> 24) & 0xffffffffff;
		if (ppn >= MAX_PPN) {
	                printf("invalid ppn\n");
		}

		if(pid == now_pid)
			return ;
		if (using_pid != -1 && pid == using_pid){
			ppn2pid[ppn] = pid;
                        ppn2vpn[ppn] = vpn;
                }
		else if(using_pid == -1){
			ppn2pid[ppn] = pid;
			ppn2vpn[ppn] = vpn;
		}
	}
	else  if(magic == free_page_table_get_clear || magic == free_page_table_get_clear_full || magic == free_page_table_magic){
		pid = (*(int*)((ch_ptr + 1)));
                val = (*(uint64_t*)(ch_ptr + 5));
                ppn = val & 0xffffff;
                vpn = (val >> 24) & 0xffffffffff;
                if (ppn >= MAX_PPN) {
                        printf("invalid ppn\n");
                }
		if(pid == now_pid)
                        return ; 
		if (using_pid != -1 && pid == using_pid){
                        ppn2pid[ppn] = 0;
                        ppn2vpn[ppn] = 0;
                }
                else if(using_pid == -1){
                        ppn2pid[ppn] = 0;
                        ppn2vpn[ppn] = 0;
                }
		
#ifdef MONITOER_PREFETCH
		new_ppn[ppn].isprefetch=0;
		new_ppn[ppn].prefetch_time=0;
#endif
		ppn2num[ppn] = 0;
	}
}


int analysis_kt_buffer_for(void  *arg){
	unsigned int seq_no,r_w;
        unsigned long paddr;
        unsigned long long timer;
        unsigned int pid;
        unsigned long ppn;
        unsigned long vpn;
        uint64_t val;
        char magic;
        int first_flag = 0;
        while(1){
                if(finish_kt_analysis == 0)
                {
                        printf(" analysis_kt_buffer finish , kt_hmtt_reading_addr = 0 , kt_hmtt_writing_addr = 0\n");
                        break;
                }

                        //start 
                        unsigned long addr = 0;
/*
                        if (glb_start_analysis == 0 && kernel_trace_tag == DUMP_PAGE_TABLE_TAG) {
                                printf("find DUMP_PAGE_TABLE_TAG in hmtt trace \n");
                                glb_start_analysis = 1;
                                printf("dump page done.\n");
                                printf("trace init done.\n");
                                continue;
                        }
*/
                        if(glb_start_analysis == 0 )
                        {
                                continue ;

                        }
//                      if (strncmp(kt_ch_tmp, "$$$$$$$$$$$$$", 13) == 0) {
//                              return ;
//                        } 

			while(*dev_readptr == *dev_writeptr){}
			
			if( *dev_readptr + 14 < buffer_size ){

				unsigned long now_write_ptr = *dev_writeptr;


				if(*dev_readptr < now_write_ptr){

					unsigned long  tmp_in = ( now_write_ptr - *dev_readptr );
        	                        if(tmp_in > 30000){
                	                      printf("left analysis kt trace = %lu\n", tmp_in);
                        	        }
                                	if(max_kt_buffer_offst < tmp_in)
                                        	max_kt_buffer_offst = tmp_in;

					char * ch_ptr = NULL;
		                        ch_ptr = p_kernel_trace_buf + *dev_readptr;
					for(int i = 0 ; i < ( now_write_ptr - *dev_readptr) / 14; i++){
						ch_ptr = p_kernel_trace_buf + *dev_readptr + i * 14;
						check_pte( ch_ptr );
					}
					*dev_readptr += ( now_write_ptr - *dev_readptr);
				}
				else{
					char * ch_ptr = NULL;
                                        ch_ptr = p_kernel_trace_buf + *dev_readptr;
					for(int i = 0 ; *dev_readptr + i < buffer_size; i += 14){
                                                ch_ptr = p_kernel_trace_buf + *dev_readptr + i;
                                                check_pte( ch_ptr );
						*dev_readptr += 14;
                                        }
				}
			}
			else{ // last page table trace
				unsigned long left = buffer_size - *dev_readptr;
				memcpy(kt_ch_tmp, p_kernel_trace_buf + *dev_readptr, left);
				memcpy(kt_ch_tmp + left, p_kernel_trace_buf, 14 - left);
				check_pte( kt_ch_tmp );
				*dev_readptr = 14 - left;
			}
	}
}


