
//#define USING_RRIP
//#define LINUX_EVICT

//#define USING_LIRS

#define USING_OPT


#define LOCAL_PAGE_SIZE (2ULL << 18)
//#define LOCAL_PAGE_SIZE (1ULL << 17)
#define USING_PAGE_SIZE (4ULL << 18)



#define MAX_REUSE (0xFFFFFFFFFFFFFFFF)


extern void init_evict_state(int choice);

extern void check_every_vpn(unsigned long vpn);

extern void print_all_analysis();
