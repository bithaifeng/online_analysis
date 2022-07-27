#define _GNU_SOURCE
/*
#include <asm/unistd.h> 
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sched.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <map>

#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>

#include <vector>
*/
//#include "/home/lhf/lhf/hmtt_kernel_control/hmtt_kernel.h"
#include "/home/lhf/lhf/hmtt_kernel_control_direct_64G/hmtt_kernel.h"
#include "prefetch_mine.h"
#include "page_table.h"
#include "/home/lhf/receiver_driver_hopp_64g/cfg_content.h"
//#include "evict.h"
#include "config.h"
#include <limits.h>


#include "filter_table.h"

#include "lt_profile.h"
#include "lightswapinterface.h"

#include "memory_manage.h"

using namespace std;

cpu_set_t mask_cpu_2;
// cpu_set_t mask_cpu_hmtt;
// cpu_set_t mask_cpu_train;


cpu_set_t mask_kt;
cpu_set_t mask_cpu_8;
cpu_set_t mask_cpu_prefetch_seek;
cpu_set_t mask_cpu_prefetch;
cpu_set_t mask_prefetch_seek;


extern void store_to_trace_store_buff(unsigned long vpn, unsigned char flag);

map<unsigned long, unsigned char> prefetch_pid_flag[65536];


int analysis_kt_num = 0;

unsigned long syscall_use = 0;

int prefetch_flag = 10;

inline int set_cpu(int i)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
 
    CPU_SET(i,&mask);
 
    printf("thread %u, i = %d\n", pthread_self(), i);
    if(-1 == pthread_setaffinity_np(pthread_self() ,sizeof(mask),&mask))
    {
        fprintf(stderr, "pthread_setaffinity_np erro\n");
        return -1;
    }
    return 0;
}



int using_pid = -1;

#define KT_HMTT_SIZE  (2ULL << 30)
#define BLK_DELTA 8
#define ISSTREAM 4
#define HIST_TSIZE 1024
#define STREAM_TSIZE 256
#define PG_DELTA (10ULL << 12)


//#define USE_SLEEP
//

//#define _GNU_SOURCE
#define MAXPPN MAX_PPN

#if 1
#define WRITE write
#else
int WRITE(int handle, void *buf, int nbyte){
	return nbyte;
}
#endif

#define	 DMA
//#include "../GetCfgContent.h"

#define phys_to_virtual(phys) (phys - DMA_REG_ADDR + memory_dev_addr)
//#define store_phys_to_virtual(phys) (phys - + );

	char filename[64];

volatile unsigned long  duration_all = 0;
unsigned long max_latency = 0;
unsigned long          last_writeptr;
volatile unsigned long *dev_writeptr;
unsigned long *dev_readptr;
unsigned long long kernel_trace_offset = 0;
unsigned long long tagp = 0;
volatile unsigned long *ppn2vpn = NULL;
volatile unsigned long *ppn2rpt = NULL;


unsigned long pgtable_transfer( unsigned long ppn ){
	return ppn2rpt[ppn];
}


volatile int *ppn2pid = NULL;
int glb_start_analysis = 0;
volatile unsigned long long total_trace = 0;
//volatile unsigned long long none_pte = 0;
//std::atomic< int  > total_trace;
std::atomic< int > none_pte;

std::atomic< int > store_index (0);


//#define PRINT_MSG
//#define STORE_FILE
#ifdef STORE_FILE
#endif 

unsigned long long last_none_pte = 0;

unsigned long long set_pte_cnt = 0, set_pte_cnt_w = 0;
unsigned long long free_pte_cnt = 0, free_pte_cnt_w = 0;
unsigned long long some_error_trace = 0;
unsigned long long kernel_config_entry = KERNEL_TRACE_CONFIG_ENTRY_ADDR << 20;
unsigned long long kernel_config_end =
        (KERNEL_TRACE_CONFIG_ENTRY_ADDR + KERNEL_TRACE_CONFIG_SIZE * KERNEL_TRACE_SEQ_NUM) << 20;

char free_pt_addr_magic     = 0xfd;
char set_page_table_magic   = 0xec;
char free_page_table_magic  = 0xfc;
char free_page_table_get_clear = 0x22;
char free_page_table_get_clear_full = 0x33;
unsigned long long skip_free_in_set = 0;

unsigned long long redundant_set_pte = 0, redundant_free_pte = 0;


int now_pid = -1;
int using_inter_page = 400;



int last_kernel_trace_seq = -1;
//#define STORE_TRACE


char *memory_dev_addr;
extern int errno;

volatile unsigned long long *ptr_writing_addr;
unsigned long long *ptr_stopt_addr;
//unsigned long long  writing_addr;
unsigned long long  reading_addr;
unsigned long long  stopt_addr;

volatile unsigned long long glb_writing_addr;// = DMA_BUFFER_ADDRESS;
volatile unsigned long long glb_reading_addr;// = DMA_BUFFER_ADDRESS-1;


unsigned long long kt_hmtt_writing_addr = 0;
unsigned long long kt_hmtt_reading_addr = 0;


unsigned long long tmp_reading_addr = 0;
unsigned long long tmp_writing_addr = 0;

unsigned long long tmp_phy_buffer_addr = 0;
double use_percent = 0.7;



unsigned long long writing_size;
unsigned long long reading_size;
struct timeval	tvafter_writing, tvpre_writing;
struct timeval	tvafter_reading, tvpre_reading;
struct timezone tz;
double tv_writing=0;
double tv_reading=0;
double writing_speed;
double reading_speed;

double sum_tv_writing=0;
struct timeval	tvlast_writing, tvfirst_writing;
char datetime[200];

int direct_io = 0;
int flag = O_RDWR|O_CREAT|O_TRUNC;

int overflow = 0;
unsigned long long error_aligning = 0;

int fp;

int last_hmtt_seq = -1;
int last_kernel_seq = -1;
int start_cycle = 0;
volatile unsigned long long miss_set_pte = 0, miss_free_pte = 0;
volatile unsigned long long last_miss_set_pte = 0, last_miss_free_pte = 0;


#if 1
#define LRU_LSD_MUN 64
#define MAX_INTER 40
#define LONGSTRIDE_STRIDEN 8
#define STRIDEN LONGSTRIDE_STRIDEN
struct LST_LRU{
//	struct LST_LRU * pre;
//	struct LST_LRU  *last;
	char valid;
	unsigned long start_vpn;
	unsigned long ppn[LONGSTRIDE_STRIDEN];
	unsigned long vpn[LONGSTRIDE_STRIDEN];
//	unsigned char stride[LONGSTRIDE_STRIDEN];
	unsigned long accessn[LONGSTRIDE_STRIDEN];
	int pid;
	int access_times;
	int size;
	unsigned long current_time;
};



struct LST_LRU table[STREAM_TSIZE] = {0};
/*
struct RT{
	int access_times;
	unsigned long ppn;
	unsigned long long current_time;
};
*/
struct RT{
	char valid;
	unsigned long start_blk;
    unsigned long end_blk;
    unsigned long accessn;
    unsigned long current_time;
};

#define RT_NUM 256
struct RT history[HIST_TSIZE];
map<unsigned long, unsigned char> ppn2RT;

map<unsigned long, unsigned char>  ppn2LST_LRU;

int delta(unsigned long a,unsigned long b){
	if (a > b) {
		return (a - b);
	} else {
		return (b - a);
	}
}


int checkInter(RT &h, int inx){
	if (table[inx].valid <= 0) return -1;
}

int last_inx_use = -1;

void sortPage(int inx){
	for (int i = 0; i < table[inx].size; i++) {
        for (int j = i; j < table[inx].size; j++) {
            if (table[inx].vpn[i] > table[inx].vpn[j]) {
                unsigned long tmp = table[inx].vpn[i];
				unsigned long tmp_ppn = table[inx].ppn[i];
                table[inx].vpn[i] = table[inx].vpn[j];
                table[inx].vpn[j] = tmp;
				table[inx].ppn[i] = table[inx].ppn[j];
                table[inx].ppn[j] = tmp_ppn;
            }
        }
    }
}

void insert_LST_LRU(RT &h, int pid, unsigned long vpn, unsigned long now_cycle ){
	int inx = -1;
	int inter = -1;
	unsigned long minStride = 1ULL << 30;
	for (int i = 0; i < STREAM_TSIZE; i++) {
		if (table[i].valid > 0) {
			if (table[i].size == 0) continue;
			if(table[i].pid != pid) continue;
			unsigned long tmp = delta(vpn, table[i].vpn[table[i].size - 1]);
			if(tmp < minStride && tmp < (PG_DELTA >> 12) )
			{
				inx = i;
            	minStride = tmp;
				break;
			}
		}
	}
	if (inx >= 0) {
		// insert page here
		if (table[inx].size >= STRIDEN) {
			for (int i = 0; i < table[inx].size - 1; i++) {
                table[inx].ppn[i] = table[inx].ppn[i+1];
				table[inx].vpn[i] = table[inx].vpn[i+1];
            }
            table[inx].size--;
		}
		table[inx].ppn[table[inx].size] = h.start_blk >> 6;
		table[inx].vpn[table[inx].size] = vpn;
		table[inx].size++;
		if(last_inx_use != inx){
//			printf("<add stream> id = %d, pid = %d, vpn = %lu\n", inx, 
//			table[inx].pid, vpn);
			last_inx_use = inx;
		}

		//sort 
		sortPage(inx);
	}
	else{
		// create stream
        // find empty stream entry
        for (int i = 0; i < STREAM_TSIZE; i++) {
            if (table[i].size == 0) {
                inx = i;
                break;
            }
        }
		if (inx < 0) {
            // stream evict
            inx = -1;
            unsigned long early = now_cycle;
            for (int i = 0; i < STREAM_TSIZE; i++) {
                if (table[i].current_time < early) {
                    early = table[i].current_time;
                    inx = i;
                }
            }
        }
		table[inx].size = 1;
		table[inx].pid = pid;
		table[inx].vpn[0] = vpn;
		table[inx].valid = 1;
		table[inx].ppn[0] = h.start_blk >> 6;
		table[inx].accessn[0] = 0;
		table[inx].current_time = now_cycle;
		table[inx].access_times = 1;
		
	}

/*
	int i = 0 , j = 0;
	int tmp_len = 0;
	int found = 0;
	for(i = 0; i < LRU_LSD_MUN ; i++){
		if(pid == table[i].pid){
			tmp_len = 0;
			if(  table[i].start_vpn - MAX_INTER < vpn && vpn < table[i].start_vpn + MAX_INTER ){
				// update LST index ppn vpn now_cycle
				table[i].current_time = now_cycle;
				update_LST_LRU(ppn, vpn, i);
			}

		}
	}
*/	
}

int print_f = 10;

int checkStream(unsigned long p_addr, unsigned long tt){
	return 0;
}





#endif



volatile char prefech_flag[724288] = {0};

unsigned long long kt_hmtt_addr;
map<unsigned long, unsigned long> vpn2ppn;
volatile map<unsigned long, char> vpn2ppn_prefetch;


#if 1
void RT_read_report(unsigned long long reading_size){
	reading_speed = (double)reading_size/tv_reading/1024/1024;
	printf("					the store speed is %7.2f MiB/s.\r",reading_speed);
	fflush(stdout);
}
#else
#define RT_read_report(...) 
#endif

void RT_read_and_write(unsigned long long reading_size, unsigned long long writing_size){
	printf("$ The trace bandwidth is %7.2f MiB/s;   the store speed is %7.2f MiB/s.\r", writing_size/ tv_reading/1024/1024,reading_size/ tv_reading/1024/1024 );
// ,writing_size/ tv_reading/1024/1024);
	fflush(stdout);
}



#if 1
void RT_write_report(unsigned long long writing_size){
	writing_speed = (double)writing_size/tv_writing/1024/1024;		//in MB/s
//	printf("$ The trace bandwidth is %7.2f MiB/s;									\r",writing_speed);
//	fflush(stdout);
	sum_tv_writing += tv_writing;
}
#else
#define RT_write_report(...) 
#endif

void gettime(){
	time_t now;
	struct tm *tm_now;
	
	time(&now);
	tm_now = localtime(&now);
	strftime(datetime, 200, "%Y-%m-%d %H:%M:%S", tm_now);
}

#define __u64 unsigned long long 
#define __u32 unsigned int 

static inline __u64 get_cycles(void)
{
        __u32 timehi, timelo;
        asm("rdtsc":"=a"(timelo),"=d"(timehi):);
        return (__u64)(((__u64)timehi)<<32 | (__u64)timelo);
}
//kernel_address

static char *kernel_trace_path = "/dev/hmtt_kernel";
int kt_ff;
unsigned long dev_size, buffer_size; //
unsigned char *p_kernel_trace = NULL, *p_kernel_trace_buf = NULL;

static char *kt_hmtt_trace_path = "/dev/malloc_tag";
int kt_hmtt_ff = -1;
unsigned long kt_hmtt_dev_size, kt_hmtt_buffer_size;
unsigned char *kt_hmtt_p_kernel_trace = NULL, *kt_hmtt_p_kernel_trace_buf = NULL;


volatile unsigned long *trace_store_writeptr;
volatile unsigned long *trace_store_readptr;



char *kt_ch;
char *ht_ch;
char kt_ch_tmp[20] = {0};
char ht_ch_tmp[20] ={0} ;
int kt_left = 0;
int  ht_left = 0;
struct record_t {
    unsigned int rw;
    int pid;
    unsigned long tm;
    unsigned long paddr;
    unsigned long long vaddr;
};
struct record_t record;

//#define PREFETCH_BUFFER_SIZE 100000
#define PREFETCH_BUFFER_SIZE 2204288

#define TRAINING_BUFFER_SIZE 5200000

unsigned long prefetch_buffer[PREFETCH_BUFFER_SIZE] = {0};
unsigned long prefetch_buffer_ppn[PREFETCH_BUFFER_SIZE] = {0};
int prefetch_buffer_pid[PREFETCH_BUFFER_SIZE] = {0};
int prefetch_buffer_inter_page[PREFETCH_BUFFER_SIZE] = {0};
volatile unsigned int ppn2num[MAXPPN] = {0};


struct prefetch_entry prefetch_buffer_entry[ PREFETCH_BUFFER_SIZE ] = {0};

/*
struct ppn2num_t{
//	volatile unsigned int num;
	unsigned char num;
	unsigned long timer;
};
*/
volatile unsigned long ac_counter=0;
unsigned long pre_counter=0;

