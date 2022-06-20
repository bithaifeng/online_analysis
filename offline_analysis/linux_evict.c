#include "config.h"

#define RECLAIM_BATCH 32

#define BITS_PER_LONG 64
//extern unsigned long int_sqrt(unsigned long x);

unsigned long linux_evict_using_page[2] = {0};
struct page_list linux_evict_head[2], linux_evict_tail[2];

unsigned long hit_num_linux_evict = 0, miss_num_linux_evict = 0;

unsigned long free_page_num = 0;

unsigned long free_page_pool[LOCAL_PAGE_SIZE] = {0};
unsigned long read_ptr = 0, write_ptr = 0; 

unsigned long get_free_page_linux_default(){
	if( (write_ptr - read_ptr) == 0) return LOCAL_PAGE_SIZE;
	unsigned long ret = free_page_pool[ read_ptr % LOCAL_PAGE_SIZE ];
	read_ptr ++;
	return ret;
}


static bool inactive_list_is_low(){
	unsigned long gb;
	unsigned long inactive_ratio;
	unsigned long inactive, active;
	inactive = linux_evict_using_page[0];
	active = linux_evict_using_page[1];

	gb = LOCAL_PAGE_SIZE >> 18;
	if (gb)
		inactive_ratio = int_sqrt(10 * gb);
	else
		inactive_ratio = 1;
	return inactive * inactive_ratio < active;
}

static void insert_to_page_list_linux_evict(unsigned long ppn, int active_flag){
	linux_evict_head[active_flag].next->prev = &ppn2page_list[ppn];
	ppn2page_list[ppn].next = linux_evict_head[active_flag].next;
	linux_evict_head[active_flag].next = &ppn2page_list[ppn];
	ppn2page_list[ppn].prev = &linux_evict_head[active_flag];
	linux_evict_using_page[active_flag] ++;
}


void call_linux_evict(){
	printf("bool = %d\n", inactive_list_is_low() );
}

void linux_evict_init(){
	printf("linux init start\n");
	int i = 0;
	for(i = 0; i < LOCAL_PAGE_SIZE; i++){
		free_page_pool[i] = i;
		write_ptr ++;
	}
	for(i = 0; i < 2; i++){
		linux_evict_head[i].next = &linux_evict_tail[i];
		linux_evict_head[i].prev = NULL;
		linux_evict_tail[i].next = NULL;
		linux_evict_tail[i].prev = &linux_evict_head[i];
	}

	for(i = 0; i < MAX_PPN; i++){
//                ppn2groupid[i] = (1ULL << RRIP_BITS);
                ppn2page_list[i].ppn = i;
                ppn2page_list[i].prev = NULL;
                ppn2page_list[i].next = NULL;
        }


	//set bound ppn
        for(i = 0 ; i < USING_PAGE_SIZE; i++)
                vpn2ppn[i] = LOCAL_PAGE_SIZE;
        for(i = 0 ; i < LOCAL_PAGE_SIZE; i++)
                ppn2vpn[i] = USING_PAGE_SIZE;

	unsigned long free_ppn = 0;
	//init memory access pattern
	for(i = 0; i < USING_PAGE_SIZE; i++){
		free_ppn = select_and_return_free_ppn_linux_default();
		vpn2ppn[i] = free_ppn;
		ppn2vpn[free_ppn] = i;

//		if(i % 1000 == 0)
//		printf("init i = %d,  %lu , %lu\n", i, linux_evict_using_page[0], linux_evict_using_page[1]);
		//insert page to active page list
		insert_to_page_list_linux_evict(free_ppn, 0);
		update_access_bit(free_ppn, 0);
//		if(i % LOCAL_PAGE_SIZE == 0)
//			print_analysis_linux_default();
	}
	printf("linux init finish!\n");
}

