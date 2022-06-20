#include "prefetch_mine.h"
#include "config.h"
#include "lt_profile.h"

#define TABLESIZE (1024 * 16 * 16) //1024~ 32KB
#define WAY_NUM 2
#define SET_NUM ( TABLESIZE / WAY_NUM )

struct struct_filter_table{
	unsigned int ppn_and_num; // ppn~6+18=24 bits, num = 7 bits, lru bit  1 bits, all use  32 bits
};

unsigned int filter_table_array[ TABLESIZE ] = {0} ;


#define ACCESS_NUM 8

void filter_table( unsigned long p_addr, unsigned long tt ){
	int ret = -1;
        unsigned long ppn = p_addr >> 12;
	unsigned long set_id = ppn % SET_NUM;
	unsigned long tmp_ppn;
	unsigned char tmp_num;
	int i = 0;
	unsigned evict_id = 0;
	for(i = 0; i < WAY_NUM; i++){
		tmp_num = filter_table_array[set_id * WAY_NUM + i] & 0xff;
		tmp_ppn = filter_table_array[set_id * WAY_NUM + i] >> 8;

		if( (tmp_num >> 7) == 0)
			evict_id = i;

		if(tmp_ppn == ppn){
			//check num
			if(tmp_num >= ACCESS_NUM){
				//extract

				if(duration_all - new_ppn[ppn].timer >= (1ULL << 10))
					store_to_tb(ppn, tt);
				new_ppn[ppn].timer = duration_all;


			}
			else{
				filter_table_array[set_id * WAY_NUM + i] ++;
			}
			//update two lru bit
			filter_table_array[set_id * WAY_NUM + i] = ( filter_table_array[set_id * WAY_NUM + i] | 0x80 );
			filter_table_array[set_id * WAY_NUM + !i ] = ( filter_table_array[set_id * WAY_NUM + !i] & 0xFFFFFF7F );
			return ;
		}
	}
	// new entry of ppn
	filter_table_array[set_id * WAY_NUM + evict_id] = (ppn << 8) | (0x81);
	filter_table_array[set_id * WAY_NUM + !evict_id ] = ( filter_table_array[set_id * WAY_NUM + !evict_id] & 0xFFFFFF7F );
}

