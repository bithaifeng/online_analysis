#define _GNU_SOURCE
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

#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#include "/root/lhf/hmtt_kernel_control/hmtt_kernel.h"

using namespace std;

//#define USE_SLEEP


//#define _GNU_SOURCE
#define MAXPPN ((64ULL << 30) >> 12)

#if 1
#define WRITE write
#else
int WRITE(int handle, void *buf, int nbyte){
	return nbyte;
}
#endif

#define	 DMA
//#include "../GetCfgContent.h"
#include "cfg_content.h"

#define phys_to_virtual(phys) (phys - DMA_REG_ADDR + memory_dev_addr)


//#define STORE_TRACE

#define MAX_PRINT 10
int has_print = 0;

char *memory_dev_addr;
extern int errno;

volatile unsigned long long *ptr_writing_addr;
unsigned long long *ptr_stopt_addr;
//unsigned long long  writing_addr;
unsigned long long  reading_addr;
unsigned long long  stopt_addr;

unsigned long long glb_writing_addr;// = DMA_BUFFER_ADDRESS;
unsigned long long glb_reading_addr;// = DMA_BUFFER_ADDRESS-1;

volatile unsigned long long glb_kt_writing_addr = 0;// = DMA_BUFFER_ADDRESS;
volatile unsigned long long glb_kt_reading_addr = 0;// = DMA_BUFFER_ADDRESS-1;

unsigned long long kt_hmtt_writing_addr = 0;
unsigned long long kt_hmtt_reading_addr = 0;



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
unsigned long long miss_set_pte = 0, miss_free_pte = 0;



#if 1
void RT_read_report(unsigned long long reading_size){
	reading_speed = (double)reading_size/tv_reading/1024/1024;
	printf("					the store speed is %7.2f MiB/s.\r",reading_speed);
	fflush(stdout);
}
#else
#define RT_read_report(...) 
#endif

#if 1
void RT_write_report(unsigned long long writing_size){
	writing_speed = (double)writing_size/tv_writing/1024/1024;		//in MB/s
	printf("$ The trace bandwidth is %7.2f MiB/s;									\r",writing_speed);
	fflush(stdout);
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

//kernel_address

static char *kernel_trace_path = "/dev/hmtt_kernel";
int kt_ff;
unsigned long dev_size, buffer_size;
volatile unsigned char *p_kernel_trace = NULL, *p_kernel_trace_buf = NULL;


static char *kt_hmtt_trace_path = "/dev/malloc_tag";
int kt_hmtt_ff = -1;
unsigned long kt_hmtt_dev_size, kt_hmtt_buffer_size;
unsigned char *kt_hmtt_p_kernel_trace = NULL, *kt_hmtt_p_kernel_trace_buf = NULL;




char *kt_ch;
char *ht_ch;
char kt_ch_tmp[20];
char ht_ch_tmp[20];
char kt_left = 0;
char ht_left = 0;
struct record_t {
    unsigned int rw;
    int pid;
    unsigned long tm;
    unsigned long paddr;
    unsigned long long vaddr;
};
struct record_t record;
unsigned long long duration_all = 0;
unsigned long          last_writeptr;
volatile unsigned long *dev_writeptr;
volatile unsigned long *dev_readptr;
unsigned long long kernel_trace_offset = 0;
unsigned long long tagp = 0;
unsigned long *ppn2vpn = NULL;
int *ppn2pid = NULL;
map<unsigned long, unsigned long> vpn2ppn;
int glb_start_analysis = 0;
volatile unsigned long long total_trace = 0;
unsigned long long none_pte = 0;
unsigned long long last_none_pte = 0;
unsigned long long last_miss_set_pte = 0;
unsigned long long last_miss_free_pte = 0;
int finimmap(0, memory_dev_size, PROT_READ|PROT_WRITE, MAP_SHARED,ff,0);
	printf("memset start.\n");
	memset(memory_dev_addr, 0, memory_dev_size);
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

/* ********************************************************************/
//      deal KT trace
//	kt_hmtt_dev_size = MALLOC_TAG_SIZE << 20;
	kt_hmtt_dev_size = (MALLOC_TAG_SIZE + TMP_PHY_BUFFER)<< 20;
	printf("kt_hmtt_dev_size = %dMB\n", kt_hmtt_dev_size);
	printf("start open\n");
	kt_hmtt_ff = open(kt_hmtt_trace_path, O_RDWR);
	printf("open kt_hmtt_trace_path  %s in %d\n", kt_hmtt_trace_path, kt_hmtt_ff);
	if( kt_hmtt_ff < 0 ){
                printf("#### open %s failed\n", kt_hmtt_trace_path);
	        exit(-8);
        }
	kt_hmtt_p_kernel_trace = (unsigned char*)mmap(0, kt_hmtt_dev_size, PROT_READ|PROT_WRITE, MAP_SHARED,kt_hmtt_ff,0);
	if ((long long)kt_hmtt_p_kernel_trace < 0 ){
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


/* ********************************************************************/	



	/*
	 **  lhf add init pte collect
	 */
	dev_size = KERNEL_TRACE_SIZE << 20; //16384*4096;//ioctl(ff,PRO_IOCQ_PAGENUM) * 4096;         //get pro's buffer size
        printf("dev_size = %dMB\n",KERNEL_TRACE_SIZE);
	kt_ff = open(kernel_trace_path, O_RDWR);
	printf("open %s in %d\n", kernel_trace_path, kt_ff);
        if( kt_ff < 0 ){
	        printf("#### open %s failed\n",kernel_trace_path);
	        exit(-8);
	}
	p_kernel_trace = (unsigned char*)mmap(0, dev_size, PROT_READ|PROT_WRITE, MAP_SHARED,kt_ff,0);
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


	/*
	 *  lhf add init pte collect
	 */

	init_kt_collect();
	init_page_table();

	
	printf("\n Collecting trace...\n\n");    
   
	 
	glb_writing_addr = DMA_BUF_ADDR;
	glb_reading_addr = DMA_BUF_ADDR - 1;

	//create two threads: one to check overflow; one to store traces into file
	pthread_t	tid_1,tid_2;
	pthread_t tid_3;
	
	pthread_create(&tid_1, NULL, check_overflow, NULL);
	pthread_create(&tid_2, NULL, store_to_disk, NULL);
	pthread_create(&tid_3, NULL, analysis_kt_buffer, NULL);
	pthread_join(tid_1,NULL);
	if(!overflow) 
	{
		pthread_join(tid_2,NULL);
		pthread_join(tid_3,NULL);

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
		finish_kt_collect();	
		printf("\n\n # ERROR: OVERFLOW HAPPENED!\n\n");
	}
	//if(overflow) printf("  Overflow happened! %7.2f MB of trace lost!\n\n",(double)overflow*DMA_BUFFER_TOTAL_SIZE/1024/1024);
	printf("\n");
	printf("none_pte = %llu, total trace = %llu, none_pte_percent = %lf \n", none_pte, total_trace, (double)none_pte/ (double)total_trace);
	printf("set_pte_cnt = %llu , set_pte_cnt_w = %llu, free_pte_cnt = %llu, free_pte_cnt_w = %llu, some_error_trace = %llu\n",set_pte_cnt, set_pte_cnt_w, free_pte_cnt, free_pte_cnt_w, some_error_trace);

	munmap(memory_dev_addr,memory_dev_size);
	    munmap(p_kernel_trace,dev_size);
	    munmap(kt_hmtt_p_kernel_trace,kt_hmtt_dev_size);
	    close(kt_ff);
	close(ff);
#ifdef STORE_TRACE
	close(fp);
#endif
	return 0;
 }

