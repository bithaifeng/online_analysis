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

int main(int args, char* argv[]){
    

    pid=getpid();
    printf("pid= %d\n",pid);


    unsigned long len=4;
    unsigned int thd=1;
    len = 170* 1024;

    if(args>1){
        len = atoi(argv[1])* 1024;
    }

    cout<<"run "<<thd<<" thread, access 4 GB"<<endl;

    char* arr = (char*)malloc(len*sizeof(char));
    unsigned long sum = 0;
    while (1)
    {    
        for(unsigned long c1=0;c1<len;c1++){
            sum =(sum+ arr[c1])%12;
        }
    }
    
    printf("%lu\n", sum);
     
    free(arr);
    return 0;
}
