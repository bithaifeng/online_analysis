

extern int finish_kt_analysis ;
extern int glb_start_analysis;

extern char kt_ch_tmp[20];
extern volatile unsigned long *ppn2vpn;
extern volatile int *ppn2pid;

extern int using_pid;
extern unsigned long moniter_base_address;


#define ANALYSIS_BUFFER_SIZE 20000
#define MAX_ANALYSIS_NUM 1

extern void init_page_table_pthread();
extern int analysis_kt_buffer_for(void  *arg);



struct analysis_pg_task{
  unsigned int real_tid;
  int tid;
  volatile unsigned long long value[ANALYSIS_BUFFER_SIZE];
  volatile int pid[ANALYSIS_BUFFER_SIZE];
  volatile char magic[ANALYSIS_BUFFER_SIZE];
  volatile unsigned long ptr_w,ptr_r;
  volatile int task_len;
};


extern struct analysis_pg_task analysis_pg_task_set[ MAX_ANALYSIS_NUM ];



