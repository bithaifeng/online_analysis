#include "evict_control.h"



struct lirs_entry{
	unsigned long vpn,ppn;
	int status; //LIR or HIR // -1 miss no init or dont in stack S, 0 means LIR, 1 means HIR , 2 means non-resident block
	int in_local; // 1 means in local, 0 means not in local.
	struct lirs_entry *prev, *next;
};

extern struct lirs_entry vpn2list_entry[ USING_PAGE_SIZE ];
extern struct lirs_entry ppn2list_entry_stackQ[ LOCAL_PAGE_SIZE ];

extern void check_vpn_lirs(unsigned long vpn);
extern void print_analysis_lirs();

extern void init_lirs();