volatile unsigned long latency_all = 0;
volatile unsigned long last_latency_all = 0;
volatile unsigned long last_ac_counter = 0;


//struct ppn2num_t new_ppn[MAX_PPN] = {0};
#ifdef OTHERPPN
struct ppn2num_t * new_ppn;
#else
struct ppn2num_t new_ppn[MAX_PPN] = {0};
#endif

unsigned long start_new_ppn_address = 0;



//volatile int pb_w_ptr = 0, pb_r_ptr = 0;
//std::atomic< int  > pb_w_ptr;
volatile int   pb_w_ptr;
volatile int pb_r_ptr;
int pb_w_ptr_pid,pb_r_ptr_pid;

//std::atomic< int  > pb_r_ptr;

//std::atomic< int  > tb_w_ptr;
//std::atomic < int > tb_r_ptr;
volatile unsigned long  tb_w_ptr, tb_r_ptr;


volatile unsigned long training_buffer[TRAINING_BUFFER_SIZE] = {0};
volatile unsigned long training_time[TRAINING_BUFFER_SIZE] = {0};
struct evict_transfer_entry_struct training_transfer[TRAINING_BUFFER_SIZE] = {0};






pthread_t tid[max_prefetcher_count];
pthread_t tid_4;
pthread_t tid_train;

struct prefetch_task prefetcher_task_set[max_prefetcher_count];

unsigned long syscall_all_time = 0;

volatile int prefetch_program[max_prefetcher_count][1058576] = {0};


void print_pthread_id(void *arg){
	struct prefetch_task* task = (struct prefetch_task*)arg;
	task->real_tid = pthread_self();
	int bug_flag = 0;
	printf("curent tpid = %u, id = %d\n", pthread_self(), task->tid);
	int  i = 0;
	while(1){
//		if(task->task_len != 0) 
//			printf("task->vaddr'vpn = %lu, task->tid = %d\n", task->vaddr >> 12, task->tid);
#if 0
		if(task->task_len != 0){
//			printf("task->vaddr'vpn = %lu, task->tid = %d\n", task->vaddr >> 12, task->tid);
//			int ret = syscall(335, using_pid, task->vaddr, 0);
			int ret = syscall(335, task->pid, task->vaddr, 0);
//			printf("******************** ret = %d, vaddr = 0x%lx, thread id = %d\n", ret ,  task->vaddr, task->tid);
			task->task_len = 0;
		}
#endif
		if(task->ptr_w > task->ptr_r){
			if(task->ptr_w - task->ptr_r > PREFETCH_BUFFER_PTHREAD){
				if(bug_flag == 0){
					bug_flag = 1;
					printf("overflow task->tid = %d, inter = %u, ptr_w = %u, ptr_r = %u  \n",  task->tid, task->ptr_w - task->ptr_r, task->ptr_w, task->ptr_r);
				}
			
			}
			int tmp_len = task->ptr_w - task->ptr_r;
			unsigned long tmp_ppn = task->vaddr[task->ptr_r % PREFETCH_BUFFER_PTHREAD] >> 12;
			unsigned long value_d = ppn2rpt[ tmp_ppn ];
		        unsigned long vpn_d = 0;
		        int pid_d = 0;
		        vpn_d = ( value_d  >> 16) & 0xffffffffff;
		        pid_d = value_d & 0xffff;


			unsigned long tmp_vpn = vpn_d;
			int tmp_pid = pid_d;
			int tmp_inter_page =  task->p_inter_page[task->ptr_r % PREFETCH_BUFFER_PTHREAD];

//			if(tmp_vpn >1000000 && tmp_pid != now_pid){
			if( tmp_pid != now_pid && tmp_pid != 0 && tmp_len)// <=1600)
			{
				
				if(using_pid == tmp_pid  )
	                        {
					;
#ifdef STORE_FILE
//	                                store_to_trace_store_buff( tmp_vpn +  tmp_inter_page , 1  );
#endif
	                        }

				if(i < 10)
				{
//					printf("%lu\n");
					for(int j = 0; j < i;j ++)
						printf("*");
					printf("tid = %d, pid = %d , prefetch_vpn = %lu, now_vpn = %lu\n",   task->tid, tmp_pid  , tmp_vpn +  tmp_inter_page, tmp_vpn);
					i++;
				}
				unsigned long start_c = get_cycles();
				// unsigned long ret = syscall(335,  tmp_pid  , ( tmp_vpn +  tmp_inter_page ) << 12  , 0);
                unsigned long ret = make_fetch_page(tmp_pid, ( tmp_vpn +  tmp_inter_page ));

				syscall_use ++;
				unsigned long end_c = get_cycles();
				if(end_c - start_c > 300000){
					;
//					printf("<syscall>vpn offset=%lu, use %lu cycles\n", ( tmp_vpn +  tmp_inter_page )- 34358653424, end_c - start_c);
				}
				if(( tmp_vpn +  tmp_inter_page )- 34358653424 >=0 && ( tmp_vpn +  tmp_inter_page )- 34358653424 <= 1048576  )
				{
					unsigned long tmp_inter = ( tmp_vpn +  tmp_inter_page )- 34358653424;
					prefetch_program[task->tid][tmp_inter / 100000] ++;

				}
//				unsigned long ret = syscall(335,  using_pid  , ( tmp_vpn +  tmp_inter_page ) << 12  , 0);
//				if(ret>0 && ret<MAX_PPN){
				if(ret>10 && ret<MAX_PPN){
                                        pre_counter++;
					syscall_all_time += (end_c - start_c);
#ifdef MONITOER_PREFETCH
					new_ppn[ret].isprefetch = (new_ppn[ret].isprefetch | 1) ; //set prefetch flag
//                                        new_ppn[ret].isprefetch=1;
                                        new_ppn[ret].prefetch_time =  duration_all ;
#endif
					
                                }else if(ret>MAX_PPN){
//                                        printf("ERROR, %lu\n",ret);
                                }
			}
			
			int prefetch_tmp_len = task->ptr_w - task->ptr_r;
			if(prefetch_tmp_len > 200){
//				task->ptr_r  = task->ptr_r + prefetch_tmp_len / 50;		
				task->ptr_r  = task->ptr_r + prefetch_tmp_len / 2;		
				task->delete_num += prefetch_tmp_len / 2;
				printf("***********prefetch_tmp_len = %lu\n", prefetch_tmp_len);
			}
			else

				task->ptr_r ++;
		}
		else{
			//maybe set pthread sleep?
		}
	}
}

extern void analysis_single_trace(char *trace_start);




//#define USE_PROCESS_PTHREAD

volatile int monitor_trace_num = 0;
volatile int send_monitor_trace_num = 0;

int monitor_num = 0;
int monitor_num_split[200] = {0};
int monitor_num_split_first_100k[200] = {0};

int monitor_num_pte = 0;
int monitor_num_split_pte[200] = {0};

int HMTT_trace_num[200] = {0};

unsigned long moniter_base_address = 0x7ffef6dc3010;





extern void prefetch_monitor_seek();

void init_prefetch_thread(){
	for(int i = 0; i < max_prefetcher_count; i ++){
		prefetcher_task_set[i].tid = i;
		prefetcher_task_set[i].task_len = 0;
//		prefetcher_task_set[i].vaddr = 0;
		
		prefetcher_task_set[i].ptr_r = 0;
		prefetcher_task_set[i].delete_num = 0;
		prefetcher_task_set[i].ptr_w = 0;
//		prefetcher_task_set[i].current_time = 0;
//		prefetcher_task_set[i].pid = 0;
	}

	
	
	pthread_create(&tid_4, NULL, prefetch_monitor_seek, NULL);
//	pthread_setaffinity_np(tid_4, sizeof(mask_cpu_8), &mask_cpu_8);
	pthread_setaffinity_np(tid_4, sizeof(mask_cpu_prefetch_seek), &mask_cpu_prefetch_seek);
//	pthread_setaffinity_np(tid_4, sizeof(mask_prefetch_seek), &mask_prefetch_seek);

#ifdef USE_MORE_CORE
	for(int i = 0; i < max_prefetcher_count; i ++){

		pthread_create(&tid[i], NULL, print_pthread_id , (void*)(&prefetcher_task_set[i]));
	        pthread_setaffinity_np(tid[i], sizeof(mask_cpu_prefetch), &mask_cpu_prefetch);
		printf("i = %d, create\n",i);
	}
#else
	for(int i =0 ; i < max_prefetcher_count; i++){
		pthread_create(&tid[i], NULL, print_pthread_id , (void*)(&prefetcher_task_set[i]));
	        pthread_setaffinity_np(tid[i], sizeof(mask_cpu_8), &mask_cpu_8);
		printf("i = %d, create\n",i);
	}
#endif
	printf("start join\n");
	printf("end init_prefetch_thread\n");
}
volatile unsigned long  now_seq = 0;

void  prefetch_monitor_seek(){
	int cur_prefetcher_idx = 0;
	printf("start prefetch_monitor_seek\n");
	int last_pb_r = -1, last_pb_w = 0;
	int rand_id0, rand_id1;
	int continue_print = 1;


	while(1){
		if(last_pb_r != pb_r_ptr || pb_w_ptr != last_pb_w){
	//		printf("pb_r_ptr = %d, pb_w_ptr = %d\n", pb_r_ptr, pb_w_ptr);
		}
		last_pb_r = pb_r_ptr;
		last_pb_w = pb_w_ptr;
		if(now_seq % 5000 == 19 ){
			int i;
			int max_len = 0, all_len = 0;
			for(i = 0 ; i < max_prefetcher_count; i++){
				unsigned long tm = ( prefetcher_task_set[i].ptr_w - prefetcher_task_set[i].ptr_r);
				all_len += tm;
				if(max_len < tm)
					max_len = tm;
			}
			if((float)all_len/ (float)max_prefetcher_count > 8)
				if(max_len != 0 && continue_print == 1){
					// printf("max_len = %d, average len = %.3f\n",  max_len, (float)all_len/ (float)max_prefetcher_count );
				}
				else
					continue_print = 0;
		}
		else continue_print = 1;
		

		if(pb_r_ptr < pb_w_ptr){

			cur_prefetcher_idx = (cur_prefetcher_idx + 1) % max_prefetcher_count;
			now_seq ++;
#if 1
			rand_id0 = rand()% max_prefetcher_count;
			rand_id1 = rand() % max_prefetcher_count;
			if(  prefetcher_task_set[ rand_id0 ].ptr_w - prefetcher_task_set[ rand_id0 ].ptr_r > prefetcher_task_set[ rand_id1 ].ptr_w - prefetcher_task_set[ rand_id1 ].ptr_r  )
			{
				cur_prefetcher_idx = rand_id1;
			}
			else 
				cur_prefetcher_idx = rand_id0;
#endif

			unsigned long tmp_p_addr = prefetch_buffer[pb_r_ptr % PREFETCH_BUFFER_SIZE] << 12;
			unsigned long value_s , vpn_s;
			int pid_s;
			value_s = ppn2rpt[ tmp_p_addr  >> 12 ];
			vpn_s = ( value_s  >> 16) & 0xffffffffff;
			pid_s =  value_s & 0xffff;
//			if(pid_s != now_pid && pid_s != 0)
//				printf(" access vpn = %llu = 0x%lx,  pid = %d , PA = %lx \n",  vpn_s  , vpn_s,  pid_s , tmp_p_addr);
			

			if(using_pid == pid_s)
			{
				monitor_num ++;
#ifdef PRINT_MSG
				printf("*** access vpn = %llu = 0x%lx,  pid = %d , PA = %lx \n",  vpn_s  , vpn_s,  pid_s , tmp_p_addr);
			
#endif
				int tmp_index = ( vpn_s - (moniter_base_address >> 12)) / 100000;
				if( ( vpn_s - (moniter_base_address >> 12)) % 100000 < 10){
					HMTT_trace_num[tmp_index] = total_trace; 
				}
				else if( ( vpn_s - (moniter_base_address >> 12)) % 100000 > (100000 - 10) ){
					HMTT_trace_num[tmp_index + 1] = total_trace;
				}

				if(tmp_index < 200 && tmp_index >= 0)
				{
					if( tmp_index < 1){
						monitor_num_split_first_100k[ ( vpn_s - (moniter_base_address >> 12)) / 1000 ] ++ ;

					}
					monitor_num_split[tmp_index] ++;
				}
			}
			else{
//				printf("[none_pte] access vpn = %llu,  0x%lx  \n",  tmp_vpn >> 12, tmp_vpn >> 12  );
				;

			}


//			int tmp_pid = prefetch_buffer_pid[pb_r_ptr % PREFETCH_BUFFER_SIZE];
#if 1
			int tmp_inter_page = prefetch_buffer_inter_page[pb_r_ptr % PREFETCH_BUFFER_SIZE];
	
			prefetcher_task_set[ cur_prefetcher_idx ].vaddr[ prefetcher_task_set[ cur_prefetcher_idx ].ptr_w % PREFETCH_BUFFER_PTHREAD] = prefetch_buffer[pb_r_ptr % PREFETCH_BUFFER_SIZE] << 12;
//			prefetcher_task_set[ cur_prefetcher_idx ].pid[ prefetcher_task_set[ cur_prefetcher_idx  ].ptr_w % PREFETCH_BUFFER_PTHREAD] = prefetch_buffer_pid[pb_r_ptr % PREFETCH_BUFFER_SIZE];
			prefetcher_task_set[ cur_prefetcher_idx ].p_inter_page[ prefetcher_task_set[ cur_prefetcher_idx  ].ptr_w % PREFETCH_BUFFER_PTHREAD] = prefetch_buffer_inter_page[pb_r_ptr % PREFETCH_BUFFER_SIZE];
			prefetcher_task_set[ cur_prefetcher_idx ].ptr_w ++;
#endif
			pb_r_ptr ++;
		}
	}
}

//void store_to_pb(unsigned long vpn, unsigned long ppn){




unsigned long store_to_pb_num = 0;
unsigned long store_to_tb_num = 0;




void store_to_pb_entry(struct prefetch_entry new_entry){
	store_to_pb_num ++;
	if( pb_w_ptr - pb_r_ptr > PREFETCH_BUFFER_SIZE ){
		for(int i = 0 ;i < 20; i ++)
	                printf("$$$$$ prefetch buffer overflow !!!!\n");
        }
}




