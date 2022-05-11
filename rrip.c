#define _GNU_SOURCE
#include "rrip.h"

using namespace std;

#define MAX_PPN (64ULL << 18)

unsigned char ppn2groupid[MAX_PPN] = {0};
std::queue<unsigned long> rripQueue[ (1ULL << RRIP_BITS) - 1];



void init_rrip_structure(){
	int i = 0;
	for(i = 0; i < MAX_PPN; i++){
		ppn2groupid[i] = (1ULL << RRIP_BITS);
	}

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


void check_and_update_page(unsigned long ppn){
	int now_group_id = check_group_id( ppn );
	if(now_group_id == ((1ULL << RRIP_BITS)) ){
		//new page, insert page to list with RRIP value 
	
		return ;	
	}

	if(now_group_id == 0){
		//skip
		return ;
	}

	//decrease its RRIP value
	decrease_rrip(ppn);
	

}


