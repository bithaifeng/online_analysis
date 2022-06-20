//#define _GNU_SOURCE
//#include "rrip.h"
#include "config.h"


using namespace std;

/* for temply pagetable*/
//unsigned long vpn2ppn[USING_PAGE_SIZE] = {0};
//unsigned long ppn2vpn[USING_PAGE_SIZE] = {0};
//struct page_state ppn2state[USING_PAGE_SIZE] = {0};



/******************/

unsigned long hit_num = 0, miss_num = 0;



int ppn2groupid[MAX_PPN] = {0};



/*for each rrip group*/
struct page_list head[1 << RRIP_BITS], tail[1 << RRIP_BITS];
long long maintain_number[ 1 << RRIP_BITS ] = {0};
//struct page_list ppn2page_list[ MAX_PPN ];


long long mem_using = 0;


/********************/




void init_rrip_structure(){
	int i = 0;
	for(i = 0; i < MAX_PPN; i++){
		ppn2groupid[i] = (1ULL << RRIP_BITS);
		ppn2page_list[i].ppn = i;
		ppn2page_list[i].prev = NULL;
		ppn2page_list[i].next = NULL;
	}

	for(i = 0 ; i < (1 << RRIP_BITS); i++){
		head[i].prev = NULL;
		head[i].next = &tail[i];
		tail[i].prev = &head[i];
		tail[i].next = NULL;	
	}

	//set bound ppn
	for(i = 0 ; i < USING_PAGE_SIZE; i++)
		vpn2ppn[i] = LOCAL_PAGE_SIZE;
	for(i = 0 ; i < LOCAL_PAGE_SIZE; i++)
		ppn2vpn[i] = USING_PAGE_SIZE;

}


int check_group_id(unsigned long ppn){
	int id = 0;
	id = ppn2groupid[ppn];		
	return id;
}

void decrease_rrip(unsigned long ppn){
	maintain_number[ ppn2groupid[ppn] ] --;
	ppn2groupid[ppn] --;
}

void change_pnn_group(unsigned long ppn, int target_id){
	list_del( &ppn2page_list[ppn] );
	insert_page( ppn,  target_id);
	mem_using --;

}


void update_page_group(unsigned long ppn, int from_id, int to_id){
	//update the least rrip's group or just last 1 group.
	

}

void check_vpn(unsigned long vpn){
	unsigned long ppn;
	if( vpn2ppn[vpn] == LOCAL_PAGE_SIZE){
		//reclaim one page and   ,insert new to it.
		ppn = select_and_return_free_ppn();
//		insert_page(ppn, (1ULL << RRIP_BITS) - 2 );
//		insert_page(ppn, (1ULL << RRIP_BITS) - 3 );
		insert_page(ppn, INSERT_VALUE );
		ppn2vpn[ppn] = vpn;
		vpn2ppn[vpn] = ppn;
		miss_num ++;
	}
	else{
		check_and_update_page( vpn2ppn[vpn] );
		hit_num ++;
	}
}

int pf2 = 0;

#define lru_0

