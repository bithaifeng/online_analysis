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

using namespace std;
int pid=0;
static inline unsigned long long rdtsc(void)  
{  
    unsigned hi, lo;  
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));  
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );  
}  

static inline int accessArr(double* arr,  unsigned long len, unsigned long stride){
    unsigned long sum =1;
    for(unsigned long c1=0;c1<len;c1+=512){
    	for(int c2=0;c2<stride;c2++){
        //     if(c1+c2<len)
            sum += arr[c1+c2];
        }
    }

    return sum;
}


int accessArr_2(double* arr,  unsigned long len, unsigned long stride){
    unsigned long sum =1;
    unsigned long total_pass=10;
    for(int pass=0;pass<total_pass;pass++){
    	for(unsigned long c1=pass;c1<len;c1+=total_pass*512){
    		for(int c2=0;c2<stride;c2++){
       			//     if(c1+c2<len)
				sum += (sum+arr[c1+c2])/2;
		}
        }
    }

    return sum;
}



int Rand(int i){return rand()%i;}



// 512 means all
#define read_len_in_page (512)
#define read_len_offset (510)
// #define read_len_in_page (256)

int main(int args, char* argv[]){
    

    pid=getpid();
    printf("pid= %d\n",pid);


    unsigned long len=4;
    unsigned int thd=1;
    double opt = 0;
    len = len* 0x8000000;

    //  dist in len/2
    //  68 in middle 1/8    0.5g
    //  95 in middle 1/4    1g
    unsigned long ltmean = len/512/2;
    // std:0.25G, hot=0.5G
    unsigned long ltstd =  len/512/16;
    // std:0.50G, hot=1.0G
    // unsigned long ltstd =  len/512/8;

    if(args>1){
        ltstd =  len/512/atoi(argv[1]);
    }

    cout<<"run "<<thd<<" thread, access 4 GB, std:"<<ltstd<<endl;


    cin>>opt;
    double* arr = (double*)malloc(len*sizeof(double));
    unsigned long* arr_idx = (unsigned long*)malloc(len/512*sizeof(unsigned long));
    unsigned long*  arr_check = (unsigned long*)malloc(len/512*sizeof(unsigned long));
    double*         arr_cost  = (double*)malloc(len/512*sizeof(double));
    // for(unsigned int iidx=0; iidx<len/512; iidx++){
    // just let the last idx to 0
    for(unsigned int iidx=0; iidx<len/512-1; iidx++){
        arr_idx[iidx]= iidx;
        arr_check[iidx]= 0;
        arr_cost[iidx]=0;
    }

    srand ( unsigned ( time(0) ) );
    random_shuffle(arr_idx, arr_idx+len/512,Rand);

    std::default_random_engine gen;

    printf("base: %p:\n",arr);
    printf("read base: %p:\n",arr + read_len_offset);
        
        unsigned long startidx = 0;
        
        unsigned long endidx  = len/512 * 10;   


        for(unsigned long c1=startidx;c1<len;c1++){
            arr[c1] = c1;
        }

        printf("LTINFO: addr %lx ~ %lx \n", (unsigned long)&arr[startidx], (unsigned long)&arr[endidx-1]);

        cout<<"continue flush"<<endl;

        double sum =2.0;
        unsigned long based = 0;
        unsigned long based_idx = 0;
        unsigned long miss_hot = 0;

        for(unsigned long c1=startidx;c1<len;c1++){
            sum += arr[c1];
        }

        std::normal_distribution<double> dist(ltmean, ltstd);

        cout<<"continue norm flush"<<endl;

        for(unsigned long c1=startidx;c1<endidx;c1++){
            based_idx = (unsigned long)(abs(dist(gen)));
            based_idx = based_idx%(len/512); 
            based = arr_idx[based_idx]*512 + read_len_offset;
            for(unsigned long c2=0; c2<read_len_in_page; c2++)
        		sum += arr[based + c2];
        }

        cout<<"continue norm test?"<<endl;
        cin>>opt;


        unsigned long long bt_r = rdtsc(),et_r=0, sum_r=0;

        sum =2.0;
        for(unsigned long c1=0;c1<endidx;c1++){
            bt_r = rdtsc();
            based_idx = (unsigned long)(abs(dist(gen)));
            based_idx = based_idx%(len/512); 
            arr_check[based_idx]++;
            based = arr_idx[based_idx]*512 + read_len_offset;
            for(unsigned long c2=0; c2<read_len_in_page; c2++)
        		sum += arr[based + c2];
            et_r=rdtsc();
            sum_r+=(et_r-bt_r);
            arr_cost[based_idx]+=(et_r-bt_r);

            if(c1%(endidx/20) == (endidx/20 -1))
                printf("[PID: %-6d  %.2f] rand access: sum %6lu interval: %-10.4f us\n",
                    getpid(), 1.0*c1/endidx, ((unsigned long)sum)%1000, 1.0*sum_r/(c1)/2400);
        }

        printf("[PID: %-6d] rand access sum %6lu interval: %-10.4f us\n",
            getpid(), ((unsigned long)sum)%1000, 1.0*sum_r/(endidx)/2400);

        cout<<"ct free test?"<<endl;
        cin>>sum;

        unsigned long ltlog[3] = {0};
        double ltlog_cost[3] = {0};
        if((4*(1UL << 18)) != len/512){
            cout<<(4*(1UL << 18))<<"???"<<len/512<<endl;
        }

        for(unsigned long c1=0;c1<len/512;c1++){
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

        printf("LTINFO ps: %.3f<<[2std]<<%.3f<<[1std]<<%.3f<<[0]\n", 1.0*ltlog[0]/endidx, 1.0*ltlog[1]/endidx, 1.0*ltlog[2]/endidx);
        printf("LTINFO us: %.3f<<[2std]<<%.3f<<[1std]<<%.3f<<[0]\n", 1.0*ltlog_cost[0]/ltlog[0]/2400, 1.0*ltlog_cost[1]/ltlog[1]/2400, 1.0*ltlog_cost[2]/ltlog[2]/2400);

    free(arr);
    free(arr_idx);
    free(arr_check);
	cout<<"finished all, "<<opt<<endl;
    return 0;
}
