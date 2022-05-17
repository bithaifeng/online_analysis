
extern void call_linux_evict();
extern unsigned long int_sqrt(unsigned long x);

extern void update_access_bit(unsigned long ppn, unsigned char flag);
extern void update_active_bit(unsigned long ppn, unsigned char flag);

extern unsigned char get_access_bit(unsigned long ppn);


extern void print_analysis_linux_default();
extern unsigned long select_and_return_free_ppn_linux_default();
extern void check_vpn_linux_default( unsigned long vpn );
extern void linux_evict_init();


