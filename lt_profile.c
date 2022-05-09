#include "lt_profile.h"
#include  <stdlib.h>
#include <stdio.h>

#include "config.h"
//#define LTLOG(format, args...) printk("LTDEBUG s %s LINE %d: "format"\n",__FILE__, __LINE__,##args)


unsigned long reclaim_type[3] = {0};


unsigned long ltstatics_initdata_type[11]={0};

int statics_datatype(int type){
    if(type == lt_hmtt_page_unused){
	    ltstatics_initdata_type[type]++;
    }else if(type == lt_hmtt_page_unstable){
	    ltstatics_initdata_type[type]++;
    }else if(type == lt_hmtt_page_free){
	    ltstatics_initdata_type[type]++;
    }else if(type == lt_hmtt_page_using){
	    ltstatics_initdata_type[type]++;
    }else{
	    ltstatics_initdata_type[10]++;
    }
	return 0;
}

unsigned long ltstatics_initdata_type2[11]={0};

int statics_datatype2(int type){
    if(type == lt_hmtt_page_unused){
	    ltstatics_initdata_type2[type]++;
    }else if(type == lt_hmtt_page_unstable){
	    ltstatics_initdata_type2[type]++;
    }else if(type == lt_hmtt_page_free){
	    ltstatics_initdata_type2[type]++;
    }else if(type == lt_hmtt_page_using){
	    ltstatics_initdata_type2[type]++;
    }else{
	    ltstatics_initdata_type2[10]++;
    }
	return 0;
}



inline void init_ltls_accessstatics(void){

}


inline void  end_ltls_accessstatics(void){

    unsigned int idx=0;

	printf("\nin filter\n get data in pagestate type [unused]: %8lu,  [free]: %8lu,  [using]: %8lu, [unstable]: %8lu, [err]: %8lu\n", 
            ltstatics_initdata_type[lt_hmtt_page_unused],
            ltstatics_initdata_type[lt_hmtt_page_free],
            ltstatics_initdata_type[lt_hmtt_page_using],
            ltstatics_initdata_type[lt_hmtt_page_unstable],
            ltstatics_initdata_type[10]
            );

    printf("\nin train\n get data in pagestate type [unused]: %8lu,  [free]: %8lu,  [using]: %8lu, [unstable]: %8lu, [err]: %8lu\n", 
            ltstatics_initdata_type2[lt_hmtt_page_unused],
            ltstatics_initdata_type2[lt_hmtt_page_free],
            ltstatics_initdata_type2[lt_hmtt_page_using],
            ltstatics_initdata_type2[lt_hmtt_page_unstable],
            ltstatics_initdata_type2[10]
            );
}





int statics_log(int type){
	if(type >= 2){

		return -1;
	}
	reclaim_type[type]++;
	return 0;
}


int init_ltstatics(){
	init_ltls_accessstatics();
	return 0;
}


int end_ltstatics(){
    end_ltls_accessstatics();
	printf("force reclaim: %8lu, activate reclaim: %8lu\n", reclaim_type[1], reclaim_type[0]);
	return 0;
}


