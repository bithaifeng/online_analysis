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


using namespace std;
int pid=0;
static inline unsigned long long rdtsc(void)  
{  
    unsigned hi, lo;  
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));  
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );  
}  


int main(){
    
    // FILE *p = fopen("/proc/ltproc", "r+");
    pid=getpid();
    printf("pid= %d\n",pid);
    // if(p==NULL){
    //     return -1;
    // }

    int fd;
    long page_size;
    char *address1;
//    fd = open(pid_dir_name, O_RDWR | O_SYNC);
    page_size = sysconf(_SC_PAGE_SIZE);
//    if (fd < 0) {
 //       perror("open");
//        exit(-1);
//    }
//    printf("fd = %d\n", fd);

    /* mmap twice for double fun. */
    puts("mmap 1");
//    address1 = (char*)mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//    if (address1 == MAP_FAILED) {
//        perror("mmap");
//        assert(0);
//    }


    // ltwrite(p,(unsigned long long)(p));
    // strcpy(address1, (to_string((unsigned long long)address1)+"+"+to_string(pid)).c_str());

    // ltread(p);
    unsigned long len;
    
    cin>>len;
//    len = 1;

    int opt = 0;
    len = len* 0x8000000;
    double* arr = (double*)malloc(len*sizeof(double));
    printf("base: %p:\n",arr);

    //
    for(unsigned long c1=0;c1<len;c1++){
        arr[c1] = c1+1;
    }

    for(unsigned long c1=0;c1<len;c1++){
        opt += arr[c1];
    }

    usleep(100);
    cout<<"continue?"<<endl;
    int ccc =0;
    ccc = 1;
    cin>>ccc;
    float sum =2.0;
    unsigned long long bt = rdtsc(),et=0;
    for(unsigned long c1=0;c1<len-8*1024;c1+=1024){
        // ltwrite(p,(unsigned long long)(&arr[c1]));
//        strcpy(address1, (to_string((unsigned long long)(&arr[c1]))+"+"+to_string(pid)).c_str());
        for(int recount=0;recount<ccc;recount++){
            for(int c2=0;c2<1024;c2++){
//		printf("access address = 0x%lx\n", &arr[c1 + c2]);
//		getchar();
//		if(c2 % 1024 == 0)
//			usleep(1);		
//		arr[c1 + c2] = arr[c1 + c2] + 1;
                sum = ( sum + arr[c1 + c2] )  /2;
            }
        }
    }

    et=rdtsc();
    cout<<sum<<endl;
    cout<<"bt:"<<bt<<" et:"<<et<<" intelval:"<<(et-bt)/(len*sizeof(double)/4096)<<endl;
    getchar();
    getchar();

    // fclose(p);

    /* Cleanup. */
    puts("munmap 1");
 //   if (munmap(address1, page_size)) {
//        perror("munmap");
//        assert(0);
//    }
    return 0;
}