void store_to_pb(unsigned long ppn, int inter_page){
	if(ppn >= MAXPPN)
		return ;

	store_to_pb_num ++;
//	if(pb_w_ptr >= max_prefetcher_count) return ;

	if( pb_w_ptr - pb_r_ptr > PREFETCH_BUFFER_SIZE ){
		printf("$$$$$ prefetch buffer overflow !!!!\n");
	}
//	printf("store_to_pb ,vpn = %lu, pb_w_ptr = %d, pb_r_ptr = %d\n", vpn, pb_w_ptr, pb_r_ptr);
//	prefetch_buffer[pb_w_ptr.fetch_add(1) % PREFETCH_BUFFER_SIZE] = vpn;
#if 0
	prefetch_buffer[pb_w_ptr % PREFETCH_BUFFER_SIZE] = vpn;
//	prefetch_buffer_pid[pb_w_ptr % PREFETCH_BUFFER_SIZE] = pid;
	prefetch_buffer_ppn[ pb_w_ptr % PREFETCH_BUFFER_SIZE ] = ppn;
#endif

	prefetch_buffer[pb_w_ptr % PREFETCH_BUFFER_SIZE] =  ppn;
	prefetch_buffer_inter_page[ pb_w_ptr % PREFETCH_BUFFER_SIZE ] = inter_page;
	pb_w_ptr ++;
}

