
#include "config.h"


/*
struct page_state{
        unsigned char access_bit;
        unsigned char active_bit;
};

struct page_list{
        unsigned long ppn;
        struct page_list *prev;
        struct page_list *next;
};


*/




/* for temply pagetable*/
unsigned long vpn2ppn[USING_PAGE_SIZE] = {0};
unsigned long ppn2vpn[USING_PAGE_SIZE] = {0};
struct page_state ppn2state[USING_PAGE_SIZE] = {0};
struct page_list ppn2page_list[ MAX_PPN ];




