
#include "/home/lhf/lhf/hmtt_kernel_control/hmtt_kernel.h"
#include "prefetch_mine.h"
#include "page_table.h"
#include "/home/lhf/receiver_driver/cfg_content.h"

#include "config.h"


#define PAGE_SIZE (4*1024)
#define BUF_SIZE (32*PAGE_SIZE)
#define STATE_BUFFER_SIZE ((128ULL << 18) + 3 )


#include "lt_profile.h"


#include <mutex>
#include "lightswapinterface.h"




std::mutex lru_lock;

/******for memory management********/
struct page_map page_map_array[ (128ULL << 18) ] = {0};
struct page_list lru_head, lru_tail, *scan_ptr;


unsigned long lt_mem_using = 0;

unsigned long  get_lru_size(){
    return lt_mem_using;
}

void insert_to_page_list_head( struct page_list * page_list ){
    if(page_list == &lru_head || page_list == &lru_tail)
        return;

    lt_mem_using++;
	(&lru_head)->next->prev = page_list;
	page_list->next = (&lru_head)->next;
	
	(&lru_head)->next = page_list;
	page_list->prev = &lru_head;
}

void list_del( struct page_list * page_list ){
    if(page_list == &lru_head || page_list == &lru_tail)
        return;
    

    lt_mem_using--;
	page_list->prev->next = page_list->next;
    page_list->next->prev = page_list->prev;

    page_list->next=NULL;
    page_list->prev=NULL;
}

void update_lru(unsigned long ppn){
    if(ppn >= (128ULL << 18))
        return;
    
    // page is in lru, first isolate it
    if(page_map_array[ppn].page_list.next != NULL || page_map_array[ppn].page_list.prev != NULL){
        if(page_map_array[ppn].page_list.next == NULL || page_map_array[ppn].page_list.prev == NULL){
            printf("\nLT: LRU BUG!!!\n");
        }

#ifdef use_userspace_lru_alg
        list_del(&page_map_array[ppn].page_list);
#else
        // just fifo
        // lru_lock.unlock();
        return;
#endif
    }
    
    insert_to_page_list_head(&page_map_array[ppn].page_list);
    // lru_lock.unlock();
}

int  update_lru_return(unsigned long ppn){
    int ret = -1;
    if(ppn >= (128ULL << 18))
        return 3;

    // page is in lru, first isolate it
    if(page_map_array[ppn].page_list.next != NULL || page_map_array[ppn].page_list.prev != NULL){
        if(page_map_array[ppn].page_list.next == NULL || page_map_array[ppn].page_list.prev == NULL){
            printf("\nLT: LRU BUG!!!\n");
        }

        list_del(&page_map_array[ppn].page_list);
	ret = 1;// means hit
    }
    else
	ret = 2; // means the page is a new page

    insert_to_page_list_head(&page_map_array[ppn].page_list);

    return ret;

    // lru_lock.unlock();
}



unsigned int scan_ptr_counter = 0;

struct page_list* push_scan(){
    // lru_lock.lock();

    scan_ptr_counter++;
    scan_ptr = scan_ptr->prev;
    // lru_lock.unlock();

    if(scan_ptr==NULL || scan_ptr==&lru_head || scan_ptr_counter > 2*MIN_PAGE_NUM)
        reset_scan();
    return scan_ptr;
}

struct page_list* reset_scan(){
    scan_ptr_counter = 0;
    // lru_lock.lock();
    scan_ptr = lru_tail.prev;
    // lru_lock.unlock();
    return scan_ptr;
}


//#define UPDATE_INTER_TIME ( 4ULL << 24 )
 #define UPDATE_INTER_TIME ( 2400 * 10 )

//for each hot virtual page or hot physical page
void hot_page_lru_control(unsigned long ppn){
	if(duration_all -  page_map_array[ppn].last_access_time >= UPDATE_INTER_TIME)
	{
        statics_datatype2(page_state_array[ppn].state);

        if(scan_ptr->ppn == ppn){
            reset_scan();
        }

		//check page is used?
	    // if( lt_check_page_charging(ppn)){
	    if( lt_check_page_inuse(ppn)){
                update_lru(ppn);
        }
		page_map_array[ppn].last_access_time = duration_all;
	}
}


int hot_page_lru_control_return(unsigned long ppn){
	int ret = -1;
	if(page_map_array[ppn].page_list.next == NULL && page_map_array[ppn].page_list.prev == NULL){
		// new page
		insert_to_page_list_head(&page_map_array[ppn].page_list);
		
		ret = 2; //
		page_map_array[ppn].last_access_time = duration_all;
		return ret;
	}


        if(duration_all -  page_map_array[ppn].last_access_time >= UPDATE_INTER_TIME)
        {
        	statics_datatype2(page_state_array[ppn].state);

                //check page is used?
            // if( lt_check_page_charging(ppn)){
	        if( lt_check_page_inuse(ppn)){
	                ret = update_lru_return(ppn);
	        }
                page_map_array[ppn].last_access_time = duration_all;
	}
	else ret = 5;
	return ret;
}



/*****************************/



volatile unsigned long *memory_buffer_start_addr = NULL;
struct hmtt_page_state *page_state_start_addr = NULL;
struct hmtt_page_state *page_state_array = NULL;
struct evict_struct *evict_engine_start_addr = NULL;


int fd, fd1, fd2;


unsigned long evict_single_page(unsigned long ppn, int pid, char flag  ){
	return 0;
}





const int evict_id_use = 0;
int evict_len_use = 0;

