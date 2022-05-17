//#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <iostream>
#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>     /* getopt() */

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* uintmax_t */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */

#include <omp.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
 #include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>



#define LOCAL_PAGE_SIZE (1ULL << 17)
//#define LOCAL_PAGE_SIZE (512)    //
//#define LOCAL_PAGE_SIZE (2ULL << 18)
#define USING_PAGE_SIZE (4ULL << 18)

//#define LOCAL_PAGE_SIZE 256
//#define USING_PAGE_SIZE 512



#define MAX_ACCESS_TIME (10UL << 20) // 10 million

#define RRIP_BITS 4
#define MAXVALUE ( (1ULL << RRIP_BITS) - 1 )
#define INSERT_VALUE ( MAXVALUE - 3)

#define MAX_PPN (64ULL << 18)




struct page_list{
	unsigned long ppn;
	struct page_list *prev;
	struct page_list *next;
};


extern void insert_page(unsigned long ppn, int group_id);



extern struct page_list head[1 << RRIP_BITS], tail[1 << RRIP_BITS];
extern long long maintain_number[ 1 << RRIP_BITS ];
extern struct page_list ppn2page_list[ MAX_PPN ];


extern long long mem_using;

extern int ppn2groupid[MAX_PPN];

extern void init_logicl_access();
extern void init_rrip_structure();

extern void print_analysis();

extern unsigned long hit_num , miss_num ;


void list_del( struct page_list * page_list );


extern unsigned long select_and_return_free_ppn();
extern void check_and_update_page(unsigned long ppn);

extern void check_vpn(unsigned long vpn);


/* page state  */

struct page_state{
	unsigned char access_bit;
	unsigned char active_bit;
};

extern struct page_state ppn2state[USING_PAGE_SIZE];
extern unsigned long vpn2ppn[USING_PAGE_SIZE];
extern unsigned long ppn2vpn[USING_PAGE_SIZE];








