#include "config.h"


#include <vector>
#include <map>

using namespace std;

struct reuse_entry{
	unsigned long distence;
	unsigned long scan_id;
};

map<unsigned long, vector<struct reuse_entry> > vpn2reuse;
map<unsigned long, unsigned long> vpn2lastaccess;
map<unsigned long, unsigned long> vpn2usetimes;


int max_len = 0;
int build_reuse_map_flag = 0;

void build_reuse_map( unsigned long *start_addr, unsigned long size ){
	unsigned long  vpn, tmp_distence;
	int i = 0;
	struct reuse_entry tmp_reuse_entry;
	for(i = size - 1 ; i >= 0; i --){
		vpn = start_addr[i];
		if( vpn2lastaccess[vpn] != 0){
			tmp_distence = vpn2lastaccess[vpn] - i;
			tmp_reuse_entry.distence = tmp_distence;
			tmp_reuse_entry.scan_id = i;
			vpn2reuse[vpn].insert(  vpn2reuse[vpn].begin(), tmp_reuse_entry );
//			vpn2reuse[vpn].push_back( tmp_reuse_entry );
			if(max_len < vpn2reuse[vpn].size()){
				max_len = vpn2reuse[vpn].size();
			}

		}
		else{
			tmp_reuse_entry.distence = MAX_REUSE;
                        tmp_reuse_entry.scan_id = i;
			vpn2reuse[vpn].insert(  vpn2reuse[vpn].begin(), tmp_reuse_entry );
//                        vpn2reuse[vpn].push_back( tmp_reuse_entry );
		}
		vpn2lastaccess[vpn] = i;
	}
	printf("max_len = %d\n", max_len);
}


unsigned long hit_num_opt = 0, miss_num_opt = 0;



struct opt_obj{
	unsigned long vpn;
	unsigned long reuse_dis;
	unsigned long current_time;
	struct opt_obj * prev, *next;
};


unsigned long check_reuse_dis(unsigned long *start_addr, unsigned long size, int now_id, unsigned long scan_vpn){
	int i = 0;
	unsigned long ret = MAX_REUSE;
	unsigned long last_id = vpn2usetimes[scan_vpn];
	vpn2usetimes[scan_vpn] ++;;

	if(last_id >= vpn2reuse[scan_vpn].size())
		printf("error\n");
	ret = vpn2reuse[scan_vpn][last_id].distence;
	return ret;


#if 0
	for( i = 0 ; i < vpn2reuse[scan_vpn].size(); i++){
		if( vpn2reuse[scan_vpn][i].scan_id == now_id ){
			ret = vpn2reuse[scan_vpn][i].distence;
			return ret;
		}
	}
	return ret;
	
	for(i = now_id + 1; i < size; i++){
		if( scan_vpn == start_addr[i]){
			if(i - now_id < ret){
				ret = i - now_id;
				return ret;
			}
		}
	}
#endif
	return ret;
}




unsigned long select_and_return_free_ppn_opt(unsigned current_scan_id, unsigned long *start_address, unsigned long size){
	unsigned long ret = 0;
	int i = 0;
	unsigned long ppn = 0, vpn = 0;
	unsigned long last_reuse_dis = 0, last_id = 0;
	unsigned long tmp_reuse = 0;
	for(i = 0 ; i < LOCAL_PAGE_SIZE; i++){
		vpn = ppn2vpn[i];
		tmp_reuse = check_reuse_dis(start_address, size, current_scan_id, vpn);
		if(tmp_reuse > last_reuse_dis){
			last_reuse_dis = tmp_reuse;
			last_id = i;
			if(last_reuse_dis == MAX_REUSE)
				return i;
		}
	}
	return ret;

}


void check_vpn_opt(unsigned long vpn, unsigned long current_scan_id, unsigned long *start_address, unsigned long size){
	if(build_reuse_map_flag == 0){
		build_reuse_map(start_address, size);
		build_reuse_map_flag = 1;
	}
	unsigned long ppn;
        ppn = vpn2ppn[vpn];
	if(current_scan_id % 1000 == 0){
		;
//		printf("id = %d\n",current_scan_id);
//		print_analysis_opt();
	}

	if( vpn2ppn[vpn] == LOCAL_PAGE_SIZE){
		miss_num_opt ++;
		ppn = select_and_return_free_ppn_opt(current_scan_id, start_address, size); 
		// clear old pte
		vpn2ppn[ ppn2vpn[ppn] ] = LOCAL_PAGE_SIZE;
		ppn2vpn[ppn] = vpn;
		vpn2ppn[vpn] = ppn;

	}
	else{
		hit_num_opt ++;
	}
}


void print_analysis_opt(){
        int i = 0;
	printf("miss_num = %lu, hit_num = %lu, sum = %lu\n", miss_num_opt, hit_num_opt, miss_num_opt + hit_num_opt );
        miss_num_opt = 0;
        hit_num_opt = 0;

}


void init_opt(){
	int i = 0;
	for(i = 0 ; i < USING_PAGE_SIZE; i++)
                vpn2ppn[i] = LOCAL_PAGE_SIZE;
        for(i = 0 ; i < LOCAL_PAGE_SIZE; i++)
                ppn2vpn[i] = USING_PAGE_SIZE;



	for(i = USING_PAGE_SIZE - LOCAL_PAGE_SIZE; i < USING_PAGE_SIZE; i++)
	{
		vpn2ppn[i] = i - ( USING_PAGE_SIZE - LOCAL_PAGE_SIZE );
		ppn2vpn[ i - ( USING_PAGE_SIZE - LOCAL_PAGE_SIZE ) ] = i;
	}
}
