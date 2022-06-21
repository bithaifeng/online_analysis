
#define ASYNC_BUFFER_LEN 512

struct async_struct{
        unsigned long ppn;
};


#define EVICT_PTHREAD_NUM 1

struct evict_buffer_struct{
	unsigned long ppn;
	int pid;
	unsigned char flag;
};


struct evict_task{
	volatile unsigned long evict_buffer_write_ptr, evict_buffer_read_ptr;
	int tid;
	struct evict_buffer_struct evict_buffer[ASYNC_BUFFER_LEN];

	/* for stream evict */	
	
	unsigned long evict_stream_buffer_write_ptr, evict_stream_buffer_read_ptr;
	struct evict_buffer_struct evict_stream_buffer[ASYNC_BUFFER_LEN];


};

extern struct evict_task evict_task_set[ EVICT_PTHREAD_NUM ];



//flag 0 ---- ppn, flag 1 ------ vpn
extern unsigned long copy_ppn_to_evict_buffer(unsigned long ppn, int pid, unsigned char flag, int id);
unsigned long get_evict_buffer_size(int id);


extern unsigned long copy_ppn_to_evict_stream_buffer(unsigned long ppn, int pid, unsigned char flag, int id);


void lt_mem_pressure_check();

extern unsigned long evict_buffer_write_ptr, evict_buffer_read_ptr;

extern struct evict_buffer_struct evict_buffer[ASYNC_BUFFER_LEN];


void init_evict_thread();
extern void print_msg_evict();
