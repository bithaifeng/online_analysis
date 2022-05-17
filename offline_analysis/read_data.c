#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char file0[] = "zipf/Zipf.txt";
char file1[] = "data.txt";

FILE *fp = NULL;
#define ZIPNUM 10000000

unsigned long zipf_index[ZIPNUM] = {0};

char buff[255];

unsigned long str2;

void init_zipf_index(){
	fp = fopen(file0, "r");
	int i = 0;
	for(i = 0; i < ZIPNUM; i ++){
//	for(i = 0; i < 5; i ++){
		fscanf(fp, "%s", buff);
//		if(i < 20)
//			printf("%lu\n", atol(buff));

//		printf("%d: %s\n", i,buff );
//		printf("%lu\n", strtoul(buff));
		zipf_index[i] = atol(buff);
	}

	fclose(fp);
//	for(i = 0 ; i < 20; i++)
//		printf("%lu\n", zipf_indexp[i] );

}


 
int func()
{

	init_zipf_index();
	return 0;
 
//   fp = fopen("zipf/Zipf.txt", "r");
   fp = fopen(file0, "r");
//   fp = fopen(file1, "r");
   fscanf(fp, "%s", buff);
   printf("1: %s\n", buff );
 
   fscanf(fp, "%s", buff);
   printf("2: %s\n", buff );
   
   fscanf(fp, "%s", buff);
   printf("3: %s\n", buff );
   fclose(fp);
 
}

