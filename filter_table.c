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

// for small cache 2-ways N sets
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

#if 0
				if(duration_all - new_ppn[ppn].timer >= (1ULL << 10))
					store_to_tb(ppn, tt);
				new_ppn[ppn].timer = duration_all;
#endif
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


#define RESERVED_BITS 4

void multi_filter_table( unsigned long p_addr, unsigned long tt ){
#ifdef USING_BITMAP
	int ret = -1;
        unsigned long ppn = p_addr >> 12;
	unsigned long cache_line_offset = (p_addr & 0xfff) >> 6;
//	printf("cache_line_offset = %lu, p_addr = %lu\n", cache_line_offset, p_addr);
	if(cache_line_offset >= (64 - RESERVED_BITS)){
		cache_line_offset = 64 - RESERVED_BITS - 1;
	}

	unsigned long value = new_ppn[ppn].bitmap;
//	printf("cache_line_offset = %lu\n", cache_line_offset);
	//printf("** bitmap = 0x%lx, (1 << cache_line_offset) = 0x%lx, and ans = 0x%lx\n", new_ppn[ppn].bitmap, (1 << cache_line_offset), new_ppn[ppn].bitmap & (1 << cache_line_offset));	
	if( (new_ppn[ppn].bitmap & (1ULL << cache_line_offset)) > 0){
		// the bit has been set
		;
	}
	else{
		//set bit
//		printf("set bit\n");
		new_ppn[ppn].bitmap = ( new_ppn[ppn].bitmap | (1ULL << cache_line_offset) );
	}
//	printf("bitmap = 0x%lx, (1 << cache_line_offset = %d)\n", new_ppn[ppn].bitmap, 1 << cache_line_offset);
	
	// reach max access number, only update access bit
	if( ( new_ppn[ppn].bitmap >> (64 - RESERVED_BITS)  ) == (1 << RESERVED_BITS) - 1){
		;
	}
	else{
//		printf(" mask = 0x%lx \n", (1ULL << (64 - RESERVED_BITS)) - 1 );
//		printf("part 1 = 0x%lx, part2 = 0x%lx\n", (new_ppn[ppn].bitmap & ( (1ULL << (64 - RESERVED_BITS)) - 1) ), ( ( (new_ppn[ppn].bitmap >> (64 - RESERVED_BITS))  + 1) << (64 - RESERVED_BITS)) );
//		printf("stage0 = 0x%lx, 0x%lx, 0x%lx\n", new_ppn[ppn].bitmap >> (64 - RESERVED_BITS), (new_ppn[ppn].bitmap >> (64 - RESERVED_BITS)) + 1,
//			( ( (new_ppn[ppn].bitmap >> (64 - RESERVED_BITS))  + 1 ) << (64 - RESERVED_BITS)) );
		new_ppn[ppn].bitmap = ((new_ppn[ppn].bitmap & ((1ULL << (64 - RESERVED_BITS)) - 1) ) | ( ( (new_ppn[ppn].bitmap >> (64 - RESERVED_BITS))  + 1) << (64 - RESERVED_BITS) ) );
//		printf("bitmap = 0x%lx, number = %lu\n", new_ppn[ppn].bitmap, new_ppn[ppn].bitmap >> 60);

		if( new_ppn[ppn].bitmap >> (64 - RESERVED_BITS) == (1 << RESERVED_BITS) - 1){
			//reach max, check time interval
			if(duration_all - new_ppn[ppn].timer >= (1ULL << 20) ){
#ifdef USING_BITMAP
				struct evict_transfer_entry_struct tmp;
				tmp.ppn = ppn;
				tmp.value = value;
//				store_to_eb(tmp);
#ifdef USING_PAGE_DISTRIBUTION
				store_to_tb(ppn, tt, tmp);
#endif
#else
				store_to_eb(ppn, tt);
#endif
				// maybe clear
				new_ppn[ppn].bitmap = 0;
			}
			else{
				new_ppn[ppn].timer = duration_all;
			}
		} 
	}
//	printf("bitmap = 0x%lx\n", new_ppn[ppn].bitmap);
#endif
}



