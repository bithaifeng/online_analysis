

#define RRIP_BITS 2

#define MAXVALUE ( (1ULL << RRIP_BITS) - 1 )
#define INSERT_VALUE ( MAXVALUE - 0)

extern int hot_page_rrip_control_return(unsigned long ppn);

extern unsigned long get_ppn_from_rrip();

extern void init_rrip_structure();

extern void printf_rrip_state();

