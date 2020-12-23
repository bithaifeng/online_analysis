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

int fp;



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
unsigned char *p_kernel_trace = NULL, *p_kernel_trace_buf = NULL;




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
unsigned long *dev_writeptr;
unsigned long *dev_readptr;
unsigned long long kernel_trace_offset = 0;
unsigned long long tagp = 0;
unsigned long *ppn2vpn = NULL;
int *ppn2pid = NULL;
map<unsigned long, unsigned long> vpn2ppn;
int glb_start_analysis = 0;
unsigned long long total_trace = 0;
unsigned long long none_pte = 0;
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

int is_kernel_tag_trace(unsigned long long addr) {
	    return addr >= kernel_config_entry && addr < kernel_config_end;
}

void init_kt_collect(){
	ppn2pid = new int[MAXPPN];
	ppn2vpn = new uint64_t[MAXPPN];


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
        sleep(1);

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


void get_kt_trace(){
	//memcpy from   to kt_ch_tmp
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
		while(*dev_readptr + 16 < *dev_writeptr){
			if (strncmp(p_kernel_trace_buf + *dev_readptr , "@@@@@@@@@@@@@", 13) == 0) {
				tagp += 16;
				printf("start collect trace  flag, tagp = %lu \n", tagp);
				*dev_readptr += 16;
				break;
			}
		}
		while(*dev_readptr + 16 < *dev_writeptr){
			if (strncmp(p_kernel_trace_buf + *dev_readptr , "#############", 13) == 0) {
                                tagp += 16;
                                printf("start dump page talbe flag, tagp = %lu \n", tagp);                          
                                *dev_readptr += 16;
                                break;
                        }
		}

		
		while( *dev_readptr + 16 < *dev_writeptr ){
			get_kt_trace();
//			if (strncmp(p_kernel_trace_buf + *dev_readptr, "&&&&&&&&&&&&&", 13) == 0) {
			if (strncmp(kt_ch_tmp, "&&&&&&&&&&&&&", 13) == 0) {
				tagp += 16;
				printf("end dump page talbe flag, ,dump_pte_trace = %llu, tagp = %lu\n",dump_pte_trace,tagp);
				*dev_readptr += 3;
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
		        vpn2ppn[vpn] = ppn;
			
//			*dev_readptr += 13;
			dump_pte_trace ++;
		}
	}
	printf("init page table end\n");

}

extern void analysis_single_trace(char *trace_start);

void analysis_trace_buff(unsigned long long start_addr, unsigned long long read_len){
	// 把之前开头的存下来
//	return ;

	//解决有剩的，但是新的和剩的无法拼接成一个trace
	if(ht_left + read_len < 6){
		memcpy(ht_ch_tmp + ht_left, start_addr, read_len);
		ht_left += read_len;
		glb_reading_addr += read_len;
		return ;
	}



	unsigned long long new_start_addr = start_addr, new_read_len = read_len;
	if(ht_left != 0 && ht_left < 6 ){
		memcpy(ht_ch_tmp + ht_left, start_addr, 6 - ht_left);
		new_start_addr += ( 6 - ht_left);
		new_read_len -= (6 - ht_left);
		// analysis first hmtt trace
		analysis_single_trace(ht_ch_tmp);
		glb_reading_addr += (6 - ht_left);
		ht_left = 0;

	}
	unsigned long long i = 0;
	for(i = 0; i < new_read_len / 6; i ++){
	// analysis each hmtt trace , 6B per trace
		memcpy(ht_ch_tmp, new_start_addr + i * 6, 6);
//		analysis_single_trace(ht_ch_tmp);
		analysis_single_trace(new_start_addr + i * 6);
		 glb_reading_addr += 6;

	}
	if(new_read_len % 6 != 0){
		ht_left = new_read_len % 6;
		memcpy(ht_ch_tmp, new_start_addr +  new_read_len / 6 * 6, ht_left );
		 glb_reading_addr += ht_left;
		//warning!! can not do analusis here!!!!

	}
}

void analysis_single_trace(char *trace_start){
	unsigned long long tmp = (unsigned long long)( *(unsigned long long*)trace_start);
	unsigned int seq_no,r_w;
	unsigned long paddr;
        unsigned long long timer;
	unsigned int pid;
        unsigned long ppn;
        unsigned long vpn;
        uint64_t val;
        char magic;


	seq_no = (unsigned int) ((tmp >> 40) & 0xffU);
	timer  = (unsigned long long)((tmp >> 32) & 0xffULL);
        r_w    = (unsigned int)((tmp >> 31) & 0x1U);
        paddr   = (unsigned long)(tmp & 0x7fffffffUL);
        paddr   = (unsigned long)(paddr << 6);
	record.paddr = paddr;
	record.rw = r_w;
//	return ;
	if (record.paddr == 0 && timer == 0){
		//invalid trace
		duration_all += 256;
		return ;
	}

//[OK]
//	return ;
	duration_all += timer;
	record.tm = duration_all;
	if (record.paddr >= (2ULL << 30)) {
	        record.paddr += (2ULL << 30);
        }
//[ok]	return ;
/*// [why ????? 为啥会缓冲区溢出]
	if(has_print < 10){
		printf("id = %d, seq_no = %d ,r_w = %d, paddr = %lx \n",  has_print,seq_no, r_w, record.paddr);
		has_print ++;
	}
*/
//	return ;
	if (is_kernel_tag_trace(record.paddr) ){
		//This is a hmtt kernel tag trace, we should read kernel trace buffer now
//[ERROR]	
//		return ;
		int kernel_trace_seq = (record.paddr - kernel_config_entry) / (KERNEL_TRACE_CONFIG_SIZE << 20);
		if (kernel_trace_seq >= KERNEL_TRACE_SEQ_NUM) {
			fprintf(stderr, "#### Invalid kernel trace seq:addr=0x%llx, seq=%d\n", record.paddr, kernel_trace_seq);
			exit(-1);
		}
		unsigned long long kernel_trace_seq_entry =
			                    record.paddr - kernel_config_entry - kernel_trace_seq * (KERNEL_TRACE_CONFIG_SIZE << 20);
		int kernel_trace_tag = (kernel_trace_seq_entry) / TAG_ACCESS_SIZE;
		if (kernel_trace_tag >= TAG_MAX_POS) {
			fprintf(stderr, "#### Invalid kernel_trace_tag:addr=0x%llx,tag=%d\n", record.paddr, kernel_trace_tag);
                	exit(-1);
            	}
           	 int hmtt_kt_type = ((kernel_trace_seq_entry) % TAG_ACCESS_SIZE) / TAG_ACCESS_STEP;
            	if (hmtt_kt_type > 1) {
                	fprintf(stderr, "#### Invalid hmtt_kt_type:addr=0x%llx,tag=%d,type=%d\n", record.paddr, kernel_trace_tag,
                        	hmtt_kt_type);
                	exit(-1);
            	} else if (hmtt_kt_type == 1);
		
		if (glb_start_analysis == 0 && kernel_trace_tag == DUMP_PAGE_TABLE_TAG) {
                	printf("find DUMP_PAGE_TABLE_TAG in hmtt trace \n");
			glb_start_analysis = 1;
			printf("dump page done.\n");
    			printf("trace init done.\n");
			return ;
            	}
		if(glb_start_analysis == 0)
			return ;

		switch (kernel_trace_tag) {
			case SET_PTE_TAG:
				if(record.rw == 0){
					set_pte_cnt_w ++;
					break;
				}
				set_pte_cnt ++;
				get_kt_trace();
				magic =  (*(char*)(kt_ch_tmp));
				if(magic != set_page_table_magic){
					while((magic != set_page_table_magic)){
		                                if(magic == free_page_table_magic || magic == free_page_table_get_clear || magic == free_page_table_get_clear_full)
                		                        skip_free_in_set ++;
                                		get_kt_trace();
		                                magic =  (*(char*)(kt_ch_tmp));
                        		}
				}	
				pid = (*(int*)((kt_ch_tmp + 1)));
				val = (*(uint64_t*)(kt_ch_tmp + 5));
	                        ppn = val & 0xffffff;
            	                vpn = (val >> 24) & 0xffffffffff;
				if (ppn >= MAXPPN) {
                    		    printf("invalid ppn in set pte\n ");
				    exit(-1);
				    return ;
                    		}
                    		ppn2pid[ppn] = pid;
	                    	ppn2vpn[ppn] = vpn;
                    		vpn2ppn[vpn] = ppn;				


				break;
			case FREE_PTE_TAG:
				if(record.rw == 0){
					free_pte_cnt_w ++;
					break;
				}
				free_pte_cnt ++;
				get_kt_trace();
                                magic =  (*(char*)(kt_ch_tmp));
				if(magic != free_page_table_magic && magic != free_page_table_get_clear && magic != free_page_table_get_clear_full){
					break;
				}
				pid = (*(int*)((kt_ch_tmp + 1)));
                                val = (*(uint64_t*)(kt_ch_tmp + 5));
                                ppn = val & 0xffffff;
                                vpn = (val >> 24) & 0xffffffffff;
				if (ppn >= MAXPPN) {
                                    printf("invalid ppn in set pte\n ");
                                    exit(-1);
                                    return ;
                                }
				ppn2pid[ppn] = 0;
                                ppn2vpn[ppn] = 0;
				

				break;
			case DUMP_PAGE_TABLE_TAG:
				printf("****** stop analysis***** \n");
                    		break;
			case KERNEL_TRACE_END_TAG:
				printf("find KERNEL_TRACE_END_TAG\n");
				get_kt_trace();
				glb_start_analysis = 0;
	                        if (strncmp(kt_ch_tmp, "$$$$$$$$$$$$$", 13) == 0) {
					*dev_readptr += 3;		
					printf("kernel trace end.\n");
                    		} else {
                        		printf("error: kernel trace end\n");
                    		}
                    		break;
                	default:
                		some_error_trace += 1;
	
		}
	}
	else if(glb_start_analysis == 1){
		if(total_trace % 200000000 == 0){
	                printf("pte percent = %lf, total_trace = %llu\n", (double)(none_pte - last_none_pte) / 200000000.0, total_trace);
	                last_none_pte = none_pte;
	        }

		total_trace ++;
		if(total_trace > 0){
//			printf("total_trace = %llu\n",total_trace);
		}
		unsigned long long ppn = record.paddr >> 12;
		if(ppn > MAXPPN)
		{
			printf("invalid ppn in normol hmtt trace!\n ");
                        exit(-1);
			return ;
		}
		unsigned long long pmd_ppn =(ppn >> 8);
		if (ppn2vpn[ppn] == 0 && ppn2pid[ppn] == 0) {
	                none_pte += 1;
        	        record.pid = -1;
                	record.vaddr = 0;		
			
		}
		else{
			record.pid = ppn2pid[ppn];
	                record.vaddr = (ppn2vpn[ppn] << 12) | (record.paddr & 0xfff);
		}

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
		usleep(10000);//10ms,it will take at least 80ms to fillin a 64MB segment at the speed of 800MB/s
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

void *store_to_disk()
{
	unsigned long long tmp;
	int i = 0;

	unsigned long long  writing_addr;
	reading_addr = DMA_BUF_ADDR - 1;
	
	writing_addr = *ptr_writing_addr;

	//maintain the reading_addr
	while(writing_addr < 0x8000000000000000)
	{
		if(writing_addr == reading_addr + 1)
		{
			usleep(10000);
			writing_addr = *ptr_writing_addr;
			continue;
		}
		else if(writing_addr > reading_addr + 1)
		{
			gettimeofday(&tvpre_reading,&tz);
			tmp = writing_addr;
			reading_size = writing_addr - reading_addr - 1;
			analysis_trace_buff(phys_to_virtual(reading_addr)+1, reading_size);
#ifdef STORE_TRACE			
			if((i = WRITE( fp,phys_to_virtual(reading_addr)+1, reading_size)) != reading_size)
				printf("\n  #a:WARNINGing: write error!,%d,%d,%d\n",i,reading_size,errno);
#endif

			gettimeofday(&tvafter_reading,&tz);
			tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
			RT_read_report(reading_size);

//			glb_reading_addr += reading_size;
		}
		else
		{
			gettimeofday(&tvpre_reading,&tz);
			tmp = DMA_BUF_ADDR;
			reading_size = DMA_BUF_ADDR + DMA_BUF_SIZE - reading_addr - 1;
			analysis_trace_buff(phys_to_virtual(reading_addr)+1, reading_size);
#ifdef STORE_TRACE
			if((i = WRITE( fp,phys_to_virtual(reading_addr)+1, reading_size)) != reading_size)
				printf("\n  #b:WARNINGing: write error!,%d,%d\n",i,errno);
#endif

			gettimeofday(&tvafter_reading,&tz);
			tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
			RT_read_report(reading_size);

//			glb_reading_addr += reading_size;
			
			//if((writing_addr = *ptr_writing_addr) && (writing_addr != DMA_BUFFER_ADDRESS))
			{
				gettimeofday(&tvpre_reading,&tz);
				tmp = writing_addr;
				reading_size = writing_addr - DMA_BUF_ADDR;

				analysis_trace_buff(phys_to_virtual(DMA_BUF_ADDR), reading_size);
#ifdef STORE_TRACE
				if((i = WRITE( fp,phys_to_virtual(DMA_BUF_ADDR), reading_size)) != reading_size)
					printf("\n  #c:WARNINGing: write error!,%d,%d\n",i,errno);
#endif

				gettimeofday(&tvafter_reading,&tz);
				tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
				RT_read_report(reading_size);
				
//				glb_reading_addr += reading_size;
			}
		}
		reading_addr = tmp - 1;
		writing_addr = *ptr_writing_addr;
	}
	printf("only go here one times !!!\n ");
	finish_kt_collect();
	
	writing_addr = writing_addr - 0x8000000000000000;
	//printf("\n%lx, %lx\n", writing_addr, reading_addr);
	if(writing_addr > reading_addr + 1)
	{
		gettimeofday(&tvpre_reading,&tz);
		reading_size = writing_addr - reading_addr - 1;
		//printf("\n%lx\n", reading_size);
		if(direct_io) reading_size = (reading_size + 511ull) & ~511ull;
		analysis_trace_buff(phys_to_virtual(reading_addr)+1, reading_size);
#ifdef STORE_TRACE
		if((i = WRITE( fp, phys_to_virtual(reading_addr)+1, reading_size)) != reading_size)
			printf("\n  #1:WARNINGing: write error!,%d,%d,%d\n",i,errno,reading_size);
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
		analysis_trace_buff(phys_to_virtual(reading_addr)+1, reading_size);
#ifdef STORE_TRACE
		if((i = WRITE( fp, phys_to_virtual(reading_addr)+1, reading_size)) != reading_size)
			printf("\n  #d:WARNINGing: write error!,%d,%d\n",i,errno);
#endif
		tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
		RT_read_report(reading_size);
		
//		glb_reading_addr += reading_size;
		
		gettimeofday(&tvpre_reading,&tz);
		reading_size = writing_addr - DMA_BUF_ADDR;
		//printf("\n%lx\n", reading_size);
		if(direct_io) reading_size = (reading_size + 511ull) & ~511ull;
		//printf("direct_io:%d",direct_io);
		analysis_trace_buff(phys_to_virtual(DMA_BUF_ADDR), reading_size);
#ifdef STORE_TRACE
		if((i = WRITE( fp, phys_to_virtual(DMA_BUF_ADDR), reading_size)) != reading_size)
			printf("\n  #2:WARNINGing: write error!,%d,%d,%d\n",i,errno,reading_size);
#endif
		tv_reading = (double)(tvafter_reading.tv_sec - tvpre_reading.tv_sec) + (double)(tvafter_reading.tv_usec - tvpre_reading.tv_usec)/1000000;
		RT_read_report(reading_size);
		
//		glb_reading_addr += reading_size;
	}
	//printf("\r\033[K\033[2A");
	//fflush(stdout);
}


int main(int argc, char **argv)
{
	char filename[64];
	
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

	init_kt_collect();

	/*
	 *  lhf add init pte collect
	 */

	init_page_table();

	
	printf("\n Collecting trace...\n\n");    
   
	 
	glb_writing_addr = DMA_BUF_ADDR;
	glb_reading_addr = DMA_BUF_ADDR - 1;

	//create two threads: one to check overflow; one to store traces into file
	pthread_t	tid_1,tid_2;
	
	pthread_create(&tid_1, NULL, check_overflow, NULL);
	pthread_create(&tid_2, NULL, store_to_disk, NULL);
	pthread_join(tid_1,NULL);
	if(!overflow) 
	{
		pthread_join(tid_2,NULL);

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
	    close(kt_ff);
	close(ff);
#ifdef STORE_TRACE
	close(fp);
#endif
	return 0;
 }