void scan_update_page_list(int active_flag){
	struct page_list *tmp, *scan_ptr;
	tmp = &linux_evict_head[active_flag];
	tmp = tmp -> next;
	unsigned long ppn = 0;

	while(tmp -> next != NULL){
		scan_ptr = tmp;
		ppn = scan_ptr->ppn;
		tmp = tmp -> next;			
		//check accesstbit
		if(get_access_bit(ppn) == 1){

			update_access_bit(ppn, 0);
			//delete the page and insert it to head
			list_del(scan_ptr);
			linux_evict_using_page[1] --;
	
			if(active_flag == 1)
				insert_to_page_list_linux_evict(ppn, active_flag  );
			else{
				insert_to_page_list_linux_evict(ppn, 1);
			}
		}
		else{
			//no action
		}
	}
}

static void move_page_to_inactive(){
	int i = 0;
	struct page_list *scan_ptr;
	for(i = 0; i < RECLAIM_BATCH;i++){
		scan_ptr = linux_evict_tail[1].prev;
		if(scan_ptr->prev == NULL) return ;
		update_access_bit(scan_ptr->ppn, 0);
		list_del(scan_ptr);
		linux_evict_using_page[1] --;
		insert_to_page_list_linux_evict(scan_ptr->ppn , 0 );
	}

}


void transfer_page_to_inactive(){
	//scan first and move pages to inactive page list
//	scan_update_page_list(1);
//	move_page_to_inactive();	

	int i = 0;
	struct page_list *tmp, *scan_ptr;
        tmp = &linux_evict_tail[1];
        tmp = tmp -> prev;
	scan_ptr = tmp;
	unsigned long ppn = 0;
	int find_old_page = 0;
	for(i = 0; i < RECLAIM_BATCH * 2;i++){
		//
		ppn = scan_ptr->ppn;
		tmp = scan_ptr;
/*
		scan_ptr = scan_ptr->prev;
                list_del( tmp );
                linux_evict_using_page[1] --;
                insert_to_page_list_linux_evict( ppn , 0 );
*/
		if(get_access_bit(ppn) == 1){
			update_access_bit(ppn, 0);
			scan_ptr = scan_ptr->prev;
			list_del(tmp);
			linux_evict_using_page[1] --;
			insert_to_page_list_linux_evict(ppn, 1);
		}
		else{
			find_old_page ++;
			scan_ptr = scan_ptr->prev;
			list_del( tmp );
			linux_evict_using_page[1] --;
			insert_to_page_list_linux_evict( ppn , 0 );
		}
	}
//	if(find_old_page == 0){
//		move_page_to_inactive();
//	}
}

//#define LRU

