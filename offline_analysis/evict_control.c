#include "config.h"




void init_evict_state(int choice){

#ifdef USING_OPT
	init_opt();
#endif


#ifdef USING_RRIP
	init_rrip_structure();
        init_logicl_access();
        int i = 0;
        for(i = 0; i < ((1ULL << RRIP_BITS)); i++){
                printf("maintains[%d] = %lu\n", i,maintain_number[i]);
        }	
#endif

#ifdef LINUX_EVICT
	linux_evict_init();	
	print_analysis_linux_default();
#endif	

#ifdef USING_LIRS
	init_lirs();
	print_analysis_lirs();
#endif


}





void check_every_vpn(unsigned long vpn){

#ifdef USING_RRIP
	check_vpn(vpn);
#endif

#ifdef LINUX_EVICT
	check_vpn_linux_default(vpn);;
#endif

#ifdef USING_LIRS
	check_vpn_lirs(vpn);
#endif

}


void print_all_analysis(){
	
#ifdef USING_RRIP
	print_analysis();
#endif

#ifdef LINUX_EVICT
	print_analysis_linux_default();	
#endif

#ifdef USING_LIRS
	print_analysis_lirs();
#endif


#ifdef USING_OPT
	print_analysis_opt();
#endif

}