//void store_to_tb(unsigned long vpn){
#ifdef USING_PAGE_DISTRIBUTION
void store_to_tb(unsigned long ppn, unsigned long time, struct evict_transfer_entry_struct transfer_entry){
#else
void store_to_tb(unsigned long ppn, unsigned long time){
#endif
	store_to_tb_num ++;
//	if(store_to_tb_num % 7 == 1) return;
	if( tb_w_ptr - tb_r_ptr > TRAINING_BUFFER_SIZE ){
		 printf("$$$$$ training buffer overflow !!!!\n");
	}
//	if(tb_w_ptr % 100 == 0) printf("");
//	training_buffer[ tb_w_ptr.fetch_add(1) % TRAINING_BUFFER_SIZE ] = vpn;
	training_buffer[ tb_w_ptr % TRAINING_BUFFER_SIZE ] = ppn;
	training_time[ tb_w_ptr % TRAINING_BUFFER_SIZE ] = time;
#ifdef USING_PAGE_DISTRIBUTION
	training_transfer[ tb_w_ptr % TRAINING_BUFFER_SIZE ] = transfer_entry;
#endif
	tb_w_ptr ++;

}

int print_ti = 10;

int print_times_train = 0;
unsigned long time_use_train = 0;

#define PRINT_TRAIN 50000

//#define PRINT_TRAIN_FLAG
int print_round = 0;

int max_training_len = 0;

void training_seek(){
	printf("start training_ seek !\n");
	unsigned long st,ed;
	while(1){

		if(tb_r_ptr < tb_w_ptr){
//                        unsigned long tmp_vpn = training_buffer[tb_r_ptr % TRAINING_BUFFER_SIZE];
			unsigned long tmp_tb_len = tb_w_ptr - tb_r_ptr;
			if(tmp_tb_len > max_training_len)
				max_training_len = tmp_tb_len;
//#define LSDSTREAM_SIZE 64
			if(tmp_tb_len > 500){
				;
				print_round ++;
				// if(print_round % 20 == 1)
#ifndef USING_PAGE_DISTRIBUTION
				     printf("    <train buffer> len = %lu\n", tmp_tb_len);
				tb_r_ptr += (tmp_tb_len / 2);
#endif
			}
			
			print_times_train ++;
#ifdef PRINT_TRAIN_FLAG
			st = get_cycles();
			if(print_times_train % PRINT_TRAIN == PRINT_TRAIN - 1){
				printf("<training speed> = %lu cycle per trains\n", time_use_train / (PRINT_TRAIN - 1));
				time_use_train = 0;
				
			}
#endif
			
                        unsigned long tmp_ppn = training_buffer[tb_r_ptr % TRAINING_BUFFER_SIZE];
			unsigned long tmp_time = training_time[tb_r_ptr % TRAINING_BUFFER_SIZE];
#ifdef USING_PAGE_DISTRIBUTION
			struct evict_transfer_entry_struct tmp_evict_buffer_struct = training_transfer[ tb_r_ptr % TRAINING_BUFFER_SIZE ];
#endif

			tb_r_ptr ++;
			//check is not necessary
			if( print_ti > 5){
				print_ti --;
				printf("training_sekk ppn = %lu. time  = %lu\n", tmp_ppn, tmp_time);
			}

			// store_to_trace_store_buff( tmp_ppn, 3);

			unsigned long value_d = ppn2rpt[tmp_ppn];
			unsigned long vpn_d = 0;
		        int pid_d = 0;
		        vpn_d = ( value_d  >> 16) & 0xffffffffff;
		        pid_d = value_d & 0xffff;

#ifdef EVICT_ON
                // if(pid_d == using_pid && (vpn_d >= (moniter_base_address>>12)) && (vpn_d < (moniter_base_address>>12)+4*(1UL<<18)))
                //     statics_access_log(vpn_d - (moniter_base_address>>12));
#endif

#ifdef use_userspace_lru_alg
            hot_page_lru_control(tmp_ppn);
#endif

			if(vpn_d != 0 && pid_d != now_pid)
			{

				if( print_ti > 0){
                                	print_ti --;
//	                                printf("insert ppn = %lu. time  = %lu\n", tmp_ppn, tmp_time);
                        	}

//				if(using_pid == ppn2pid[ prefetch_buffer[pb_r_ptr % PREFETCH_BUFFER_SIZE] ]  )
				if(using_pid == pid_d  )
				{
					monitor_num++;
					;
#ifdef STORE_FILE
//					store_to_trace_store_buff(ppn2vpn[tmp_ppn] , 0);
#endif
				}

//				InsertNewLSD_vpn(tmp_ppn, ppn2vpn[tmp_ppn], tmp_time, ppn2pid[tmp_ppn]);
#ifdef USING_PAGE_DISTRIBUTION
				InsertNewLSD_vpn(tmp_ppn, vpn_d, tmp_time, pid_d, tmp_evict_buffer_struct);
#else
				InsertNewLSD_vpn(tmp_ppn, vpn_d, tmp_time, pid_d);
#endif
			}
#ifdef PRINT_TRAIN_FLAG
			ed = get_cycles();
			time_use_train += (ed - st);
#endif		

		}
	}
}


#define AccessDegree 1
#define INTER_ACCESS_CYCLE (60ULL << 30) //about 10s
#define LONG_STREAM_PREFETCH_DEGREE 3
#define INTER_PAGE 8

//#define USE_1toMore  //USE_one_map_to_more

struct PPN2VPNS{
        int validNum; // init 0
        volatile  int Number; // init 0
//	std::atomic< int  > Number;
	int delayValidNum; // init 0
//      unsigned long vpn[255]; 

#ifdef USE_1toMore
        vector<unsigned long> vpns;
        vector<unsigned char> valids;
        vector<int> pids;
	vector<unsigned long > free_time;
#endif

	 /*For prefetch*/
        int access_times;
        unsigned long first_access_time;


};
#if 1//#ifdef USE_1toMore
struct PPN2VPNS ppn2vpns[MAXPPN];
#endif
map<unsigned long, int> vpn2prefetch;
map<unsigned long, int> vpn2LSDid;
map<unsigned long, int >ppn2LSDid;

#define MAXLSDPPN 1000
struct LongStreamDetector{
        unsigned long start_vpn;
	unsigned long ppn[MAXLSDPPN];
	int ppn_array_len;
        unsigned long len;
        int confidence;
//      unsigned long last_access_time[AccessDegree];
        unsigned long last_access_time;
	int pid;
        char valid;
};

#define LST_ENTRT_NUM 1600

struct LongStreamDetector longStreamDetectors[1][LST_ENTRT_NUM];
unsigned long long prefetch_hit = 0;
int pre_prefetch_hit_page = 0, prefetch_hit_page = 0;
unsigned long long pre_prefetch_hit = 0;
unsigned long long prefetch_times = 0;

void init_prefetch(){
        int i = 0,j = 0;
	pthread_create(&tid_train, NULL, training_seek, NULL);

	pthread_setaffinity_np(tid_train, sizeof(mask_cpu_2), &mask_cpu_2);
//	pthread_setaffinity_np(tid_train, sizeof(mask_cpu_train), &mask_cpu_train);

        for(i = 0; i < LST_ENTRT_NUM; i++){
		for(j = 0 ;j < 1; j++)
                longStreamDetectors[j][i].valid = 0;
        }

#if 1//#ifdef USE_1toMore
        for(i = 0; i < MAXPPN; i++){
                ppn2vpns[i].access_times = 0;
                ppn2vpns[i].first_access_time = 0;
                ppn2vpns[i].validNum = 0;
                ppn2vpns[i].Number = 0;
        }
#endif
}

void print_prefetch(){
	int i;
	for(i = 0; i < LST_ENTRT_NUM ; i++){
		if(longStreamDetectors[0][i].valid != 0 && longStreamDetectors[0][i].len > 10){
			printf("id = %d,pid = %d, start vpn = %lu, len = %lu, confidence = %d\n",i, longStreamDetectors[0][i].pid  ,longStreamDetectors[0][i].start_vpn, longStreamDetectors[0][i].len, longStreamDetectors[0][i].confidence);
		}
	}
}
extern void InsertLSD(unsigned long vpn, unsigned long now_time, int id, int pid);





int last_use_lsd_id = -1;
int insert_tt = 0;








#define INTER_CYCLE (3ULL << 20)
//#define EXSIT_CYCLE (3ULL << 30)
#define EXSIT_CYCLE (3ULL << 19)
#define MAXVPN2PPN  32


int tmpint = 1;
unsigned long long right_pte_insert = 0, conflict_pte_insert = 0, right_pte_free = 0, conflict_pte_free = 0, error_pte_free = 0;
unsigned long long pre_right_pte_insert = 0, pre_conflict_pte_insert = 0, pre_right_pte_free = 0, pre_conflict_pte_free = 0, pre_error_pte_free = 0;;
int max_len_vector = 0;
int max_valid_vector = 0;


#ifdef USE_1toMore

int InsertPPN(unsigned long ppn, unsigned long vpn, int pid){
	int i = 0;
	int oldest_index = -1, oldest_indexValue1 = -1;
	unsigned long  now_time = get_cycles();
	unsigned long oldest_time = now_time, oldest_time1 = now_time;
	
        for(i = 0; i < ppn2vpns[ppn].Number; i++){
                if(vpn == ppn2vpns[ppn].vpns[i] && pid == ppn2vpns[ppn].pids[i]){
                        break;
                }
		// delete useless pte 
		if(oldest_time > ppn2vpns[ppn].free_time[i] && ppn2vpns[ppn].valids[i] != 1){
			oldest_time =  ppn2vpns[ppn].free_time[i];
			oldest_index = i;
		}
		if(oldest_time1 > ppn2vpns[ppn].free_time[i] && ppn2vpns[ppn].valids[i] == 1){
			oldest_time1 = ppn2vpns[ppn].free_time[i];
			oldest_indexValue1 = i;
		}

        }
        if(i < ppn2vpns[ppn].Number){
                //find
                if(ppn2vpns[ppn].valids[i] == 1){
                        conflict_pte_insert ++;
                        return 2;
                }
                else{
                        ppn2vpns[ppn].valids[i] = 1;
                        ppn2vpns[ppn].validNum ++;
			ppn2vpns[ppn].free_time[i] = now_time;
                        if(ppn2vpns[ppn].validNum > max_valid_vector){
                                        max_valid_vector = ppn2vpns[ppn].validNum;
                        }
                        right_pte_insert ++;
                        return 1;
                }
        }
        else{// not find
//              ppn2vpns[ppn].vpns[i] = vpn;
//              ppn2vpns[ppn].pids[i] = pid;    
//              ppn2vpns[ppn].valids[i] = 1;

		if(ppn2vpns[ppn].vpns.size() >= MAXVPN2PPN && oldest_index != -1 /*&& now_time - oldest_time >= EXSIT_CYCLE */){
			//use oldest and replace it
			ppn2vpns[ppn].vpns[ oldest_index ] = vpn;
			ppn2vpns[ppn].pids[ oldest_index ] = pid;
			if(ppn2vpns[ppn].valids[ oldest_index ] != 1)
				ppn2vpns[ppn].validNum ++;
			if(ppn2vpns[ppn].valids[ oldest_index ] == 2){
				ppn2vpns[ppn].delayValidNum --;
			}
			ppn2vpns[ppn].valids[ oldest_index ] = 1;
			ppn2vpns[ppn].free_time[ oldest_index ] = now_time;
			if(now_time - oldest_time < EXSIT_CYCLE ){
				printf("[Warning]: repalece a delay pte exist %ld cycle\n", now_time - oldest_time);
			}
			right_pte_insert ++;
			return 1;	

		}
		if(ppn2vpns[ppn].vpns.size() >= MAXVPN2PPN && oldest_indexValue1 != -1){
			//replace oldest pte
			ppn2vpns[ppn].vpns[ oldest_indexValue1 ] = vpn;
                        ppn2vpns[ppn].pids[ oldest_indexValue1 ] = pid;
                        if(ppn2vpns[ppn].valids[ oldest_indexValue1 ] != 1)
                                ppn2vpns[ppn].validNum ++;
                        if(ppn2vpns[ppn].valids[ oldest_indexValue1 ] == 2){
                                ppn2vpns[ppn].delayValidNum --;
                        }
                        ppn2vpns[ppn].valids[ oldest_indexValue1 ] = 1;
                        ppn2vpns[ppn].free_time[ oldest_indexValue1 ] = now_time;
                        if(now_time - oldest_time1 < EXSIT_CYCLE ){
                                printf("[Warning]: repalece a pte exist %ld cycle\n", now_time - oldest_time1);
                        }
                        right_pte_insert ++;
                        return 1;
		}

                ppn2vpns[ppn].vpns.push_back(vpn);
                ppn2vpns[ppn].pids.push_back(pid);
                ppn2vpns[ppn].valids.push_back(1);
//		ppn2vpns[ppn].free_time.push_back(0);
		ppn2vpns[ppn].free_time.push_back( get_cycles() );

                ppn2vpns[ppn].validNum ++;
                if(ppn2vpns[ppn].validNum > max_valid_vector){
                        max_valid_vector = ppn2vpns[ppn].validNum;
                }
                if(ppn2vpns[ppn].Number == 255){
        //              printf("255 reach!!!  1 ppn to 255 vpn\n");
                }
                ppn2vpns[ppn].Number ++;
                if(ppn2vpns[ppn].Number > max_len_vector)
                        max_len_vector = ppn2vpns[ppn].Number;
                right_pte_insert ++;
                return 1;
        }
}

int FreePPN(unsigned long ppn, unsigned long vpn, int pid){
        int i = 0;
        for(i = 0; i < ppn2vpns[ppn].Number; i++){
                if(vpn == ppn2vpns[ppn].vpns[i] && pid == ppn2vpns[ppn].pids[i]){
                        break;
                }
        }
        if(i < ppn2vpns[ppn].Number){
                //find
                if(ppn2vpns[ppn].valids[i] == 1){
			ppn2vpns[ppn].free_time[i] = get_cycles();
                        ppn2vpns[ppn].valids[i] = 2;
			ppn2vpns[ppn].delayValidNum ++;
                        ppn2vpns[ppn].validNum --;
                        right_pte_free ++;
                        return 1;
                }
                else{
                        conflict_pte_free ++;
                        return 2;  //free twice ,error exist !!
                }
        }
        else{
//                      conflict_pte_free ++;
                error_pte_free ++;
                return 2; //find no vpn, error!!
        }
}
#endif

unsigned long long real_hit = 0, fake_hit = 0, delay_hit = 0, real_miss = 0;;
unsigned long long pre_real_hit = 0, pre_fake_hit = 0, pre_delay_hit = 0, pre_real_miss = 0;;


#ifdef USE_1toMore
int CheckPPN(unsigned long ppn){
        if(ppn2vpns[ppn].validNum == 0){
		real_miss ++;
		return 0;
		if(ppn2vpns[ppn].delayValidNum != 0){
			int j = 0;
			unsigned long now_time = get_cycles();
			int find_ans = -1;
			for(j = 0 ; j < ppn2vpns[ppn].Number; j ++){
				if(ppn2vpns[ppn].valids[j] == 2){
					if(now_time - ppn2vpns[ppn].free_time[j] > INTER_CYCLE){
						ppn2vpns[ppn].delayValidNum --;
						ppn2vpns[ppn].valids[j] = 0;
					}
					else{
						find_ans = 1;

					}
				}
			}
			if(find_ans = 1){
				delay_hit ++;
				return 2;
			}
			else{
				fake_hit ++;
				return 2;
			}

		}
		

                if(ppn2vpns[ppn].Number == 0){
                        real_miss  ++;
                        return 0;
                }
                else //fake hit
                {
                        fake_hit ++;
                        return 2;
                }
        }
        else //real hit
        {
                real_hit ++;
                return 1;
        }
}
#endif





/*
volatile unsigned long long duration_all = 0;
unsigned long          last_writeptr;
volatile unsigned long *dev_writeptr;
volatile unsigned long *dev_readptr;
unsigned long long kernel_trace_offset = 0;
unsigned long long tagp = 0;
unsigned long *ppn2vpn = NULL;
int *ppn2pid = NULL;
volatile int glb_start_analysis = 0;
volatile unsigned long long total_trace = 0;
volatile unsigned long long none_pte = 0;
unsigned long long last_none_pte = 0;

unsigned long long set_pte_cnt = 0, set_pte_cnt_w = 0;
unsigned long long free_pte_cnt = 0, free_pte_cnt_w = 0;
unsigned long long some_error_trace = 0;
unsigned long long kernel_config_entry = KERNEL_TRACE_CONFIG_ENTRY_ADDR << 20;
unsigned long long kernel_config_end =
        (KERNEL_TRACE_CONFIG_ENTRY_ADDR + KERNEL_TRACE_CONFIG_SIZE * KERNEL_TRACE_SEQ_NUM) << 20;

char free_pt_addr_magic     = 0xfd;
char set_page_table_magic   = 0xec;
char free_page_table_magic  = 0xfc;
char free_page_table_get_clear = 0x22;
char free_page_table_get_clear_full = 0x33;
unsigned long long skip_free_in_set = 0;

unsigned long long redundant_set_pte = 0, redundant_free_pte = 0;
*/

int is_kernel_tag_trace(unsigned long long addr) {
	    return addr >= kernel_config_entry && addr < kernel_config_end;
}



void init_kt_collect(){
	ppn2pid = new int[MAXPPN];
	ppn2vpn = new uint64_t[MAXPPN];
	int i ;
	for(i = 0;i < MAXPPN; i++){
              ppn2vpns[i].validNum = 0;
             ppn2vpns[i].Number = 0;
	      ppn2vpns[i].delayValidNum = 0;
        }
//	init_prefetch_thread();


	int command, wr_cnt;
	command = CMD_BEGIN_KERNEL_TRACE;
        wr_cnt = write(kt_ff, &command, sizeof(int));
        if( wr_cnt != sizeof(int) )
        {
               printf("#### Begin Kernel Trace: write 0 failure!\n");
        }
        else
        {
               printf("$$$$ Being Kernel Trace: write 0 success!\n");
        }

	command = CMD_PT_ADDR_START_TRACE;
	wr_cnt = write(kt_ff, &command, sizeof(int));
	if( wr_cnt != sizeof(int) )
        {
               printf("#### Begin PT Addr Trace: write 0 failure!\n");
        }
        else
        {
               printf("$$$$ Being PT Addr Trace: write 0 success!\n");
        }

	// dump
	command = CMD_DUMP_PAGE_TABLE_TRACE;
        wr_cnt = write(kt_ff,&command,sizeof(int));
        if(wr_cnt != sizeof(int))
	        printf("Begin Dump Page Table Trace: write 0 failure!\n");
        else
                printf("Begin Dump Page Table Trace: write 0 success!\n");
//        sleep(1);

	//start collect pte
	command = CMD_PAGE_TABLE_START_TRACE;
        wr_cnt = write(kt_ff,&command,sizeof(int));

        if(wr_cnt != sizeof(int))
                printf("Begin Page Table Trace: write 0 failure!\n");
        else
                printf("Begin Page Table Trace: write 0 success!\n");
}

void finish_kt_collect(){
	int command, wr_cnt;
	command = CMD_WRITE_KERNEL_TRACE_END_TAG;
        wr_cnt = write(kt_ff,&command,sizeof(int));
    if(wr_cnt != sizeof(int))
        printf("Write Kernel Trace End Tag Error\n");
    else
        printf("Write Kernel Trace End Tag OK\n");
	
	command = CMD_RESET_BUFFER;
    wr_cnt = write(kt_ff,&command,sizeof(int));

    if(wr_cnt != sizeof(int))
        printf("Reset Buffer Error\n");
    else
        printf("Reset Buffer OK\n");

	//stop and clear trace
	 command = CMD_PAGE_TABLE_STOP_AND_CLEAR_TRACE;
    wr_cnt = write(kt_ff,&command,sizeof(int));
    if(wr_cnt != sizeof(int))
        printf("Stop Page Table Trace: write 2 failure!\n");
    else
        printf("Stop Page Table Trace: write 2 success!\n");

    command = CMD_PT_ADDR_STOP_TRACE;
    wr_cnt = write(kt_ff, &command, sizeof(int));
    if( wr_cnt != sizeof(int) )
    {
        printf("#### Stop PT Addr Trace: write 0 failure!\n");
    }
    else
    {
        printf("$$$$ Stop PT Addr Trace: write 0 success!\n");
    }

    //dump the trace to log file
    command = CMD_END_KERNEL_TRACE; //CMD_END_AND_DUMP_TRACE;
    wr_cnt = write(kt_ff,&command,sizeof(int));
    if(wr_cnt != sizeof(int))
        printf("Dump Trace: write 3 failure!\n");
    else
        printf("Dump Trace: write 3 success!\n");

    printf("[Kernel_Trace]  Collect Kernel Trace OK.\n");


}


int finish_kt_analysis = 1;

extern void get_kt_trace(void);
int analysis_kt_buffer(void  *arg){
        unsigned int seq_no,r_w;
        unsigned long paddr;
        unsigned long long timer;
        unsigned int pid;
        unsigned long ppn;
        unsigned long vpn;
        uint64_t val;
        char magic;

        while(1){
                if(finish_kt_analysis == 0)
                {
                        printf(" analysis_kt_buffer finish , kt_hmtt_reading_addr = %lx , kt_hmtt_writing_addr = %lx\n",kt_hmtt_reading_addr,kt_hmtt_writing_addr);
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
                        if(glb_start_analysis == 0)
                        {
                                continue ;

                        }
//			if (strncmp(kt_ch_tmp, "$$$$$$$$$$$$$", 13) == 0) {
//				return ;
//                        } 

                        int tag_seq = (int)(*(char*)(kt_ch_tmp + 13));
			magic =  (*(char*)(kt_ch_tmp));
			if(magic == set_page_table_magic){

				tagp += 1;
				pid = (*(int*)((kt_ch_tmp + 1)));
				tagp += 4;
				val = (*(uint64_t*)(kt_ch_tmp + 5));
				ppn = val & 0xffffff;
				vpn = (val >> 24) & 0xffffffffff;
				tagp+= 9;


				if (ppn >= MAXPPN) {
                        	        printf("invalid ppn\n");
				}
				if(pid == now_pid){
					goto skip;
				}
				set_pte_cnt ++;


#ifdef USE_1toMore
					tmpint = InsertPPN(ppn, vpn, pid);
#endif
				if (using_pid != -1 && pid == using_pid){
					ppn2pid[ppn] = pid;           
                                        ppn2vpn[ppn] = vpn; 					
				}
				else if(using_pid == -1){
					ppn2pid[ppn] = pid;
                                        ppn2vpn[ppn] = vpn;
				}
					ppn2pid[ppn] = pid;
                                        ppn2vpn[ppn] = vpn;
					ppn2num[ppn] = 0;
//                                printf("update set vpn = %llu, vaddr = %llx, pid = %d\n",  ppn2vpn[ppn], (ppn2vpn[ppn] << 12) | (record.paddr & 0xfff), pid);
				unsigned long tmp = (ppn) | (vpn << 24);
				store_to_trace_store_buff(ppn,0);

				if(using_pid == pid && vpn  >= (moniter_base_address >> 12))
	                        {
	                                monitor_num_pte ++;
//					unsigned long tmp = (ppn) | (vpn << 24);
					/*store_to_trace_store_buff(tmp,0);	*/
	                                int tmp_index = ( vpn  - (moniter_base_address >> 12)) / 100000;
	                                if(tmp_index < 200)
					{
	                                        monitor_num_split_pte[tmp_index] ++;
						if(  monitor_num_split_pte[tmp_index] % 100000 == 0  ){
							printf("index = %d, num = %d\n", tmp_index, monitor_num_split_pte[tmp_index]);
						}
					}
					if( vpn == (moniter_base_address >> 12) + 20000)
						printf("update set vpn = %llu, vaddr = %llx, ppn = 0x%lx , pid = %d, id = %d\n",  ppn2vpn[ppn], (ppn2vpn[ppn] << 12) | (record.paddr & 0xfff), ppn  ,pid,  analysis_kt_num);
	                        }

				

				if(pid == using_pid )// && ppn2vpn[ppn] >= 34358787532 && ppn2vpn[ppn]  <= 34359705042)
//				if( ppn2vpn[ppn] >= 34358787532 && ppn2vpn[ppn]  <= 34359705042)
//				if(ppn2vpn[ppn] >= 34358787532  && ppn2vpn[ppn]  <= 34359705042)
                        	{
#ifdef PRINT_MSG
				printf("update set vpn = %llu = 0x%lx, vaddr = %llx, ppn = 0x%lx , pid = %d, id = %d\n",  ppn2vpn[ppn] ,ppn2vpn[ppn], (ppn2vpn[ppn] << 12) | (record.paddr & 0xfff), ppn  ,pid,  analysis_kt_num);
#endif

				;
//                                printf("update set vpn = %llu, vaddr = %llx, pid = %d\n",  ppn2vpn[ppn], (ppn2vpn[ppn] << 12) | (record.paddr & 0xfff), pid);

                        	}
//				vpn2ppn[vpn] = ppn;
			}
			else if(magic == free_page_table_get_clear || magic == free_page_table_get_clear_full || magic == free_page_table_magic){
				tagp += 1;
                                pid = (*(int*)((kt_ch_tmp + 1)));
                                tagp += 4;
                                val = (*(uint64_t*)(kt_ch_tmp + 5));
                                ppn = val & 0xffffff;
                                vpn = (val >> 24) & 0xffffffffff;
				if(pid == now_pid){
					goto skip;
				}
				free_pte_cnt ++;
			//	if(pid == 6350)
#ifdef USE_1toMore
				tmpint = FreePPN(ppn, vpn, pid);
#endif
                                tagp+= 9;
				if(using_pid == pid && vpn  >= (moniter_base_address >> 12))
	                        {
//	                                monitor_num_pte ++;
					unsigned long tmp = (ppn) | (vpn << 24);
					/*store_to_trace_store_buff(tmp,1);	*/
				}
                                if (ppn >= MAXPPN) {
                                        printf("invalid ppn\n");
                                }
				unsigned long tmp = (ppn) | (vpn << 24);
				store_to_trace_store_buff(ppn,1);

				if (using_pid != -1 && pid == using_pid){
	                                ppn2pid[ppn] = 0;
	                                ppn2vpn[ppn] = 0;
				}
				else if(using_pid == -1){
	                                ppn2pid[ppn] = 0;
	                                ppn2vpn[ppn] = 0;
				}
	                        ppn2pid[ppn] = 0;
	                        ppn2vpn[ppn] = 0;

#ifdef MONITOER_PREFETCH
//					new_ppn[ppn].isprefetch=0;
					new_ppn[ppn].isprefetch = (new_ppn[ppn].isprefetch & 2) ; //clear prefetch flag
					new_ppn[ppn].prefetch_time=0;
#endif

					ppn2num[ppn] = 0;
			}
			else{
				printf("Unknow maigc = %x\n", magic);
			}
skip:

			get_kt_trace();
        }
        return 0;


}

unsigned long max_kt_buffer_offst = 0;

void get_kt_trace(){
	analysis_kt_num ++;

        //memcpy from   to kt_ch_tmp
        if(start_cycle == 0){
		while(*dev_readptr == *dev_writeptr){}
                if(*dev_readptr + 13 < buffer_size){
                        memcpy(kt_ch_tmp, p_kernel_trace_buf + *dev_readptr , 13);
                        *dev_readptr += 13;
                }
                else{
                        int left = buffer_size - *dev_readptr;
                        memcpy(kt_ch_tmp, p_kernel_trace_buf + *dev_readptr, left);
                        memcpy(kt_ch_tmp + left, p_kernel_trace_buf, 13 - left);
                        *dev_readptr = 13 - left;
                }
                tagp +=13;
        }
        else{
		while(*dev_readptr == *dev_writeptr)
		{

		}
                if(*dev_readptr + 14 < buffer_size){
#if 0
			if(*dev_readptr < *dev_writeptr){
				unsigned long  tmp_in = ( *dev_writeptr - *dev_readptr );
				if(tmp_in > 30000){
//					printf("left analysis kt trace = %lu\n", tmp_in);
				}

				if(max_kt_buffer_offst < tmp_in)
					max_kt_buffer_offst = tmp_in;
			}
#endif
                        memcpy(kt_ch_tmp, p_kernel_trace_buf + *dev_readptr , 14);
                        *dev_readptr += 14;
                }
                else{
			
                        unsigned long left = buffer_size - *dev_readptr;
                        memcpy(kt_ch_tmp, p_kernel_trace_buf + *dev_readptr, left);
                        memcpy(kt_ch_tmp + left, p_kernel_trace_buf, 14 - left);
                        *dev_readptr = 14 - left;
                }
                tagp += 14;
        }
}
 int print_times = 1;

void init_page_table(){
	kernel_trace_offset = 0;
	tagp = 0;

	buffer_size = KERNEL_TRACE_BUF_SIZE << 20;
	p_kernel_trace_buf = p_kernel_trace + (KERNEL_TRACE_WRRD_PTR_SIZE << 20);
	dev_writeptr = (unsigned long*)p_kernel_trace;
	dev_readptr = (unsigned long*)(dev_writeptr + 1);
        last_writeptr = *dev_writeptr;
	printf("p_kernel_trace = 0x%lx, p_kernel_trace_buf = 0x%lx  \n",p_kernel_trace ,p_kernel_trace_buf);
	printf("p_kernel_trace = 0x%lx, dev_writeptr = %lu , dev_readptr = %lu\n",p_kernel_trace, *dev_writeptr, *dev_readptr);


	unsigned long long dump_pte_trace = 0;
        unsigned long long set_pt_num = 0,free_pt_num = 0, set_page_table_num = 0, free_page_table_num = 0;
	unsigned int pid;
        unsigned long ppn;
        unsigned long vpn;
        uint64_t val;


	printf("init_page_table start \n");
	//默认dump时，page不会导致buffer溢出
	int end = 0;
	while(end == 0){
		while(*dev_readptr + 13 < *dev_writeptr){
			if (strncmp(p_kernel_trace_buf + *dev_readptr , "@@@@@@@@@@@@@", 13) == 0) {
				tagp += 13;
				printf("start collect trace  flag, tagp = %lu \n", tagp);
				*dev_readptr += 13;
				break;
			}
		}
		while(*dev_readptr + 13 < *dev_writeptr){
			if (strncmp(p_kernel_trace_buf + *dev_readptr , "#############", 13) == 0) {
                                tagp += 13;
                                printf("start dump page talbe flag, tagp = %lu \n", tagp);                          
                                *dev_readptr += 13;
                                break;
                        }
		}

		
		while( *dev_readptr + 13 < *dev_writeptr ){
			get_kt_trace();
//			if (strncmp(p_kernel_trace_buf + *dev_readptr, "&&&&&&&&&&&&&", 13) == 0) {
			if (strncmp(kt_ch_tmp, "&&&&&&&&&&&&&", 13) == 0) {
				tagp += 13;
				printf("end dump page talbe flag, ,dump_pte_trace = %llu, tagp = %lu\n",dump_pte_trace,tagp);
				start_cycle = 1;
                                get_kt_trace();
				glb_start_analysis = 1;
				max_kt_buffer_offst = 0;
				total_trace = 0;
//				*dev_readptr += 3;
				// print some msg here!
				printf("set_pt_num = %llu,free_pt_num = %llu, set_page_table_num = %llu, free_page_table_num = %llu\n",set_pt_num,free_pt_num, set_page_table_num, free_page_table_num);
				end = 1;
				break;
			}

		//	char tmp_magic = *(p_kernel_trace_buf + *dev_readptr);
			char tmp_magic = *(kt_ch_tmp);
			if(tmp_magic == set_page_table_magic){
		                set_page_table_num ++;
		        }
		        else if(tmp_magic == free_page_table_magic){
		                free_page_table_num ++;
		        }
		        else {printf("****************************error !!!\n");}
			pid = (*(int*)((kt_ch_tmp  + 1)));;
			val = (*(uint64_t*)(kt_ch_tmp + 5 ));			
//			pid = (*(int*)(( *(p_kernel_trace_buf + *dev_readptr)  + 1)));;
///		        val = (*(uint64_t*)(*(p_kernel_trace_buf + *dev_readptr)  + 5));
		        ppn = val & 0xffffff;
		        vpn = (val >> 24) & 0xffffffffff;
			ppn2pid[ppn] = pid;
		        ppn2vpn[ppn] = vpn;
			unsigned long tmp = (ppn) | (vpn << 24);
                        store_to_trace_store_buff(ppn,0);

#ifdef USE_1toMore
                        tmpint = InsertPPN(ppn, vpn, pid);
#endif
			
//			*dev_readptr += 13;
			dump_pte_trace ++;
		}
	}
	printf("init page table end\n");

}

volatile unsigned long long all_has_read = 0;
volatile int times_use = 0;


//#define Process_count 1024288
#define Process_count 512144
// buffer 	      6268928

//#define PRINT_ROUND 500
#define PRINT_ROUND 50000


//#define MIN_PROCESS 120000
#define MIN_PROCESS 240000


unsigned long start_time = 0, end_time = 0;
int real_analysis_num = 0;
unsigned long long max_read_len = 0;

//#define SPLITE_ADD_glb_reading_addr

unsigned long s1,e1;
int min_process_trace = 1;

//#define SKIP_TRACE



void analysis_trace_buff(unsigned long long start_addr, unsigned long long read_len){
	// 把之前开头的存下来
//	return ;
//	if(read_len > 20000)
//	 printf("start_addr = 0x%lx, read_len = %lu\n",start_addr, read_len);
	if(max_read_len < read_len)
		max_read_len = read_len;


	all_has_read += read_len;
	times_use ++;
	if(read_len > 500000){
		;
	}
	if(read_len > 500000){
		s1 = get_cycles();
	}
	if(times_use % PRINT_ROUND == PRINT_ROUND - 1)
//	if(now_seq % 200000 == 19999 )
	{
//		printf( "read_len = %lu\n", read_len  );	
//		start_time = get_cycles();
//		real_analysis_num = 0;

	}

	int flag_skip = 0;

	if(read_len > Process_count * 2 / 3){
//	if(glb_start_analysis == 1 && read_len > Process_count * 2){
//		printf("start_addr = 0x%lx, read_len = %lu\n",start_addr, read_len);
		flag_skip = 1;
		min_process_trace = MIN_PROCESS / 6;
	}
//	flag_skip = 0;


	//解决有剩的，但是新的和剩的无法拼接成一个trace
	if(ht_left + read_len < 6){
		memcpy(ht_ch_tmp + ht_left, start_addr, read_len);
		ht_left += read_len;
#ifdef SPLITE_ADD_glb_reading_addr
		glb_reading_addr += read_len;
#endif
		return ;
	}



	unsigned long long new_start_addr = start_addr, new_read_len = read_len;
	if(ht_left != 0 && ht_left < 6 ){
		memcpy(ht_ch_tmp + ht_left, start_addr, 6 - ht_left);
		new_start_addr += ( 6 - ht_left);
		new_read_len -= (6 - ht_left);
		// analysis first hmtt trace
		if(flag_skip == 0)
		{
			real_analysis_num ++;
			analysis_single_trace(ht_ch_tmp);
		}


#ifdef SPLITE_ADD_glb_reading_addr
		glb_reading_addr += (6 - ht_left);
#endif
		ht_left = 0;
	}

	unsigned long long i = 0;
#ifdef SKIP_TRACE
	int num_trace_left = new_read_len / 6;	



	int skip_trace = new_start_addr + 6 * (num_trace_left - min_process_trace) ;
	if(flag_skip == 1){

		for(i = num_trace_left - min_process_trace ; i < new_read_len / 6; i ++){
			real_analysis_num ++;
	                analysis_single_trace(new_start_addr + i * 6);
#ifdef SPLITE_ADD_glb_reading_addr
        	        glb_reading_addr += 6;
#endif

		}
	}
	else{
		for(i = 0; i < new_read_len / 6; i ++){
			real_analysis_num ++;
                        analysis_single_trace(new_start_addr + i * 6);
			
#ifdef SPLITE_ADD_glb_reading_addr
                        glb_reading_addr += 6;
#endif
		}
	}


#else
	int inter = new_read_len / 6 / min_process_trace; 
	for(i = 0; i < new_read_len / 6; i ++){
	// analysis each hmtt trace , 6B per trace
		if(flag_skip == 0)
		{
			real_analysis_num ++;
			analysis_single_trace(new_start_addr + i * 6);
		}
//		else if(i > new_read_len / 6 - 130000){
		else if(i % inter == 0){
			real_analysis_num ++;
			analysis_single_trace(new_start_addr + i * 6);
		}
#ifdef SPLITE_ADD_glb_reading_addr
		glb_reading_addr += 6;
#endif
	}

#endif



//	if(flag_skip == 1)
//		printf("all = %d, sample every %d trace \n", new_read_len / 6 ,inter);

	if(new_read_len % 6 != 0){
		ht_left = new_read_len % 6;
		memcpy(ht_ch_tmp, new_start_addr +  new_read_len / 6 * 6, ht_left );
#ifdef SPLITE_ADD_glb_reading_addr
		 glb_reading_addr += ht_left;
#endif
		//warning!! can not do analusis here!!!!
	}

#ifndef SPLITE_ADD_glb_reading_addr
	glb_reading_addr += read_len;
#endif

	if(read_len > 500000){
                e1 = get_cycles();
//		printf("round %d,  use %lu cycles , every 6 Bytes use %.3lf cycles \n", times_use, e1 - s1, (double)(e1 - s1) / (double)read_len);
//		printf("read_len = %lu\n", read_len);
        } 


	if(times_use % PRINT_ROUND == (PRINT_ROUND - 1))
	{
		end_time = get_cycles();
//		printf("round %d,  use %lu cycles , per Byte use %lu cycles \n", times_use, end_time - start_time, (end_time - start_time) / read_len);
		unsigned long tmp_ans = (end_time - start_time) / real_analysis_num;
//		if(tmp_ans > 6 )
		printf("[%8lu : %8lu] round %d,  use %lu cycles , every 6 Bytes use %.3lf cycles \n", get_lru_size(), memory_buffer_start_addr[2],times_use, end_time - start_time, (double)(end_time - start_time) / (double)real_analysis_num);
		start_time = get_cycles();
		real_analysis_num = 0;
		
	}
}


unsigned char tmphmtt[6] = {0};

void analysis_single_trace(char *trace_start){
	unsigned long long tmp;
	unsigned int seq_no,r_w;
	unsigned long paddr;
        unsigned long long timer;
	unsigned int pid;
        unsigned long ppn;
        unsigned long vpn;
        uint64_t val;
        char magic;
/*
	if(trace_start >= memory_dev_addr && trace_start < (memory_dev_addr + DMA_BUF_SIZE - 20))
	{
		tmp = (unsigned long long)( *(unsigned long long*)trace_start);
		seq_no = (unsigned int) ((tmp >> 40) & 0xffU);
	        timer  = (unsigned long long)((tmp >> 32) & 0xffULL);
	        r_w    = (unsigned int)((tmp >> 31) & 0x1U);
	        paddr   = (unsigned long)(tmp & 0x7fffffffUL);
	}
	else
*/
	{
		memcpy( tmphmtt , trace_start  ,6);
		seq_no = (unsigned int) ( *((unsigned char*)(tmphmtt + 5))  );
		timer  = (unsigned long long)*((unsigned char*)(tmphmtt + 4) );
		r_w  =  (unsigned int)(*((unsigned int*)(tmphmtt) ) >> 31 );
		paddr  =  (unsigned long)(*((unsigned int*)(tmphmtt) ) & 0x7fffffffUL );
/*
		seq_no = (unsigned int) ( *((unsigned char*)(trace_start + 5))  );
		timer  = (unsigned long long)*((unsigned char*)(trace_start + 4) );
		r_w  =  (unsigned int)(*((unsigned int*)(trace_start) ) >> 31 );
		paddr  =  (unsigned long)(*((unsigned int*)(trace_start) ) & 0x7fffffffUL );
*/
	}

	paddr   = (unsigned long)(paddr << 6);
	if (paddr >= (2ULL << 30)) {
	        paddr += (2ULL << 30);
        }
	if(paddr >= 0x800000000)
		return 0;
/*
	seq_no = (unsigned int) ((tmp >> 40) & 0xffU);
	timer  = (unsigned long long)((tmp >> 32) & 0xffULL);
        r_w    = (unsigned int)((tmp >> 31) & 0x1U);
        paddr   = (unsigned long)(tmp & 0x7fffffffUL);
        paddr   = (unsigned long)(paddr << 6);
*/
//	record.paddr = paddr;
//	record.rw = r_w;
	if (paddr == 0 && timer == 0){
		//invalid trace
		duration_all += 256;
		return ;
	}
	duration_all += timer;
//	record.tm = duration_all;
	if(r_w == 0){ 
		printf("addr = ox%lx, r_w = %d\n", paddr, r_w);
		return ;
	}
	ppn = paddr >> 12;
	if(ppn >= MAXPPN)
		return 0;
//#if 0
#ifdef MONITOER_PREFETCH
//		if(new_ppn[ppn].isprefetch == 1){
		if( (new_ppn[ppn].isprefetch & 1) == 1){
                        new_ppn[ppn].isprefetch = (new_ppn[ppn].isprefetch & 2) ; //clear prefetch flag
			if(new_ppn[ppn].prefetch_time == 0){
				printf("prefetch time = 0 error!!!\n");
			}
			if(max_latency < duration_all - new_ppn[ppn].prefetch_time)
				max_latency = duration_all - new_ppn[ppn].prefetch_time;
			latency_all  += (duration_all - new_ppn[ppn].prefetch_time);
                        ac_counter++;
                }
#endif

//	filter_lru( paddr, duration_all  );
//	return 0;
//	if( insert_entry( ppn  , duration_all) == 1){
//		store_to_tb(ppn, duration_all );
//	}

//	new_ppn[ ppn ].num ++;
//	store_to_trace_store_buff(ppn, 3);
//	return 0;

#if 0
	if(duration_all - new_ppn[ppn].timer >= (1ULL << 21)){
		new_ppn[ ppn ].num = 0;
		new_ppn[ppn].timer = duration_all;
		new_ppn[ppn].isprefetch = new_ppn[ppn].isprefetch & 1;  // clear extracted flag
	}
	if(new_ppn[ppn].num  % 18 == 16){
		new_ppn[ppn].num = 0;
		new_ppn[ppn].timer = duration_all;
		if( ( new_ppn[ppn].isprefetch & 2 ) != 0 )  // check extracted flag ,have extracted hot page
		{
			; //skip	
		}else{
			new_ppn[ppn].isprefetch = new_ppn[ppn].isprefetch | 2; 
			store_to_tb(ppn, duration_all);
		}

	}
	return 0;
#endif	

//	filter_table( paddr, duration_all );

//	multi_filter_table(paddr, duration_all);
	filter_check(paddr, duration_all); // for prefetch and traditional eviction
	return 0;
//	insert_entry(paddr >> 12, duration_all);
//	return 0; return ;

/*
	if( (new_ppn[ppn].num ) % 33 == 32){
//	if( (new_ppn[ppn].num ) % 9 == 8){
		if(duration_all - new_ppn[ppn].timer >= (1ULL << 21)){
			store_to_pb(ppn, using_inter_page);
//			 store_to_tb(ppn, duration_all);
		}
		new_ppn[ppn].timer = duration_all;
		new_ppn[ppn].num = 0;
	}
	return ;
*/

//	if( (new_ppn[ppn].num ) % 3 == 2){
//	if( (new_ppn[ppn].num ) % 9 == 8){
#if 0
	if( (new_ppn[ppn].num ) % 32 == 31){
#ifdef USETIMER
		if(duration_all - new_ppn[ppn].timer >= (1ULL << 22)){
			store_to_pb(ppn, using_inter_page);
//			store_to_pb(ppn, 2);
			
		}
		new_ppn[ppn].timer = duration_all;
#endif
		new_ppn[ppn].num = 0;
	}
	return ;
#endif
#if 0 
	if (is_kernel_tag_trace(paddr) ){
		//This is a hmtt kernel tag trace, we should read kernel trace buffer now
		//only anaysis read trace
		if(glb_start_analysis == 1) return ;
                if(r_w == 0 ) return;
                else{
                // copy trace to KT buffer
//                        store_to_kt_buffer(&record.paddr, 1);
//                        return ;
                }
//		unsigned long addr =  record.paddr;
		unsigned long addr =  paddr;
		int kernel_trace_seq = (addr - kernel_config_entry) / (KERNEL_TRACE_CONFIG_SIZE << 20);
		if (kernel_trace_seq >= KERNEL_TRACE_SEQ_NUM) {
			printf("#### Invalid kernel trace seq:addr=0x%llx, seq=%d\n \n", addr, kernel_trace_seq);
		}
		unsigned long long kernel_trace_seq_entry = addr - kernel_config_entry - kernel_trace_seq * (KERNEL_TRACE_CONFIG_SIZE << 20);
                int kernel_trace_tag = (kernel_trace_seq_entry) / TAG_ACCESS_SIZE;
                if((kernel_trace_seq_entry) % TAG_ACCESS_SIZE != 0)
		{
        //                        error_aligning ++;
                               // continue;
                               return ;;
                }

                        if (kernel_trace_tag >= TAG_MAX_POS) {
                                printf( "#### Invalid kernel_trace_tag:addr=0x%llx,tag=%d\n", addr, kernel_trace_tag);
                               // exit(-1);
                        }
                        int hmtt_kt_type = ((kernel_trace_seq_entry) % TAG_ACCESS_SIZE) / TAG_ACCESS_STEP;
                        if (hmtt_kt_type > 1) {
                                printf( "#### Invalid hmtt_kt_type:addr=0x%llx,tag=%d,type=%d\n", addr, kernel_trace_tag,
                                hmtt_kt_type);
                                //exit(-1);
                        } else if (hmtt_kt_type == 1);

                        if (glb_start_analysis == 0 && kernel_trace_tag == DUMP_PAGE_TABLE_TAG) {
                                printf("find DUMP_PAGE_TABLE_TAG in hmtt trace \n");
                                glb_start_analysis = 1;
                                printf("dump page done.\n");
                                printf("trace init done.\n");
//                                continue;
                                return ;
                        }
			if(glb_start_analysis == 0)
                        {
//                                continue ;
                              return ;

                        }
	}
	else if(glb_start_analysis == 1){
#endif
//			total_trace ++;
		ppn = paddr >> 12;
		if(ppn > MAXPPN)
		{
			if(print_times == 1){
			printf("invalid ppn in normol hmtt trace! ppn = %llu\n ",ppn);
			print_times = 0;
			}
//                        exit(-1);

			return ;
		}
		unsigned long long pmd_ppn =(ppn >> 8);


#ifdef USING_NUM_PREFETCH
		new_ppn[ppn].num ++;
#endif

//		filter_check(paddr, duration_all);
  //              return 0;

#if 0
		if(ppn2pid[ppn] == using_pid ){
	                if(ppn2pid[ppn] == using_pid && (ppn2vpn[ppn] == 34359574635 || ppn2vpn[ppn] ==  34359574635 + 31 || ppn2vpn[ppn] ==  34359574635 + 62 || ppn2vpn[ppn] ==  34359574635 + 93 || ppn2vpn[ppn] ==  34359574635 + 124  ))
			{
                              printf("*** access vpn = %llu, vaddr = %llx, pid = %d , r_w = %d,cache line offset = %llu\n",  ppn2vpn[ppn], (ppn2vpn[ppn] << 12) | (paddr & 0xfff), ppn2pid[ppn], r_w, (paddr & 0xfff) / 64   );
			}

		}
#endif 

		{

#ifdef USING_NUM_PREFETCH
//			if(ppn2num[ppn] % 13 == 0)
			if( (new_ppn[ppn].num ) % 9 == 8)
//			if(ppn2pid[ppn] != 0 &&  ppn2pid[ppn] == using_pid)
			{
//				store_to_pb(ppn, using_inter_page);
#if 1
#ifdef USETIMER
				if(duration_all - new_ppn[ppn].timer >= (1ULL << 27))  //2GHZ  3 seconds
				{
					store_to_pb(ppn, using_inter_page);
					new_ppn[ppn].timer = duration_all;
				}
				else{
					new_ppn[ppn].timer = duration_all;
				}
#endif
#endif
//				store_to_pb(ppn, using_inter_page);
				new_ppn[ppn].num = 0;
				
//				store_to_pb(ppn2vpn[ppn] + 800, ppn2pid[ppn]);
//				store_to_pb(ppn2vpn[ppn] + 800, ppn);
				if(prefetch_flag > 0){
					prefetch_flag --;
					printf("prefetch pid = %d, vpn = %lu\n", ppn2pid[ppn], ppn2vpn[ppn]);

				}
//				if( prefetch_bit[ppn] == 0  ){
//					prefetch_bit[ppn] = 1;
//					store_to_pb(ppn, using_inter_page);
//				store_to_tb(ppn2vpn[ppn]);
//				}
//				if(prefetch_bit[ppn] == 0){
//					prefetch_bit[ppn] = 1;
//					store_to_pb(ppn, using_inter_page);
//				}
//				store_to_tb(ppn, duration_all);
			}
#endif
		}
}




//check the writing pointer to see whether overflow happens.

void *check_overflow()
{
	//maintain the writing_addr
	
	unsigned long long  writing_addr;
	unsigned long long  last_writing_addr = DMA_BUF_ADDR;

	gettimeofday(&tvpre_writing,&tz);
	tvfirst_writing = tvpre_writing;

	writing_addr = *ptr_writing_addr;
	
	while(writing_addr < 0x8000000000000000)
	{
		if(writing_addr != last_writing_addr)
		{	//update the glb_writing_addr;
			gettimeofday(&tvafter_writing,&tz);
			writing_size = (writing_addr - last_writing_addr + DMA_BUF_SIZE) % DMA_BUF_SIZE;
			tv_writing = (double)(tvafter_writing.tv_sec - tvpre_writing.tv_sec) + (double)(tvafter_writing.tv_usec - tvpre_writing.tv_usec)/1000000;
			RT_write_report(writing_size);
			
			last_writing_addr = writing_addr;
			glb_writing_addr += writing_size;
			gettimeofday(&tvpre_writing,&tz);
			
			if(glb_writing_addr > glb_reading_addr + DMA_BUF_SIZE)
			{
				overflow ++;
				printf("glb_writing_addr %10.2f, glb_reading_addr %10.2f\n",(double)glb_writing_addr/1024/1024,(double)glb_reading_addr/1024/1024);
				pthread_exit(NULL);
				//overflow ++;
				//printf("glb_writing_addr %10.2f, glb_reading_addr %10.2f\n",(double)glb_writing_addr/1024/1024,(double)glb_reading_addr/1024/1024);
				//glb_writing_addr -= DMA_BUFFER_TOTAL_SIZE;
				//printf("glb_writing_addr %10.2f, glb_reading_addr %10.2f\n",(double)glb_writing_addr/1024/1024,(double)glb_reading_addr/1024/1024);
			//	printf("\n%d\n",overflow);
			}
		}
//		usleep(10000);//10ms,it will take at least 80ms to fillin a 64MB segment at the speed of 800MB/s
		usleep(1000);
		writing_addr = *ptr_writing_addr;
	}
	
	gettimeofday(&tvafter_writing,&tz);
	tvlast_writing = tvafter_writing;
	stopt_addr = writing_addr - 0x8000000000000000;
	writing_size = (stopt_addr - last_writing_addr + DMA_BUF_SIZE) % DMA_BUF_SIZE;
	tv_writing = (double)(tvafter_writing.tv_sec - tvpre_writing.tv_sec) + (double)(tvafter_writing.tv_usec - tvpre_writing.tv_usec)/1000000;
	RT_write_report(writing_size);
	
	glb_writing_addr += writing_size;
	
	if(glb_writing_addr > glb_reading_addr + DMA_BUF_SIZE)
	{
		overflow++;
		pthread_exit(0);
		//overflow ++;
		//printf("glb_writing_addr %10.2f, glb_reading_addr %10.2f\n",(double)glb_writing_addr/1024/1024,(double)glb_reading_addr/1024/1024);
		//glb_writing_addr -= DMA_BUFFER_TOTAL_SIZE;
		//printf("glb_writing_addr %10.2f, glb_reading_addr %10.2f\n",(double)glb_writing_addr/1024/1024,(double)glb_reading_addr/1024/1024);
	}
}


unsigned long long has_reading_size = 0;


void store_to_trace_store_buff(unsigned long vpn, unsigned char flag){
#ifndef STORE_FILE
	return ;
#endif
#ifdef STORE_FILE
#endif
	int tmp_write_index = store_index.fetch_add(1);
//	if(  *trace_store_writeptr  >  MALLOC_TAG_SIZE - ( 1ULL  << 20 ))
	if(  tmp_write_index * 9  >  MALLOC_TAG_SIZE - ( 1ULL  << 20 ))
	{
		printf("store_to_trace_store_buff!! \n");
		return ;
	}


	*(unsigned long *)( kt_hmtt_p_kernel_trace_buf + tmp_write_index * 9 ) = vpn;
	*(unsigned char *)( kt_hmtt_p_kernel_trace_buf + tmp_write_index * 9 + 8 ) = flag;


//	*(unsigned long *)( kt_hmtt_p_kernel_trace_buf + *trace_store_writeptr ) = vpn;

	*trace_store_writeptr += (sizeof(unsigned long) + 1);

}


void store_hmtt_trace(){
#ifndef STORE_FILE
	return ;
#endif	
	
	if((fp = open(filename,flag,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0)
	{	
		printf("Open file %s error\n", filename);
		return 0;
	}
	printf("Start to store hmtt trace \n ");
	printf("......\n");
	

	int tmp_write_index = store_index.fetch_add(1);

	write(fp, kt_hmtt_p_kernel_trace_buf,  tmp_write_index * 9 );
	close(fp);
	

	return ;
}


void *store_to_disk()
{
	unsigned long long tmp;
	int i = 0;

	unsigned long long  writing_addr;
	reading_addr = DMA_BUF_ADDR - 1;
	
	writing_addr = *ptr_writing_addr;

	unsigned long long last_writing_a = DMA_BUF_ADDR;
	unsigned long long writing_a = *ptr_writing_addr;
	unsigned long long writing_s = (writing_a - last_writing_a + DMA_BUF_SIZE) % DMA_BUF_SIZE;


	//maintain the reading_addr
	while(writing_addr < 0x8000000000000000)
	{
//		if(has_reading_size > 10240000){
		if(has_reading_size > 1024000000){
			gettimeofday(&tvafter_reading,&tz);
			writing_a = *ptr_writing_addr;
			writing_s = (writing_a - last_writing_a + DMA_BUF_SIZE) % DMA_BUF_SIZE;
			tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
//			RT_read_report(has_reading_size);
			RT_read_and_write(has_reading_size, writing_s);
			last_writing_a = writing_a;
			has_reading_size = 0;
			gettimeofday(&tvpre_reading,&tz);
			
		}
		

		if(writing_addr == reading_addr + 1)
		{
//			usleep(10000);
			writing_addr = *ptr_writing_addr;
			continue;
		}
		else if(writing_addr > reading_addr + 1)
		{
//			gettimeofday(&tvpre_reading,&tz);
			tmp = writing_addr;
			reading_size = writing_addr - reading_addr - 1;
#ifdef STORE_TRACE			
			if((i = WRITE( fp,phys_to_virtual(reading_addr)+1, reading_size)) != reading_size)
				printf("\n  #a:WARNINGing: write error!,%d,%d,%d\n",i,reading_size,errno);
#endif
#ifndef USE_PROCESS_PTHREAD
			analysis_trace_buff(phys_to_virtual(reading_addr)+1, reading_size);
#else
			process_monitor_seek(phys_to_virtual(reading_addr)+1, reading_size, 0 );
#endif
			has_reading_size += reading_size;

//			gettimeofday(&tvafter_reading,&tz);
//			tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
//			RT_read_report(reading_size);

//			glb_reading_addr += reading_size;
		}
		else
		{
//			gettimeofday(&tvpre_reading,&tz);
			tmp = DMA_BUF_ADDR;
			reading_size = DMA_BUF_ADDR + DMA_BUF_SIZE - reading_addr - 1;
#ifdef STORE_TRACE
			if((i = WRITE( fp,phys_to_virtual(reading_addr)+1, reading_size)) != reading_size)
				printf("\n  #b:WARNINGing: write error!,%d,%d\n",i,errno);
#endif
#ifndef USE_PROCESS_PTHREAD
			analysis_trace_buff(phys_to_virtual(reading_addr)+1, reading_size);
#else
//			printf("virtual addr = 0x%lx, len = %d\n", phys_to_virtual(reading_addr)+1, reading_size);
			process_monitor_seek(phys_to_virtual(reading_addr)+1, reading_size, 0);
#endif
			has_reading_size += reading_size;

//			gettimeofday(&tvafter_reading,&tz);
//			tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
//			RT_read_report(reading_size);

//			glb_reading_addr += reading_size;
			
			//if((writing_addr = *ptr_writing_addr) && (writing_addr != DMA_BUFFER_ADDRESS))
			{
//				gettimeofday(&tvpre_reading,&tz);
				tmp = writing_addr;
				reading_size = writing_addr - DMA_BUF_ADDR;

#ifdef STORE_TRACE
				if((i = WRITE( fp,phys_to_virtual(DMA_BUF_ADDR), reading_size)) != reading_size)
					printf("\n  #c:WARNINGing: write error!,%d,%d\n",i,errno);
#endif
#ifndef USE_PROCESS_PTHREAD
				analysis_trace_buff(phys_to_virtual(DMA_BUF_ADDR), reading_size);
#else
//				printf("virtual addr = 0x%lx, len = %d\n", phys_to_virtual(DMA_BUF_ADDR), reading_size);
				process_monitor_seek(phys_to_virtual(DMA_BUF_ADDR), reading_size,  0 );
#endif

				has_reading_size += reading_size;

//				gettimeofday(&tvafter_reading,&tz);
//				tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
//				RT_read_report(reading_size);
				
//				glb_reading_addr += reading_size;
			}
		}
		reading_addr = tmp - 1;
		writing_addr = *ptr_writing_addr;
	}
	printf("only go here one times !!!\n ");
	finish_kt_analysis = 0;
//	finish_kt_collect();
	print_prefetch();
	
	writing_addr = writing_addr - 0x8000000000000000;
	//printf("\n%lx, %lx\n", writing_addr, reading_addr);
	if(writing_addr > reading_addr + 1)
	{
		gettimeofday(&tvpre_reading,&tz);
		reading_size = writing_addr - reading_addr - 1;
		//printf("\n%lx\n", reading_size);
		if(direct_io) reading_size = (reading_size + 511ull) & ~511ull;
#ifdef STORE_TRACE
		if((i = WRITE( fp, phys_to_virtual(reading_addr)+1, reading_size)) != reading_size)
			printf("\n  #1:WARNINGing: write error!,%d,%d,%d\n",i,errno,reading_size);
#endif
#ifndef USE_PROCESS_PTHREAD
		analysis_trace_buff(phys_to_virtual(reading_addr)+1, reading_size);
#else
		process_monitor_seek(phys_to_virtual(reading_addr)+1, reading_size, 0 );
#endif
		gettimeofday(&tvafter_reading,&tz);
		tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
		RT_read_report(reading_size);
		
//		glb_reading_addr += reading_size;
	}
	else if(writing_addr < reading_addr + 1)
	{
		gettimeofday(&tvpre_reading,&tz);
		reading_size = DMA_BUF_ADDR + DMA_BUF_SIZE - reading_addr - 1;
		//printf("\n%lx\n", reading_size);
#ifdef STORE_TRACE
		if((i = WRITE( fp, phys_to_virtual(reading_addr)+1, reading_size)) != reading_size)
			printf("\n  #d:WARNINGing: write error!,%d,%d\n",i,errno);
#endif
#ifndef USE_PROCESS_PTHREAD
		analysis_trace_buff(phys_to_virtual(reading_addr)+1, reading_size);
#else
		process_monitor_seek(phys_to_virtual(reading_addr)+1, reading_size, 0);
#endif
		tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
		RT_read_report(reading_size);
		
//		glb_reading_addr += reading_size;
		
		gettimeofday(&tvpre_reading,&tz);
		reading_size = writing_addr - DMA_BUF_ADDR;
		//printf("\n%lx\n", reading_size);
		if(direct_io) reading_size = (reading_size + 511ull) & ~511ull;
		//printf("direct_io:%d",direct_io);
#ifdef STORE_TRACE
		if((i = WRITE( fp, phys_to_virtual(DMA_BUF_ADDR), reading_size)) != reading_size)
			printf("\n  #2:WARNINGing: write error!,%d,%d,%d\n",i,errno,reading_size);
#endif
#ifndef USE_PROCESS_PTHREAD
		analysis_trace_buff(phys_to_virtual(DMA_BUF_ADDR), reading_size);
#else
		process_monitor_seek(phys_to_virtual(DMA_BUF_ADDR), reading_size, 0 );
#endif
		tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
		RT_read_report(reading_size);
		
//		glb_reading_addr += reading_size;
	}
	//printf("\r\033[K\033[2A");
	//fflush(stdout);
}

unsigned long adrees_base = 0;
//     address = strtol(argv[2], NULL, 16);


int main(int argc, char **argv)
{
	now_pid = getpid();
	unsigned long t0,t1;
	printf(" sizeof(unsigned long ) = %d, (1 << 0) = %d\n", sizeof(unsigned long), 1 << 0);
//	multi_filter_table(1, 1);
	int moniter_ppn = 0;
	moniter_ppn = 0;
//	printf("value = 0x%lx\n", new_ppn[moniter_ppn].bitmap);
//	multi_filter_table(64, 1);
//	printf("value = 0x%lu\n", new_ppn[moniter_ppn].bitmap);
	int ti = 2;
#ifdef USING_BITMAP
	for(ti = 0;ti < 64; ti ++){
		multi_filter_table(ti * 64, 1);
		printf("value = 0x%lu, NUM = %lu, 0X%lx\n", new_ppn[moniter_ppn].bitmap, new_ppn[moniter_ppn].bitmap >> (60), 0xFFFFFFFFFFFFFFF& new_ppn[moniter_ppn].bitmap);
	}
#endif

//	return 0;

	init_prefetch_structure();

	unsigned long value = ULONG_MAX;
	unsigned count = 1;
	int test_times = 1000000;
	int re_t = test_times;

	t0 = get_cycles();
	while(test_times --){
		value = ULONG_MAX;
		while (value >>= 1)
		    ++count;
	}
	t1 = get_cycles();
	printf("count = %d, use cycle = %lu, average = %lu cycle\n", count, (t1 - t0), (t1 - t0) / re_t);



	printf("%d\n",getpid());
/******************For page management**************/
#ifndef USING_FASTSWAP
	init_user_engine();
#endif
#ifdef EVICT_ON
	init_evict_thread(); // evict thread start
	init_async_evict(); // async evict consume_build
#endif
	

// #define lt_1g_page_size (1UL << 18)
//     // lt test
//     unsigned long lt_c1 = 0, lt_c2 = 0;
// 	unsigned long lt_use_pid_log = atoi(argv[2]);
// 	unsigned long lt_init_page = strtol(argv[4], NULL, 16) >> 12;
//     unsigned long lt_start = get_cycles(), lt_end=0;
//     unsigned long ltret=0;
//     for(lt_c1=0; lt_c1 < 3*lt_1g_page_size; lt_c1+=32){
//         // make_reclaim_lazy_async(lt_use_pid_log, lt_init_page+lt_c1);
//         for(lt_c2=0;lt_c2 < 32; lt_c2++){
//             evict_engine_start_addr[0 * 32 + lt_c2 ].flags      = 1;
//             evict_engine_start_addr[0 * 32 + lt_c2 ].pid        = lt_use_pid_log;
//             evict_engine_start_addr[0 * 32 + lt_c2 ].page_num   = lt_init_page+lt_c1 + lt_c2;
//         }
        
// 		ltret+=syscall(336,  0, 32);
//     }
//     lt_end= get_cycles();
//     printf("\nLTINFO: reclaimer test reclaim pid=%d, vpn:%lx~%lx, cost:%.2f us in %lu page, %.2f us per page\n\n", 
//                 lt_use_pid_log, lt_init_page, lt_init_page + 3*lt_1g_page_size,
//                 (lt_end-lt_start)/2400.0, ltret, (lt_end-lt_start)/2400.0/(ltret));

//     printf("begin to trace?\n");
//     scanf("%lu", &lt_c1);
    init_ltls_inter();
	init_ltstatics();
/********************/

#if 0
//	init_memory_page();
//	init_page_state();	
	init_user_engine();
	printf("all page_num = %lu, used page number = %lu, free page number = %lu\n", memory_buffer_start_addr[0], memory_buffer_start_addr[1], memory_buffer_start_addr[2]);
	
	unmap_memory_and_state();
	return 0;
#endif

#if 0
	init_evict_buffer();

	copy_ppn_to_evict_buffer(11,11,1);

	return 0;
#endif


	// CPU_ZERO(&mask_cpu_train);
	// CPU_ZERO(&mask_cpu_hmtt);
	CPU_ZERO(&mask_cpu_2);
	CPU_ZERO(&mask_kt);
	CPU_ZERO(&mask_cpu_8);
	CPU_ZERO(&mask_cpu_prefetch_seek);
//	CPU_ZERO(&mask_prefetch_seek);
#ifdef USE_MORE_CORE
	CPU_ZERO(&mask_cpu_prefetch);
#endif
	

	int use_cpu = 0;
	// mask_cpu_2 for train and analysis
	for(use_cpu = 14; use_cpu < 16; use_cpu ++){
		CPU_SET(use_cpu , &mask_cpu_2);
	}
	// CPU_SET(13, &mask_cpu_hmtt);
	// CPU_SET(12, &mask_cpu_train);

	//0~13 cpu0 , 14~27 cpu1, 28~41 cpu0 , 42~ 55 cpu1

//	CPU_SET(use_cpu , &mask_cpu_8);
	CPU_SET(PREFETCH_SEEK_CORE , &mask_cpu_prefetch_seek);
#ifdef USE_MORE_CORE
//	for(use_cpu = 18; use_cpu < 28; use_cpu ++){
	for(use_cpu = 20; use_cpu < 28; use_cpu ++){
		CPU_SET(use_cpu , &mask_cpu_8);
	}
	for(use_cpu = 32; use_cpu < 56; use_cpu ++){
//	for(use_cpu = 32; use_cpu < 43; use_cpu ++){
//		if(use_cpu % 2 == 0)
		CPU_SET(use_cpu , &mask_cpu_prefetch);
	}
#else
//	for(use_cpu = 7; use_cpu < 12; use_cpu ++){
//	for(use_cpu = 8; use_cpu < 12; use_cpu ++){
//	for(use_cpu = 10; use_cpu < 12; use_cpu ++){
	
//        for(use_cpu = 8; use_cpu < 14; use_cpu ++){
        for(use_cpu = PREFETCH_CORE_ID; use_cpu < PREFETCH_CORE_ID+max_prefetcher_count; use_cpu ++){
                CPU_SET(use_cpu , &mask_cpu_8);
        }
//	for(use_cpu = 15; use_cpu < 22; use_cpu ++){
//	for(use_cpu = 38; use_cpu < 40; use_cpu ++){
//	for(use_cpu = 35; use_cpu < 40; use_cpu ++){
//		;
        //        CPU_SET(use_cpu , &mask_cpu_8);
//        }
#endif



	
	printf("start maini pid = %d\n",getpid());


#ifdef PREFETCH_ON
	init_prefetch_thread();  // init prefetch pthread to process prefetch messages
#endif 
	init_prefetch();         // init  train pthread


//	init_process_task();
	none_pte = 0;
	total_trace = 0;
	pb_w_ptr = 0;
	pb_r_ptr = 0 ;
	pb_w_ptr_pid =0;
	pb_r_ptr_pid = 0;


	tb_w_ptr = 0;
	tb_r_ptr = 0;


	

	unsigned long long tmp_ccc = get_cycles();
	printf("cycle = %llu\n",tmp_ccc);
	if(argc < 2)
	{
		printf("Usage:./trace_collect filename [-I]\n");
		return 0;
	}
	if(argc > 2){
		if(!strcmp(argv[2], "-I")){
			direct_io = 1;
			flag |= O_DIRECT;
		}
	}
	printf("Use %s\n\n", direct_io ? "Direct IO" : "Buffered IO");

	using_pid  = atoi(argv[2]);
	using_inter_page = atoi(argv[3]);
	printf("using_inter_page = %d\n", using_inter_page);
	printf("using_pid = %d\n", using_pid);
        moniter_base_address = strtol(argv[4], NULL, 16);

	memset(filename, 0, 64);
	strcpy(filename, argv[1]);
	strcat(filename, ".trace");
	
	//if((fp = open(filename,O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0)
#ifdef STORE_TRACE
	if((fp = open(filename,flag,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0)
	{
		printf("Open file %s error\n", filename);
		return 0;
	}
#endif
	//printf("page_size is %d\n", getpagesize());
	/*
	*  Step.1 Initial the DMA buffer for trace collection
	*/	

	int ff; //the file descriptor of the device memory_dev 
	unsigned long long memory_dev_size;	//the size of DMA buffer plus the DMA control registers
	
	ff = open("/dev/memory_dev", O_RDWR);
	if( ff < 0 ){
		printf("open /dev/memory_dev failed\n");
		return 0;
	}
	//GetCfgContent();	//get the DMA information from ../cfg_content
	
	memory_dev_size = DMA_BUF_ADDR - DMA_REG_ADDR + DMA_BUF_SIZE;
	memory_dev_addr = mmap(0, memory_dev_size, PROT_READ|PROT_WRITE, MAP_SHARED,ff,0);
	printf("memset start.\n");
//	memset(memory_dev_addr, 0, memory_dev_size);
	printf("memset over.\n");
	if ((unsigned long long)memory_dev_addr< 0)
	{
		printf("error in mmap\n");
		switch(errno)
		{
			case EACCES: printf("EACCES\n"); break;
			case EAGAIN: printf("EAGAIN\n"); break;
			case EBADF: printf("EBADF\n"); break;
			case EINVAL: printf("EINVAL\n"); break;
			case ENFILE: printf("ENFILE\n"); break;
			case ENODEV: printf("ENODEV\n"); break;
			case ENOMEM: printf("ENOMEM\n"); break;
			case EPERM: printf("EPERM\n"); break;
		}
		return -1;
	}
	printf("memory_dev_addr = 0x%lx\n", memory_dev_addr);
	
	unsigned long long remain;	
	gettimeofday(&tvpre_reading,&tz);
	remain = 0;
	while(remain < memory_dev_size){
		*((unsigned long long *)((char *)memory_dev_addr + remain)) += 1ull;
		remain += 8;
	}

	gettimeofday(&tvafter_reading,&tz);

	tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
	reading_speed = (double)reading_size/tv_reading/1024/1024;
	//printf("unsigned long long : %7.2fMB/s\n",(double)memory_dev_size/tv_reading/1024/1024);
	
	/*
	*  Step.2 Read the traces from DMA buffer and store them into file
	*/	
	
	ptr_writing_addr = (unsigned long long *)memory_dev_addr;
	//ptr_stopt_addr   = (unsigned long long *)(memory_dev_addr + 8);
	
	//set the writing_addr to zero at the beginning;
	*ptr_writing_addr = 0;
    
	//printf("\n\033[31;5mspecial mode\033[0m\n");
	printf("\n Waiting for trace...\n");


	/*    init kt(malloc) buffer                            **/	
//	kt_hmtt_dev_size = (MALLOC_TAG_SIZE + TMP_PHY_BUFFER) << 20;
	kt_hmtt_dev_size = (MALLOC_TAG_SIZE ) << 20;
//	printf("kt_dev_size = %dMB", MALLOC_TAG_SIZE + TMP_PHY_BUFFER);
	printf("store_trace_dev_size = %dMB", MALLOC_TAG_SIZE );
	kt_hmtt_ff = open(kt_hmtt_trace_path, O_RDWR);	
	printf("open %s in %d\n",kt_hmtt_trace_path, kt_hmtt_ff);
	if(kt_hmtt_ff < 0){
		printf("#### open %s failed\n",kt_hmtt_trace_path);
                exit(-8);
	}
	kt_hmtt_p_kernel_trace =  (unsigned char*)mmap(0, kt_hmtt_dev_size,  PROT_READ|PROT_WRITE, MAP_SHARED, kt_hmtt_ff, 0 );
	if( (long long) kt_hmtt_p_kernel_trace < 0){
		printf("#### error in mmap\n");
            switch(errno){
                case EACCES: printf("EACCES\n"); break;
                case EAGAIN: printf("EAGAIN\n"); break;
                case EBADF: printf("EBADF\n"); break;
                case EINVAL: printf("EINVAL\n"); break;
                case ENFILE: printf("ENFILE\n"); break;
                case ENODEV: printf("ENODEV\n"); break;
                case ENOMEM: printf("ENOMEM\n"); break;
                case EPERM: printf("EPERM\n"); break;
            }
            return -1;	
	}
	kt_hmtt_addr = kt_hmtt_p_kernel_trace;
	kt_hmtt_p_kernel_trace_buf = kt_hmtt_p_kernel_trace + (1ULL << 20); 





	trace_store_writeptr = (unsigned long*)kt_hmtt_p_kernel_trace;
        trace_store_readptr  = (unsigned long*)(trace_store_writeptr + 1);



//	tmp_phy_buffer_addr = kt_hmtt_p_kernel_trace + (MALLOC_TAG_SIZE << 20);




	/*
	 **  lhf add init pte collect
	 */
	dev_size = KERNEL_TRACE_SIZE << 20; //16384*4096;//ioctl(ff,PRO_IOCQ_PAGENUM) * 4096;         //get pro's buffer size
        printf("dev_size = %dMB\n",KERNEL_TRACE_SIZE);   //#define KERNEL_TRACE_SIZE (10241LLU)
	kt_ff = open(kernel_trace_path, O_RDWR);
	printf("open %s in %d\n", kernel_trace_path, kt_ff);
        if( kt_ff < 0 ){
	        printf("#### open %s failed\n",kernel_trace_path);
	        exit(-8);
	}
//	p_kernel_trace = (unsigned char*)mmap(0, dev_size, PROT_READ|PROT_WRITE, MAP_SHARED,kt_ff,0);
	p_kernel_trace = (unsigned char*)mmap(0, dev_size + (ANALYSIS_SIZE << 20), PROT_READ|PROT_WRITE, MAP_SHARED,kt_ff,0);
	start_new_ppn_address = (unsigned long) p_kernel_trace + dev_size; 


        p_kernel_trace_buf = p_kernel_trace + (KERNEL_TRACE_WRRD_PTR_SIZE << 20);
	ppn2rpt = (unsigned long *)p_kernel_trace_buf;



	printf("start_new_ppn_address = 0x%lx\n", (unsigned long) p_kernel_trace);
	printf("start_new_ppn_address = 0x%lx\n", start_new_ppn_address);
#ifdef OTHERPPN
	new_ppn = ( struct ppn2num_t * ) start_new_ppn_address;
#endif

	for(int j = 0; j < MAX_PPN; j++){
#ifdef USING_NUM_PREFETCH
		new_ppn[j].num = 0;
#endif
//      unsigned long num;

#ifdef MONITOER_PREFETCH
	        new_ppn[j].isprefetch = 0;
	        new_ppn[j].prefetch_time = 0;
#endif
#ifdef USETIMER
	        new_ppn[j].timer = 0;
#endif
	}


	


        if ((long long)p_kernel_trace < 0){
            printf("#### error in mmap\n");
            switch(errno){
                case EACCES: printf("EACCES\n"); break;
                case EAGAIN: printf("EAGAIN\n"); break;
                case EBADF: printf("EBADF\n"); break;
                case EINVAL: printf("EINVAL\n"); break;
                case ENFILE: printf("ENFILE\n"); break;
                case ENODEV: printf("ENODEV\n"); break;
                case ENOMEM: printf("ENOMEM\n"); break;
                case EPERM: printf("EPERM\n"); break;
	    }
	    return -1;
	}   
	
	while(*ptr_writing_addr == 0)
        {
                usleep(10000);
        }


	unsigned long times_u[3] = {0};
times_u[0] = get_cycles();

//	init_kt_collect();   // start  dump all page table entry
	times_u[1] = get_cycles();

	/*
	 *  lhf add init pte collect
	 */

//	init_page_table();



	times_u[2] = get_cycles();
	printf("time1 = %lu, time2 = %lu\n", times_u[1] - times_u[0],  times_u[2] - times_u[1]);

	
	printf("\n Collecting trace...\n\n");    
   
	 
	glb_writing_addr = DMA_BUF_ADDR;
	glb_reading_addr = DMA_BUF_ADDR - 1;

	//create two threads: one to check overflow; one to store traces into file
	pthread_t	tid_1,tid_2;
	pthread_t tid_3; //tid_4;
//	pthread_create(&tid_1, NULL, check_overflow, NULL);
	pthread_create(&tid_2, NULL, store_to_disk, NULL);

//        pthread_create(&tid_3, NULL, analysis_kt_buffer, NULL);
//      pthread_create(&tid_3, NULL, analysis_kt_buffer_for, NULL);
//	pthread_setaffinity_np(tid_3, sizeof(mask_cpu_8), &mask_cpu_8);
//	pthread_setaffinity_np(tid_3, sizeof(mask_kt), &mask_kt);


//	pthread_setaffinity_np(tid_1, sizeof(mask_cpu_8), &mask_cpu_8);
	pthread_setaffinity_np(tid_2, sizeof(mask_cpu_2), &mask_cpu_2);
//	pthread_setaffinity_np(tid_2, sizeof(mask_cpu_hmtt), &mask_cpu_hmtt);

	
	
//	init_page_table_pthread();




	pthread_join(tid_1,NULL);
	if(!overflow) 
	{
//		pthread_join(tid_3,NULL);
		pthread_join(tid_2,NULL);
		for(int i =0 ; i < max_prefetcher_count; i++){
//			pthread_join(tid[i], NULL);
//			printf("i = %d, join\n",i);
		}

		printf("\n\n Done!");
		printf("  %10.2fMiB of trace collected.\n\n",(double)(glb_reading_addr-DMA_BUF_ADDR + 1)/1024/1024);
		//printf("  %llu, %llu\n",glb_writing_addr-DMA_BUFFER_ADDRESS+1,glb_reading_addr-DMA_BUFFER_ADDRESS+1);
#ifdef STORE_TRACE
		if(glb_writing_addr != glb_reading_addr && ftruncate(fp, glb_writing_addr-DMA_BUF_ADDR + 1)){
			printf("ftruncate failed\n");
		}
#endif
		tv_writing = (double)(tvlast_writing.tv_sec - tvfirst_writing.tv_sec) + (double)(tvlast_writing.tv_usec - tvfirst_writing.tv_usec)/1000000;
		writing_speed = (double)(glb_writing_addr-DMA_BUF_ADDR + 1)/tv_writing/1024/1024;		//in MB/s
		double writing_speed_1 = (double)(glb_writing_addr-DMA_BUF_ADDR + 1)/sum_tv_writing/1024/1024;
		//printf("%7.2fMB/s, %7.2fMB/s, overhead %%%.2f\n", writing_speed_1, writing_speed, (tv_writing - sum_tv_writing) / tv_writing * 100);
		printf("  Throughput: %7.2fMiB/s\n", writing_speed);
	}
	else{
//		finish_kt_collect();	
		printf("\n\n # ERROR: OVERFLOW HAPPENED!\n\n");
	}
	//if(overflow) printf("  Overflow happened! %7.2f MB of trace lost!\n\n",(double)overflow*DMA_BUFFER_TOTAL_SIZE/1024/1024);
	printf("\n");
	
	end_ltstatics();

//	printf("none_pte = %llu, total trace = %llu, none_pte_percent = %lf \n", none_pte, total_trace, (double)none_pte/ (double)total_trace);
	printf("monitor_trace_num = %d\n", monitor_trace_num);
	printf("send_monitor_trace_num = %d\n", send_monitor_trace_num);
//        printf("none_pte = %llu, real_miss =  %llu, total trace = %llu, none_pte_percent = %lf  real_none_pte_percent = %lf\n", none_pte, real_miss,total_trace, (double)none_pte/ (double)total_trace, (double)real_miss/ (double)total_trace);
	int tmp_total_trace = total_trace;
        int tmp_none_pte = none_pte;
	printf("pte percent = %lf, total_trace = %llu, miss_set_pte = %llu, miss_free_pte = %llu  \n", (double)(tmp_none_pte - last_none_pte) / 200000000.0, tmp_total_trace,miss_set_pte - last_miss_set_pte, miss_free_pte - last_miss_free_pte );
	printf("none-pte with delay hit = %lf\n", (double)(real_miss + fake_hit) /  (double)total_trace);
	printf("set_pte_cnt = %llu , set_pte_cnt_w = %llu, free_pte_cnt = %llu, free_pte_cnt_w = %llu, some_error_trace = %llu\n",set_pte_cnt, set_pte_cnt_w, free_pte_cnt, free_pte_cnt_w, some_error_trace);
	printf("average len = %lu, all_has_read = %lu, times_use = %d\n", all_has_read/ times_use, all_has_read, times_use);
	printf("now pid = %d\n", now_pid);
	printf("LT: send %lu, ac %lu, ratio %.3f \n", pre_counter, ac_counter, (float)ac_counter/(float)pre_counter);
	printf("average syscall time = %.3lf \n", (double)syscall_all_time /(double)pre_counter );
	printf("store_to_pb_num = %lu\n", store_to_pb_num);
	printf("store_to_tb_num = %lu, hot_physical_number = %lu\n", store_to_tb_num, hot_physical_number);
	printf("store_to_eb_num = %lu\n", store_to_eb_num);
	printf("syscall_use = %lu\n", syscall_use);
	printf("average late arrival time %.3lf cycles, latency_all = %lu cycles , max_latency = %lu cycles\n",  (double)latency_all / (double)ac_counter, latency_all, max_latency);
	printf("now_prefetch_step = %d\n", now_prefetch_step);
	printf("monitor_trace = %d , monitor pte = %d \n", monitor_num, monitor_num_pte);
	print_async_msg();
	print_msg_evict();
	printf("**************\n\n");
#ifndef USING_FASTSWAP
	check_ppn_list();
#endif

	for(int p = 0; p < max_prefetcher_count; p ++){
		printf("delete num = %d, id = %d\n", prefetcher_task_set[p].delete_num, p);
	}

	
	printf("max_training_len = %d , max_read_len = %lu\n", max_training_len, max_read_len);
	for(int j = 0 ; j < 25; j++)	{
		if(monitor_num_split[j] != 0){
//			printf("  id = %d, num = %d , pte num = %d\n",j , monitor_num_split[j] , monitor_num_split_pte[j]);
		}
		if(j == 0 ){
			for(int z = 0; z < 100; z++){
				;
//				printf( " ** id = %d, HMTT num = %d\n ", z , monitor_num_split_first_100k[z] );
			}

		}
	}
	printf("{");
	for(int j = 0 ; j < 25; j++)    {
                if(monitor_num_split[j] != 0){
			printf("%d,", monitor_num_split[j]);
		}
	}
	printf("}\n");
	for(int j = 0; j < 25; j++){
		if(HMTT_trace_num[j] != 0)
		{
			if( j != 0 )
				printf("id = %d, HMTT num = %lu, inter_page = %lu\n",j, HMTT_trace_num[j], HMTT_trace_num[j] - HMTT_trace_num[j - 1]);
		}
	}

	printf("max_kt_buffer_offst = %lu\n", max_kt_buffer_offst);
	print_stride();

    end_ltls_inter();

	store_hmtt_trace();

	for(int j = 0;j < max_prefetcher_count; j++){
		int tmp_sum = 0;
		for(int i = 0; i < 15;i ++){
			tmp_sum += prefetch_program[j][i] ;
		}
		printf("id = %d, sum = %d\n",j, tmp_sum);
	}
	munmap(memory_dev_addr,memory_dev_size);
//	    munmap(p_kernel_trace,dev_size);
	    munmap(p_kernel_trace,dev_size + ANALYSIS_SIZE);
	    munmap(kt_hmtt_p_kernel_trace, kt_hmtt_dev_size);
	    close(kt_ff);
	close(ff);
#ifndef USING_FASTSWAP
	unmap_memory_and_state();
#endif


#ifdef EVICT_ON
	//ummap_evict_buffer();

#endif


#ifdef STORE_TRACE
	close(fp);
#endif
	return 0;
 }