unsigned long select_and_return_free_ppn_linux_default(){
	//check using_page number first
	if( linux_evict_using_page[0] + linux_evict_using_page[1] < LOCAL_PAGE_SIZE ){
//		linux_evict_using_page[1] ++;
		// insert page to active list
		return linux_evict_using_page[0] + linux_evict_using_page[1];
	}	
//	unsigned long ret = get_free_page_linux_default();
//	if(ret != LOCAL_PAGE_SIZE)
//		return ret;
	//get free page from free page list
		
	//update inactive page list first
//	scan_update_page_list(0);
	//scan and find a oldest page
	
	struct page_list * tmp, *find_ptr, *scan_ptr, *first_find;
	find_ptr = NULL;
	unsigned long ppn = 0;
	tmp = linux_evict_tail[0].prev;
	scan_ptr = tmp;
	first_find = linux_evict_tail[0].prev;

	int i = 0;
	
	for(i = 0; i < RECLAIM_BATCH * 4; i++){
//		ppn = tmp->ppn;
		if(scan_ptr->prev == NULL){
			if(find_ptr == NULL){
				printf("error, no page in inactive list\n");
				return 0;
			}
			return find_ptr->ppn;
		}
		ppn = scan_ptr->ppn;
		tmp = scan_ptr;
		if(get_access_bit(ppn) == 1){
			update_access_bit(ppn, 0);
			scan_ptr = scan_ptr->prev;
			list_del( tmp );
                        linux_evict_using_page[0] --;
#ifdef LRU
			insert_to_page_list_linux_evict(ppn, 0);
#else
			insert_to_page_list_linux_evict(ppn, 1);
#endif
		}
		else{
			if(find_ptr == NULL){
				scan_ptr = scan_ptr->prev;
				list_del( tmp );
				linux_evict_using_page[0] --;
				vpn2ppn[ ppn2vpn[ tmp->ppn ] ] = LOCAL_PAGE_SIZE;
			        ppn2vpn[ tmp->ppn ] = USING_PAGE_SIZE;
				find_ptr = tmp;
			}
		}
//		tmp = tmp->next;
	}
	if(find_ptr == NULL){
		printf("scan %d pages, but no find a page whose access bit equals to 0\n", RECLAIM_BATCH * 4);
		tmp = first_find;
		list_del( tmp );
		linux_evict_using_page[0] --;
		vpn2ppn[ ppn2vpn[ tmp->ppn ] ] = LOCAL_PAGE_SIZE;
                ppn2vpn[ tmp->ppn ] = USING_PAGE_SIZE;
                find_ptr = tmp;
	}


#ifndef LRU
	//check acvtive page list first and decide whether to move page to inactive list
	if( inactive_list_is_low() ){
		//update active page list and transfer pages from active to inactive
		transfer_page_to_inactive();
	}
#endif


//	list_del( tmp );
//	linux_evict_using_page[0] --;
//	vpn2ppn[ ppn2vpn[ tmp->ppn ] ] = LOCAL_PAGE_SIZE;
  //      ppn2vpn[ tmp->ppn ] = USING_PAGE_SIZE;
//	return tmp->ppn;
	return find_ptr->ppn;
	//shrink page from inactive list
}

void check_vpn_linux_default( unsigned long vpn ){
	unsigned long ppn;
	ppn = vpn2ppn[vpn];	
	if( vpn2ppn[vpn] == LOCAL_PAGE_SIZE){
		ppn = select_and_return_free_ppn_linux_default();
		//insert page to list
		ppn2vpn[ppn] = vpn;
                vpn2ppn[vpn] = ppn;
		update_access_bit(ppn, 0);
		insert_to_page_list_linux_evict(ppn, 0);
		

		miss_num_linux_evict ++; 


	}
	else{
		//update access bit
		update_access_bit(ppn, 1);
		hit_num_linux_evict ++;




	}

}


void update_access_bit(unsigned long ppn, unsigned char flag){
	ppn2state[ppn].access_bit = flag;	
}

void update_active_bit(unsigned long ppn, unsigned char flag){
        ppn2state[ppn].active_bit = flag;
}

unsigned char get_access_bit(unsigned long ppn){
	return ppn2state[ppn].access_bit;
}

void print_analysis_linux_default(){
	int i = 0;
//	for( i = 0; i < 2; i++){
	printf("inactive list = %lu, active list = %lu, all = %lu\n", linux_evict_using_page[0], linux_evict_using_page[1], linux_evict_using_page[0] + linux_evict_using_page[1]);
	printf("miss_num = %lu, hit_num = %lu, sum = %lu\n", miss_num_linux_evict, hit_num_linux_evict, miss_num_linux_evict + hit_num_linux_evict );
	miss_num_linux_evict = 0;
	hit_num_linux_evict = 0;

//	}
}


/**
 * int_sqrt - rough approximation to sqrt
 * @x: integer of which to calculate the sqrt
 *
 * A very rough approximation to the sqrt() function.
 */
unsigned long int_sqrt(unsigned long x)
{
	unsigned long b, m, y = 0;

	if (x <= 1)
		return x;

	m = 1UL << (BITS_PER_LONG - 2);
	while (m != 0) {
		b = y + m;
		y >>= 1;

		if (x >= b) {
			x -= b;
			y += m;
		}
		m >>= 2;
	}

	return y;
}

