#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>


#define MALLOC_SIZE (1ULL << 29)  //512MB

#define __u64 unsigned long 
#define __u32 unsigned int


unsigned long tick_1 = 0, tick_2 = 0;


void clflush_u(volatile void *p)
{
	asm volatile ("clflush (%0)" :: "r"(p));
}


void mem_addr(unsigned long vaddr, unsigned long *paddr)
{
    int pageSize = getpagesize(); // //调用此函数获取系统设定的页面大小

    unsigned long v_pageIndex = vaddr / pageSize;//计算此虚拟地址相对于0x0的经过的页面数
    unsigned long v_offset = v_pageIndex * sizeof(uint64_t);//计算在/proc/pid/page_map文件中的偏移量
    unsigned long page_offset = vaddr % pageSize;//计算虚拟地址在页面中的偏移量
    uint64_t item = 0;//存储对应项的值

    int fd = open("/proc/self/pagemap", O_RDONLY); //以只读方式打开/proc/pid/page_map
    if(fd == -1)//判断是否打开失败
    {
        printf("open /proc/self/pagemap error\n");
        return;
    }

    if(lseek(fd, v_offset, SEEK_SET) == -1)//将游标移动到相应位置，即对应项的起始地址且判断是否移动失败
    {
        printf("sleek error\n");
        return; 
    }

    if(read(fd, &item, sizeof(uint64_t)) != sizeof(uint64_t))//读取对应项的值，并存入item中，且判断读取数据位数是否正确
    {
        printf("read item error\n");
        return;
    }

    if((((uint64_t)1 << 63) & item) == 0)//判断present是否为0
    {
        printf("page present is 0\n");
        return ;
    }

    uint64_t phy_pageIndex = (((uint64_t)1 << 55) - 1) & item;//计算物理页号，即取item的bit0-54

    *paddr = (phy_pageIndex * pageSize) + page_offset;//再加上页内偏移量就得到了物理地址
}

static inline __u64 get_cycles(void)
{
	__u32 timehi, timelo;
	asm("rdtsc":"=a"(timelo),"=d"(timehi):);
	return (__u64)(((__u64)timehi)<<32 | (__u64)timelo);
}

#define READTIMES 1
unsigned long addr_str[READTIMES] = {0};

//#define USEGETCHAR