unsigned long evict_page_default(unsigned int len){

	// unsigned long ret = syscall(336,  ltarg_max_evictor_size, 32);
    unsigned long ret = make_reclaim_page(ltarg_max_evictor_size, len);

    return ret;
}

// 0 for ppn
unsigned long evict_page(unsigned long ppn, int pid, char flags, int start_now){
	unsigned long ret;
    unsigned int idx=0;

	//if(flags == 0 && && page_state_array[ppn].state == 0){
	//	return 0;
	//}
	
    struct evict_struct * task=&evict_engine_start_addr[evict_id_use * 32];
	task[evict_len_use ].flags = flags;
	task[evict_len_use ].pid = pid;
	task[evict_len_use ].page_num = ppn;
	evict_len_use ++;
	if(evict_len_use == 32){
		//syscall evict
        ret = make_reclaim_page(evict_id_use, 32);
		// ret = syscall(336,  evict_id_use, 32);
		// evict_id_use = (evict_id_use + 1) % 4;
		evict_len_use = 0;

        // unsigned int idx=0;
        // for(idx=0; idx<32;idx++){

        //     if(task[ idx].flags == 0  && task[idx].result != 0){
        //         if( lt_check_page_charging(task[idx].page_num)){
        //             update_lru(task[idx].page_num);
        //         }
        //     }
        // }
		return ret;
	}
	if(start_now == 1){
        ret = make_reclaim_page(evict_id_use, evict_len_use);
		// ret = syscall(336,  evict_id_use, evict_len_use);

        // for(idx=0; idx<evict_len_use;idx++){

        //     if(task[ idx].flags == 0  && task[idx].result != 0){
        //         if( lt_check_page_charging(task[idx].page_num)){
        //             update_lru(task[idx].page_num);
        //         }
        //     }
        // }

                // evict_id_use = (evict_id_use + 1) % 4;
                evict_len_use = 0;
                return ret;
	}
	return 0;
}


void init_memory_page(){
	int *brk;

	fd = open("/dev/lt_init_data", O_RDWR);
	if (fd < 0) {
		perror("open failed\n");
		exit(-1);
	}

	memory_buffer_start_addr = (unsigned long*)mmap(NULL, STATE_BUFFER_SIZE * sizeof(unsigned long), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, 0);
	if (!memory_buffer_start_addr) {
		perror("mmap failed\n");
		exit(-1);
	}
}

void init_page_state(){

	char *lt_page_state_filename = "/dev/lt_page_state";
	fd1 = open(lt_page_state_filename, O_RDWR);
        if (fd1 < 0) {
                perror("open2 failed\n");
                exit(-1);
        }

        page_state_start_addr = (struct hmtt_page_state*)mmap(NULL, STATE_BUFFER_SIZE * sizeof(struct hmtt_page_state)  , PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd1, 0);
        if (!page_state_start_addr) {
                perror("mmap failed\n");
                exit(-1);
        }
//	page_state_array = page_state_start_addr + 3;
	page_state_array = page_state_start_addr;

}

void init_evict_engin(){

	fd2 = open("/dev/lt_arg_state", O_RDWR);
	if (fd2 < 0) {
                perror("open3 failed\n");
                exit(-1);
        }
		
	evict_engine_start_addr = (struct evict_struct*)mmap(NULL, EVICT_ENGIN_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd2, 0);
	
}

void init_user_engine(){


	lru_head.next = &lru_tail;
	lru_head.prev = NULL;
	lru_tail.next = NULL;
	lru_tail.prev = &lru_head;
	scan_ptr = &lru_tail;
	
	init_memory_page();
        init_page_state();
	init_evict_engin();

    
    for(unsigned long idx=0; idx<(128ULL<<18); idx++){
        page_map_array[idx].page_list.ppn = idx;
        page_map_array[idx].page_list.next = NULL;
        page_map_array[idx].page_list.prev = NULL;
        page_map_array[idx].in_lru_list = 0;
    }

}

void unmap_memory_and_state(){
	munmap( memory_buffer_start_addr, STATE_BUFFER_SIZE * sizeof(unsigned long) );
	munmap( page_state_start_addr, STATE_BUFFER_SIZE * sizeof(struct hmtt_page_state) );
	munmap( page_state_start_addr, EVICT_ENGIN_SIZE );

	close( fd );
	close( fd1 );
	close( fd2 );
}


unsigned long build_page_list_time = 0;
unsigned long using_times_build =0;
unsigned long last_time;

void check_ppn_list(){
	//memory_buffer_start_addr
	unsigned long i  = 0;
	unsigned long tmp_ppn;

	unsigned long in_Page_list = 0;
	unsigned long not_found_in_list = 0;

	for(i = 0; i < memory_buffer_start_addr[0] ; i ++){
		tmp_ppn = memory_buffer_start_addr[i + 3];
		if( page_map_array[tmp_ppn].page_list.next == NULL && page_map_array[tmp_ppn].page_list.prev == NULL){
			not_found_in_list ++;
		}
		else {
			in_Page_list++;
		}

	}	
	printf("in_Page_list = %lu, not_found_in_list = %lu\n  ", in_Page_list ,not_found_in_list);
	printf("average build cycle = %.3lf, building times = %lu\n",  (double)build_page_list_time / (double)using_times_build, using_times_build );
}


static inline __u64 get_cycles(void)
{
        __u32 timehi, timelo;
        asm("rdtsc":"=a"(timelo),"=d"(timehi):);
        return (__u64)(((__u64)timehi)<<32 | (__u64)timelo);
}





void record_build_page(char flag){
	if(flag == 0)
		last_time = get_cycles();
	else{
		using_times_build ++;
		build_page_list_time += (get_cycles() - last_time);
	}
	
}



