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
    unsigned long j = 0;
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
    len = 5;

    char opt = 0;
//    len = len* 0x8000000;
//    double* arr = (double*)malloc(len*sizeof(double));
    char* arr = (char*)malloc( (len * (1ULL << 30) + 4096) *sizeof(char));
    printf("base: %p:\n",arr);
    arr =  (char *)((((unsigned long)arr + 4096) >> 12) << 12);
    printf("base: %p:\n",arr);
    printf("size = %ld MB\n", len * 1024);
    len = len * (1ULL << 30);
    unsigned long read_page_num = 0;

    //
    for(unsigned long c1=0;c1<len;c1++){
//        arr[c1] = c1+1;
        arr[c1] = 0;
    }

    for(unsigned long c1=0;c1<len;c1 += 4096){
        opt += arr[c1];
    }

    usleep(100);
    cout<<"continue?"<<endl;
    int ccc =0;
    ccc = 1;
    cin>>ccc;
    float sum =2.0;
    char sum_ch = 0;
    unsigned long long bt = rdtsc(),et=0;
#if 0
    for(unsigned long i = 0; i < len; i += 4){
	if(i % 4096 == 0) read_page_num ++;
	sum_ch += arr[i];
    }

#endif
    int stride1 = 1;
    int stride2 = 9;


#if 1
	for(unsigned long i = 0; i < len; ){
		if(i % 4096 == 0) read_page_num ++;
		for( j = 0; j < 4096; j += 4){
			sum_ch += arr[i + j];
		}
		i = i + (4096) * stride1;
		if(i >= len) break;
		if(i % 4096 == 0) read_page_num ++;
		for( j = 0; j < 4096; j += 4){
                        sum_ch += arr[i + j];
                }		
		i = i + (4096) * stride2;
	}
#endif

    et=rdtsc();
    cout<<sum<<endl;
    cout<<sum_ch<<endl;
//    cout<<"bt:"<<bt<<" et:"<<et<<" intelval:"<<(et-bt)/(len/4096)<<endl;
    cout<<"bt:"<<bt<<" et:"<<et<<" intelval:"<<(et-bt)/( read_page_num )<<endl;
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

