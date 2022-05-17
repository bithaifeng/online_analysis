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
#include<algorithm>
#include <omp.h>

//#include "rrip.h"
#include "config.h"

using namespace std;
int pid=0;
static inline unsigned long long rdtsc(void)  
{  
    unsigned hi, lo;  
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));  
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );  
}  


/********Zipf load**************/
extern void init_zipf_index();
#define ZIPNUM 10000000
extern unsigned long zipf_index[ZIPNUM];

/****************************/

int Rand(int i){return rand()%i;}

// 512 means all
#define read_len_in_page (512)
#define read_len_offset (510)
// #define read_len_in_page (256)

int main(int args, char* argv[]){
    

    pid=getpid();
    printf("pid= %d\n",pid);
	/* eviction init  */
	init_evict_state(0);
//    printf("");
//    return 0;

    init_zipf_index();
    unsigned long scan_id = 0; 
//    for(i = 0 ; i < 20; i++)
#if 1
    for(scan_id = 0 ; scan_id < 20 ; scan_id ++){
	printf("%lu\n", zipf_index[ scan_id] );
    }
   
    printf("*******stage 1**********\n");
    for(scan_id = 0 ; scan_id < ZIPNUM ; scan_id ++){
	check_every_vpn( zipf_index[scan_id] );
    }
    print_all_analysis();
    printf("*******stage 2**********\n");

    for(scan_id = 0 ; scan_id < ZIPNUM ; scan_id ++){
	check_every_vpn( zipf_index[scan_id] );
    }
    print_all_analysis();
    return 0;
#endif

#if 0 
    for(scan_id = 0 ; scan_id < (3ULL << 18); scan_id ++){
	;check_every_vpn(scan_id);
    }
	print_all_analysis();
#endif
    unsigned long len=4;
    unsigned int thd=1;
    double opt = 0;
    len = len* 0x8000000;

//    print_analysis();

    double random_rate = 50;

    if(args<3){
        printf("get args:%d\n", args);
        return -1;
    }
    
    random_rate = 0.01*atoi(argv[2]);
    random_rate = 1;

    
    unsigned long rand_len = random_rate*len/512;
    unsigned long seq_len = 0;//(1- random_rate)*len/512;



    //  dist in len/2
    //  68 in middle 1/8    0.5g
    //  95 in middle 1/4    1g
    unsigned long ltmean = rand_len/2;
    // std:0.25G, hot=0.5G
    // unsigned long ltstd =  len/512/16;
    // std:0.50G, hot=1.0G
    // unsigned long ltstd =  rand_len/8;


    unsigned long ltstd =  rand_len/atoi(argv[1]);


    cout<<"run "<<thd<<" thread"<<endl
                <<"random access "<<rand_len<<endl
//                <<"sequence access "<<seq_len<<endl
                <<"std "<<ltstd<<endl;


    // one more page for just pending
    double* arr = (double*)malloc((len + 512)*sizeof(double));

    unsigned long* arr_idx, *arr_check;
    double* arr_cost;
    if(rand_len != 0){
        arr_idx = (unsigned long*)malloc(rand_len*sizeof(unsigned long));
        arr_check = (unsigned long*)malloc(rand_len*sizeof(unsigned long));
        arr_cost  = (double*)malloc(rand_len*sizeof(double));
        // for(unsigned int iidx=0; iidx<len/512; iidx++){
        // just let the last idx to 0
        for(unsigned int iidx=0; iidx<rand_len-1; iidx++){
            arr_idx[iidx]= iidx;
            arr_check[iidx]= 0;
            arr_cost[iidx]=0;
        }
        
        srand ( unsigned ( time(0) ) );
        random_shuffle(arr_idx, arr_idx+rand_len, Rand);
    }


    std::default_random_engine gen;

    printf("base: %p:\n",arr);
    printf("read base: %p:\n",arr + read_len_offset);
        
        
	cin>>opt;
        unsigned long startidx = 0;
        
        unsigned long endidx  = 10000000;   


        for(unsigned long c1=startidx;c1<(len + 512);c1++){
            arr[c1] = c1;
        }

        cout<<"continue flush"<<endl;

        double sum =2.0;
        unsigned long based = 0;
        unsigned long based_idx = 0;
        unsigned long miss_hot = 0;

        for(unsigned long c1=startidx;c1<(len + 512);c1++){
            sum += arr[c1];
        }

        std::normal_distribution<double> dist(ltmean, ltstd);

        cout<<"continue norm flush"<<endl;

//	print_analysis();

        for(unsigned long c1=startidx;c1<endidx;c1++){            
            if(rand_len != 0){
                based_idx = (unsigned long)(abs(dist(gen)));
                based_idx = based_idx%rand_len; 

//		printf("acces id = %lu\n", based_idx);
//		cin>>opt;
//		check_vpn(based_idx);	
		check_every_vpn( based_idx );

                based = arr_idx[based_idx]*512 + read_len_offset;
                for(unsigned long c2=0; c2<read_len_in_page; c2++)
                    sum += arr[based + c2];
            }
        }

	print_all_analysis();
//	print_analysis();
        cin>>opt;


        unsigned long long bt_r = rdtsc(),et_r=0, sum_r=0;
        unsigned long long bt_s = rdtsc(),et_s=0, sum_s=0;

        sum =2.0;
        for(unsigned long c1=0;c1<endidx;c1++){
   
            if(rand_len != 0){
                based_idx = (unsigned long)(abs(dist(gen)));
                based_idx = based_idx%rand_len; 
                based = arr_idx[based_idx]*512 + read_len_offset;

//		check_vpn(based_idx);
		check_every_vpn( based_idx );
                
                bt_r = rdtsc();
                for(unsigned long c2=0; c2<read_len_in_page; c2++)
                    sum += arr[based + c2];

                et_r=rdtsc();
                sum_r+=(et_r-bt_r);
                arr_check[based_idx]++;
                arr_cost[based_idx]+=(et_r-bt_r);
            }


            if(c1%(endidx/20) == (endidx/20 -1)){
                printf("=========\n[PID: %-6d  %.2f] rand access: sum %6lu interval: %-10.4f us\n"
                    "                    seq  access: sum %6lu interval: %-10.4f us\n",
                    getpid(), 1.0*c1/endidx, ((unsigned long)sum)%1000, 1.0*sum_r/(c1)/2400,
                                             ((unsigned long)sum)%1000, 1.0*sum_s/(c1)/2400);
            }
        }
//	print_analysis();
	print_all_analysis();
	

        printf("[PID: %-6d] rand access sum %6lu interval: %-10.4f us\n",
            getpid(), ((unsigned long)sum)%1000, 1.0*sum_r/(endidx)/2400);

        cout<<"ct free test?"<<endl;
        cin>>sum;

        unsigned long ltlog[3] = {0};
        double ltlog_cost[3] = {0};
        if(rand_len != 0){

            for(unsigned long c1=0;c1<rand_len;c1++){
                if(c1 < ltmean - ltstd*2 || c1 > ltmean+ ltstd*2){
                    ltlog[0]+=arr_check[c1];
                    ltlog_cost[0]+=arr_cost[c1];
                }else if(c1 < ltmean - ltstd || c1 > ltmean+ ltstd){
                    ltlog[1]+=arr_check[c1];
                    ltlog_cost[1]+=arr_cost[c1];
                }else{
                    ltlog[2]+=arr_check[c1];
                    ltlog_cost[2]+=arr_cost[c1];
                }
            }

            printf("=========\nLTINFO [rad] ps: %.3f<<[2std]<<%.3f<<[1std]<<%.3f<<[0]\n", 1.0*ltlog[0]/endidx, 1.0*ltlog[1]/endidx, 1.0*ltlog[2]/endidx);
        }
        
        printf("=========\n\n\n"
               "LTINFO [seq] us: %.3f\n", 1.0*sum_s/(endidx)/2400);

        if(rand_len != 0){
            printf("LTINFO [rad] us: %.3f\n", 1.0*sum_r/(endidx)/2400);
            printf("LTINFO [rad] us: %.3f<<[2std]<<%.3f<<[1std]<<%.3f<<[0]\n", 1.0*ltlog_cost[0]/ltlog[0]/2400, 1.0*ltlog_cost[1]/ltlog[1]/2400, 1.0*ltlog_cost[2]/ltlog[2]/2400);
            
            free(arr_idx);
            free(arr_check);
        }
        

    free(arr);
	cout<<"finished all, "<<opt<<endl;
    return 0;
}