int main(){
	int pid;
	printf("pid= %d\n",getpid());

	int len = 3;
	printf("sizeof(int) = %d\n",sizeof(int));

	char * str[16];
	time_t t;
	srand((unsigned) time(&t));
	int i = 0, z = 0;

/*
	int a[5] = { 0,1,2,3,4 },k=0,n=0,s=0;
	srand((unsigned)time(NULL));
	for (int i = 0; i < 1000; i++){
		k = rand() % 5;
		n = rand() % 5;
		s = a[k];
		a[k] = a[n];
		a[n] = s;
	}
*/

	int tttt = 1;
	for(z = 0 ;z < len; z++){
		str[z] = (char *) malloc(MALLOC_SIZE);
		printf("Buff[%d] --0x%lx ~ 0x%lx \n",z,str[z], str[z] + MALLOC_SIZE);
		for(i = 0; i < MALLOC_SIZE ; i = i + (1 << 12)){
//			tick_1 = get_cycles();
			*(str[z] + i) = rand() % 256;
//			tick_2 = get_cycles();
//			printf(" read use %ld \n", tick_2 - tick_1 );
//			getchar();
//			printf("addr = 0x%lx\n", str[z] + i);
//			getchar();
		}
		for(i = 0; i < MALLOC_SIZE ; i = i + 4){
			*(int *)(str[z] + i) = rand() % 9999999;
		}
		int n = MALLOC_SIZE / 4;
		printf("n = %d\n", n );
/*		
		int t,j;
		for(i = 0;i < n - 1; i++){
//			if(i %10 == 0) printf("i = %d\n",i); 
			for(j = 0; j < n -1 -i; j++){
//				if(j % (n/20) == 0) printf("j = %d\n",j);
				
				if(*(int *)(str[z] + j * 4) > *(int *)(str[z] + 4* (j + 1) )){
					t = *(int *)(str[z] + j * 4);
					*(int *)(str[z] + j * 4) = *(int *)(str[z] + 4 * (j + 1));
					*(int *)(str[z] + 4 * (j + 1) ) = t;
				}
			}
		}
*/
	}
/*
	char *str1;
	str[1] = (char *) malloc(MALLOC_SIZE);
        for(i = 0; i < MALLOC_SIZE ; i = i + (1 << 11)){
                *(str[1] + i) = rand() % 256;
	}
	char *str2;
        str[2] = (char *) malloc(MALLOC_SIZE);
        for(i = 0; i < MALLOC_SIZE ; i = i + (1 << 11)){
                *(str[2] + i) = rand() % 256;
	}
	str[3] = (char *) malloc(MALLOC_SIZE);
        for(i = 0; i < MALLOC_SIZE ; i = i + (1 << 11)){
                *(str[3] + i) = rand() % 256;
	}

	char *str4;
        str[4] = (char *) malloc(MALLOC_SIZE);
        for(i = 0; i < MALLOC_SIZE ; i = i + (1 << 11)){
                *(str[4] + i) = rand() % 256;
	}
*/
	int j = 0;
	int read_times = 1;
	int inter = 7;
	inter = 1;
	unsigned long long all_time = 0;
	char tmp;// = getchar();
	int num_loop = 6;
	while(num_loop -- > 0){
//		tmp = getchar();
		if(tmp == 'c')
			break;
/*
		for(i = 0; i < 200 ; i ++){
			tick_2 = get_cycles();
			printf("Last ten read use %ld, I will use 0x%lx\n", tick_2 - tick_1 , ((unsigned long)(str[j] + i) >> 12) << 12);
			getchar();
			tick_1 = get_cycles();
		}
		*(str[j] + rand() % 131072  ) = (*(str[j] + i) + rand() % 256) % 256;

*/
		int tmp_sum = 0;
#if 1
		printf("using str[%d]\n",j);
		for(i = 0; i < MALLOC_SIZE ; i = i + inter * (1 << 12)){
			if((read_times ) % 5 == 0){
				inter = inter + 3;
				printf("Last ten read use %ld, I will use 0x%lx, inter = %d\n", tick_2 - tick_1 , ((unsigned long)(str[j] + i) >> 12) << 12, inter);
				if(read_times != 0 ){
				all_time += ( tick_2 - tick_1 );
				printf("average time = %lf\n", (double)all_time / read_times);
				}
				//read_times = 0;
				getchar();
			}
			tick_1 = get_cycles();
//			*(str[j] + i) = (*(str[j] + i) + rand() % 256) % 256;
//			*(str[j] + i) = ( rand() % 256) % 256;
			tmp_sum = tmp_sum + *(str[j] + i);
			tick_2 = get_cycles();
			read_times ++;
		}
#endif

#if 0
                printf("using str[%d]\n",j);
                for(i = 0; i < MALLOC_SIZE ; i = i + inter * (1 << 12)){
                        if((read_times ) % READTIMES == 0){
                                tick_2 = get_cycles();
                                inter = inter + 3;
				inter = 1;
#ifdef USEGETCHAR
                                printf("Last ten read use %ld, I will use 0x%lx, inter = %d\n", tick_2 - tick_1 , ((unsigned long)(str[j] + i) >> 12) << 12, inter);
                                read_times = 0;
                                tmp = getchar();
#endif
				if(tmp == 'c')
		                        break;
//				printf("flush finish\n");
//				getchar();
                                tick_1 = get_cycles();
                        }
#if 0
			for(int z = 0; z < 4096; z = z+ 64){
				 *(char *)(((unsigned long )(str[j] + i) >> 12) << 12 + z) = (*(char *)(((unsigned long)(str[j] + i) >> 12) << 12 + z) + rand() % 256) % 256;
			}
#endif
   //                     *(str[j] + i) = (*(str[j] + i) + rand() % 256) % 256;
//			addr_str[read_times] = (unsigned long )(str[j] + i);
//                        *(str[j] + i) = (*(str[j] + i) + rand() % 256) % 256;
//                        tmp_sum = tmp_sum + *(str[j] + i);
//                        tmp_sum = tmp_sum + *(int *)( ( (unsigned long) (str[j] + i) >> 12) << 12);
			
			for(int h = 0 ; h < 64; h ++)
				tmp_sum = tmp_sum + *(int *)( ( ( (unsigned long) (str[j] + i) >> 12) << 12) + h * 64);


                        unsigned long virtual_addr = ((unsigned long)(str[j] + i) >> 12) << 12;
                        unsigned long physical_addr = 0;
#ifdef USEGETCHAR
                        mem_addr(virtual_addr  , &physical_addr);

                        printf("vpn =  0x%lx , ppn = 0x%lx \n", virtual_addr >> 12, physical_addr >> 12 );
#endif
                        read_times ++;
                }
#endif

		printf("finish str[%d]\n",j);

		j = (j + 1) % len;

	}
	for(z = 0;z < len; z++){
		free(str[z]);
	}




}
