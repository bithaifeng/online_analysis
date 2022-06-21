
#ifndef MEM_MANAGE_H
#define MEM_MANAGE_H

#define EVCIT_RET_COPY_SUCCESS 888
extern  void list_del( struct page_list * page_list );


struct evict_struct
{
    // identify the page mode.
    char    flags;

    // also present the page result
    char    result;

    // flags=0, pid will be meaningless
    unsigned int    pid;
    // flags=0 means ppn
    // flags=1 means vpn
    unsigned long   page_num;
};

#define EVICT_ENGIN_SIZE (32 * 4 * sizeof(evict_struct))


struct hmtt_page_state
{
    // in fact it is in with cpu's list
    unsigned int cpu;

    // page state
    // 0 means page is no use or just in cache
    // 1 means page is using or said mapped
    // 2 means page is using or said mapped and updated list in hmtt
    unsigned int state;
};

extern void init_memory_page();
extern void init_page_state();
extern void unmap_memory_and_state();
extern void init_user_engine();


extern volatile unsigned long *memory_buffer_start_addr;
extern struct hmtt_page_state *page_state_start_add;
extern struct hmtt_page_state *page_state_array;
extern struct evict_struct *evict_engine_start_addr;

unsigned long evict_page_default(unsigned int len);
extern unsigned long evict_page(unsigned long ppn, int pid, char flags, int start_now);
extern int hot_page_lru_control_return(unsigned long ppn);


unsigned long  get_lru_size();


/*for memory management*/
struct page_list{
	unsigned long ppn;
	struct page_list *prev;
	struct page_list *next;
};


struct page_map{
	struct page_list page_list;
	unsigned long last_access_time;
	char in_lru_list; //o means free,  1 means in lru page list
};

extern struct page_map page_map_array[ (128ULL << 18) ];

extern struct page_list lru_head, lru_tail, *scan_ptr;

extern void hot_page_lru_control(unsigned long ppn);

void insert_to_free_list( struct page_list * page_list );

struct page_list* push_scan();
struct page_list* reset_scan();

extern void check_ppn_list();

extern void record_build_page(char flag);

/*******************/


#endif
