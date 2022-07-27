#define _GNU_SOURCE
#include "config.h"
#include "rrip.h"
#include "prefetch_mine.h"

using namespace std;

#define MAX_PPN (64ULL << 18)

unsigned char ppn2groupid[MAX_PPN] = {0};
// page_map_array[idx].page_list.
/*for each rrip group*/
struct page_list head_rrip[1 << RRIP_BITS], tail_rrip[1 << RRIP_BITS];
long long maintain_number[ 1 << RRIP_BITS ] = {0};


void init_rrip_structure(){
	int i = 0;
	for(i = 0 ; i < MAX_PPN; i++){
		ppn2groupid[i] = (1ULL << RRIP_BITS);	
	}
	for(i = 0 ; i < (1 << RRIP_BITS); i++){
		head_rrip[i].prev = NULL;
		head_rrip[i].next = &tail_rrip[i];
		tail_rrip[i].prev = &head_rrip[i];
		tail_rrip[i].next = NULL;	
	}
}



void insert_page(unsigned long ppn, int group_id){
	ppn2groupid[ppn] = group_id;
		
	head_rrip[group_id].next->prev = &page_map_array[ppn].page_list;
	page_map_array[ppn].page_list.next = head_rrip[group_id].next;
	
	head_rrip[group_id].next = &page_map_array[ppn].page_list;
	page_map_array[ppn].page_list.prev = &head_rrip[group_id];
	maintain_number[group_id] ++;
	lt_mem_using ++;
}



int check_group_id(unsigned long ppn){
	int id = 0;
	id = ppn2groupid[ppn];		
	return id;
}

void decrease_rrip(unsigned long ppn){
	ppn2groupid[ppn] --;
}

void update_page_group(unsigned long ppn, int from_id, int to_id){
	//update the least rrip's group or just last 1 group.
}

void change_pnn_group(unsigned long ppn, int target_id){
	list_del( &page_map_array[ppn].page_list );
	insert_page( ppn,  target_id);
//	mem_using --;
	lt_mem_using --;
}



int pf2 = 0;
void check_and_update_page(unsigned long ppn){
	int now_group_id = check_group_id( ppn );
	if(now_group_id == ((1ULL << RRIP_BITS)) ){
		//new page, insert page to list with RRIP value 
		insert_page(ppn, INSERT_VALUE );
		return ;	
	}

	if(now_group_id == 0){
		//skip
#ifdef USING_LRU_RRIP

#endif
		return ;
	}
	if( now_group_id > 0 && now_group_id < (1ULL << RRIP_BITS) ){
	//decrease its RRIP value
		decrease_rrip(ppn);
		change_pnn_group(ppn, ppn2groupid[ppn] );
	}
	else{
		if(pf2 <= 5){
			printf(" error group_id = %d\n ", now_group_id);
			pf2 ++;
		}
	}
}

int hot_page_rrip_control_return(unsigned long ppn){
	int ret = -1;
	//check whether it is a new page?
	if(ppn2groupid[ppn] == (1 << RRIP_BITS)){
		//new page
		insert_page(ppn, INSERT_VALUE );
		//means NEW_PAGE
		return 2;
	}
	else{
		check_and_update_page(ppn);
		// means hit
		return 1;
	}
	return 0;
}

int rotate_rrip_group(){
	int i = 0;
	int ret_all = 0;

	for(i = (1 << RRIP_BITS) - 1; i > 0; i --){
		if( maintain_number[i - 1] != 0)
			ret_all = 1;
		maintain_number[i] = maintain_number[i - 1];
		
//		head[i].prev = head[i - 1].prev;
		head_rrip[i].next = head_rrip[i - 1].next;
		head_rrip[i - 1].next->prev = &head_rrip[i];

		tail_rrip[i].prev = tail_rrip[i - 1].prev;
		tail_rrip[i].prev->next = &tail_rrip[i];
//		tail[i].next = tail[i - 1].next;

		//clear 
		head_rrip[i - 1].next = &tail_rrip[i - 1];
		tail_rrip[i - 1].prev = &head_rrip[i - 1];
		maintain_number[i - 1] = 0;


		//increase
		struct page_list *tmp;
		tmp = &head_rrip[i];
		tmp = tmp->next;			
		while( tmp->next != NULL ){
			ppn2groupid[ tmp->ppn ] ++;
			if( ppn2groupid[ tmp->ppn ] >= (1ULL << RRIP_BITS) )
				printf("error groupid = %d\n", ppn2groupid[ tmp->ppn ]);

			tmp = tmp->next;
		}
	}
	maintain_number[0] = 0;
	return ret_all;
}

int flag_p = 0;
struct page_list* get_tail_and_delete(int group_id){
	if(maintain_number[group_id] == 0){
		printf("error, groupid = %d \n", group_id);
		return NULL;
	}

	struct page_list* tmp;
	tmp = tail_rrip[group_id].prev;	
	list_del( tmp );
	maintain_number[group_id] --;
	ppn2groupid[tmp->ppn] = (1 << RRIP_BITS); //reset RRIP

	if(maintain_number[group_id] < 0 && flag_p <=5){
		flag_p ++;
		printf("error in get_tail_and_delete, group id = %d, num = %ld\n", group_id , maintain_number[group_id]);
	}

	return tmp;
}


unsigned long get_ppn_from_rrip(){
	unsigned long ppn = 0;
	unsigned long ret = 0;
	while(maintain_number[(1 << RRIP_BITS) - 1] == 0){
		//decrement RRIP for each group
		if( rotate_rrip_group() == 0){
			//no page in list
			return ret;
		}
	}
	//delete a page from largest RRIP list
	struct page_list * page;
//	page = get_tail_and_delete( &tail[ (1 << RRIP_BITS) - 1 ] );
	page = get_tail_and_delete( (1 << RRIP_BITS) - 1 );
	if (page == NULL){
		printf("Error!\n");
		return 0;
	}
//	mem_using --;
//	lt_mem_using --;
	return page->ppn;
}

void printf_rrip_state(){
	int i = 0;
	for(i = 0; i < (1 << RRIP_BITS) - 1; i++){
		printf("rrip id = %d, number = %lu\n", i, maintain_number[i]);
	}

}