void check_and_update_page(unsigned long ppn){
	int now_group_id = check_group_id( ppn );
	if(now_group_id == ((1ULL << RRIP_BITS)) ){
		//new page, insert page to list with RRIP value 
		printf("error in check_group_id\n");
		insert_page(ppn, (1ULL << RRIP_BITS) - 2 );
		return ;	
	}

	if(now_group_id == 0){
		//skip
#ifdef lru_0
		//update lru list
		list_del( &ppn2page_list[ppn] );
		maintain_number[ ppn2groupid[ppn] ] --;
	        mem_using --;
		insert_page(ppn, 0 );
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

void rotate_rrip_group(){
	int i = 0;
	for(i = (1 << RRIP_BITS) - 1; i > 0; i --){
		maintain_number[i] = maintain_number[i - 1];
		
//		head[i].prev = head[i - 1].prev;
		head[i].next = head[i - 1].next;
		head[i - 1].next->prev = &head[i];

		tail[i].prev = tail[i - 1].prev;
		tail[i].prev->next = &tail[i];
//		tail[i].next = tail[i - 1].next;

		//clear 
		head[i - 1].next = &tail[i - 1];
		tail[i - 1].prev = &head[i - 1];
		maintain_number[i - 1] = 0;


		//increase
		struct page_list *tmp;
		tmp = &head[i];
		tmp = tmp->next;			
		while( tmp->next != NULL ){
			ppn2groupid[ tmp->ppn ] ++;
			if( ppn2groupid[ tmp->ppn ] >= (1ULL << RRIP_BITS) )
				printf("error groupid = %d\n", ppn2groupid[ tmp->ppn ]);

			tmp = tmp->next;
		}
	}
	maintain_number[0] = 0;
}

void list_del( struct page_list * page_list ){

    page_list->prev->next = page_list->next;
    page_list->next->prev = page_list->prev;

    page_list->next = NULL;
    page_list->prev = NULL;
}


//struct page_list* get_tail_and_delete(struct page_list * page_list){

int flag_p = 0;
struct page_list* get_tail_and_delete(int group_id){
	if(maintain_number[group_id] == 0){
		printf("error, groupid = %d \n", group_id);
		return NULL;
	}

	struct page_list* tmp;
	tmp = tail[group_id].prev;	
	list_del( tmp );
	maintain_number[group_id] --;

	if(maintain_number[group_id] < 0 && flag_p <=5){
		flag_p ++;
		printf("error in get_tail_and_delete, group id = %d, num = %ld\n", group_id , maintain_number[group_id]);
	}

	return tmp;
}






unsigned long select_and_return_free_ppn(){
	while(maintain_number[(1 << RRIP_BITS) - 1] == 0){
		//decrement RRIP for each group
		rotate_rrip_group();
	}
	//delete a page from largest RRIP list
	struct page_list * page;
//	page = get_tail_and_delete( &tail[ (1 << RRIP_BITS) - 1 ] );
	page = get_tail_and_delete( (1 << RRIP_BITS) - 1 );
	if (page == NULL){
		printf("Error!\n");
		return 0;
	}
	//remove ppn2vpn and vpn2ppn
	vpn2ppn[ ppn2vpn[ page->ppn ] ]	= LOCAL_PAGE_SIZE;
	ppn2vpn[ page->ppn ] = USING_PAGE_SIZE;
	mem_using --;
	return page->ppn;
}

void init_logicl_access(){
	int i = 0;
	for(i = 0; i < LOCAL_PAGE_SIZE; i++){
		ppn2vpn[i] = i;
		vpn2ppn[i] = i;
		//insert page to list
		insert_page(i, (1 << RRIP_BITS) - 2);
	}

	printf("init local memory success !\n");
	mem_using = LOCAL_PAGE_SIZE;
	print_analysis();

	printf("\n");

	//init other page
	unsigned long free_ppn = 0;
	for(i = LOCAL_PAGE_SIZE; i < USING_PAGE_SIZE; i++){
		free_ppn = select_and_return_free_ppn();

		vpn2ppn[i] = free_ppn;
		ppn2vpn[free_ppn] = i;
		insert_page( free_ppn, (1 << RRIP_BITS) - 2 );

		if(i % (LOCAL_PAGE_SIZE) == 500)
			print_analysis();

	}
}


void insert_page(unsigned long ppn, int group_id){
	ppn2groupid[ppn] = group_id;
		
	head[group_id].next->prev = &ppn2page_list[ppn];
	ppn2page_list[ppn].next = head[group_id].next;
	
	head[group_id].next = &ppn2page_list[ppn];
	ppn2page_list[ppn].prev = &head[group_id];
	maintain_number[group_id] ++;
	mem_using ++;
}

void print_analysis(){
	int i = 0;
	for(i = 0; i < ((1ULL << RRIP_BITS)); i++){
                printf("maintains[%d] = %ld\n", i,maintain_number[i]);
        }
	printf("miss_num = %lu, hit_num = %lu\n", miss_num, hit_num);
	miss_num = 0; hit_num = 0;
	printf("mem_using = %ld\n", mem_using);
}

