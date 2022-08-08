#include <asm/unistd.h> 
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sched.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <map>

#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>

#include <vector>
//#include <hash_map>
#include <hash_map>

#include "config.h"

using namespace std;
using namespace __gnu_cxx;
//#using namespace stdext;  
//#define _GNU_SOURCE
#define MAX_PPN ((66ULL << 30) >> 12)

#define SIMPLE_STREAM
//#define LADDER
//#define LADDER_RIPPLE

#ifdef LADDER_RIPPLE
#define USING_RIIPLE
#endif


 #define MONITOER_PREFETCH
#define USETIMER
#define USING_BITMAP
//#define USING_NUM_PREFETCH

//#define STORE_FILE


#define PREFETCH_BUFFER_PTHREAD 300000

//#define OTHERPPN

//#define USE_MORE_CORE



extern int now_prefetch_step;

extern volatile unsigned long last_latency_all;
extern volatile unsigned long last_ac_counter;
extern volatile unsigned long ac_counter;
extern volatile unsigned long latency_all;

extern int insert_entry(unsigned long ppn, unsigned long time);

extern int now_pid;
extern unsigned long pgtable_transfer( unsigned long ppn );


struct ppn2num_t{
#ifdef USING_NUM_PREFETCH
	unsigned char num; // using
#endif
//	unsigned long num;

#ifdef MONITOER_PREFETCH
        unsigned char isprefetch;
	unsigned long prefetch_time;
#endif

#ifdef USING_BITMAP
	unsigned long bitmap;
#endif

#ifdef USETIMER
	unsigned long timer;
#endif
};

#define LruCacheSize (64)


struct value_struct{
	unsigned char num;
	unsigned long timer;
};

extern void init_lruCache(int size);
extern void filter_lru(unsigned long p_addr, unsigned long tt);

#define __u64 unsigned long long 
#define __u32 unsigned int 


//extern static  __u64 get_cycles(void);


struct CacheNode {
  unsigned long key;
  struct value_struct value;
  CacheNode *pre, *next;
  CacheNode(int k, struct value_struct v) : key(k), value(v), pre(NULL), next(NULL) {}
};
 
class LRUCache{
private:
  int size;                     // Maximum of cachelist size.
  CacheNode *head, *tail;
//  map<unsigned long, CacheNode *> mp;          // Use hashmap to store
   hash_map<unsigned long, CacheNode *> mp;

public:
  LRUCache(int capacity)
  {
    size = capacity;
    head = NULL;
    tail = NULL;
  }
 
  struct value_struct get(unsigned long key)
  {
   // map<unsigned long, CacheNode *>::iterator it = mp.find(key);
     hash_map<unsigned long, CacheNode *>::iterator it = mp.find(key);


    if (it != mp.end())
    {
      CacheNode *node = it -> second;
      remove(node);
      setHead(node);
      return node -> value;
    }
    else
    {
      struct value_struct tmp;
      tmp.num = 0;
      tmp.timer = 0;
      return tmp;
    }
  }
 
  void set(unsigned long key, struct value_struct value)
  {
  //  map<unsigned long, CacheNode *>::iterator it = mp.find(key);
    hash_map<unsigned long, CacheNode *>::iterator it = mp.find(key);
    if (it != mp.end())
    {
      CacheNode *node = it -> second;
      node -> value = value;
      remove(node);
      setHead(node);
    }
    else
    {
      /*
      CacheNode *newNode = new CacheNode(key, value);
      if (mp.size() >= size)
      {
	map<unsigned long, CacheNode *>::iterator iter = mp.find(tail -> key);
      	remove(tail);
	mp.erase(iter);
      }
      setHead(newNode);
      mp[key] = newNode;
      */
     if (mp.size() >= size){
//	map<unsigned long, CacheNode *>::iterator iter = mp.find(tail -> key);
	hash_map<unsigned long, CacheNode *>::iterator iter = mp.find(tail -> key);
	CacheNode *newnode = iter->second;
//	printf("eveict key = %d, value = %d\n", newnode->key, newnode->value);
	newnode->key = key;
	newnode->value =value;

	remove(newnode);
        setHead(newnode);
	mp[key] = newnode;
	mp.erase(iter);

     }
     else{
       CacheNode *newNode = new CacheNode(key, value);
       setHead(newNode);
       mp[key] = newNode;
//       printf("key = %d, value = %d, now size = %d\n", key, value.num, mp.size());
     }



    }
  }
 
  void remove(CacheNode *node)
  {
    if (node -> pre != NULL)
    {
      node -> pre -> next = node -> next;
    }
    else
    {
      head = node -> next;
    }
    if (node -> next != NULL)
    {
      node -> next -> pre = node -> pre;
    }
    else
    {
      tail = node -> pre;
    }
  }
 
  void setHead(CacheNode *node)
  {
    node -> next = head;
    node -> pre = NULL;
 
    if (head != NULL)
    {
      head -> pre = node;
    }
    head = node;
    if (tail == NULL)
    {
      tail = head;
    }
  }
};

extern LRUCache *lruCache;



struct prefetch_task{
  unsigned int real_tid;
  int tid;
  volatile unsigned long long vaddr[PREFETCH_BUFFER_PTHREAD];
  volatile unsigned long ptr_w,ptr_r;
  volatile int task_len;
  int stride;
  int delete_num;
//  volatile int pid[PREFETCH_BUFFER_PTHREAD];
  volatile int p_inter_page[PREFETCH_BUFFER_PTHREAD];
};

struct prefetch_entry{
	unsigned long vpn_target;
	int pid;
};


extern void store_to_pb(unsigned long ppn, int inter_page);
extern int now_pid ;
extern volatile unsigned long *ppn2vpn;
extern volatile int *ppn2pid;
extern volatile unsigned long *ppn2rpt;

extern map<unsigned long, unsigned char> prefetch_pid_flag[65536];

extern volatile unsigned int ppn2num[MAX_PPN];

extern map<unsigned long, unsigned char> ppn2train_buffer;
extern unsigned int bit_map[MAX_PPN];
//extern struct ppn2num_t new_ppn[MAX_PPN];
#ifdef OTHERPPN
extern struct ppn2num_t * new_ppn;
#else
extern struct ppn2num_t new_ppn[MAX_PPN];
#endif
extern void store_to_tb(unsigned long ppn, unsigned long time);
extern int delta(unsigned long a,unsigned long b);
extern int using_inter_page;

extern int filter_check(unsigned long p_addr, unsigned long tt);
extern struct prefetch_task prefetcher_task_set[max_prefetcher_count];


extern volatile unsigned long  tb_w_ptr, tb_r_ptr;

#define STEP_USE 500

extern void InsertNewLSD_vpn(unsigned long ppn, unsigned long vpn, unsigned long now_time, int real_pid);
#define LONGSTRIDE_STRIDEN 8
//#define LONGSTRIDE_STRIDEN 24
#define Stride_Array_len (LONGSTRIDE_STRIDEN - 1)
//#define LONGSTRIDE_STRIDEN 5
#define STRIDEN LONGSTRIDE_STRIDEN
#define LSDSTREAM_SIZE 64
//#define LSDSTREAM_SIZE 96
//#define LSDSTREAM_SIZE 32
#define PREFETCH_TIME (40000ULL)

//#define LSDUSE_PPN

#define USE_TIMER

#define USE_STRIDE


struct LSD_vpn{
	char valid;
	unsigned long start_vpn;
	int pid;
	unsigned long vpn[LONGSTRIDE_STRIDEN];
	int size;
#ifdef LSDUSE_PPN
	unsigned long ppn[LONGSTRIDE_STRIDEN];
#endif
	int stride;
	int history_size;
#ifdef USE_TIMER
	unsigned long timer[LONGSTRIDE_STRIDEN];
#endif
	
#ifdef USE_STRIDE
	int stride_array[ LONGSTRIDE_STRIDEN ];
#endif

//	unsigned long accessn[LONGSTRIDE_STRIDEN];
//	int access_times;

//	map<unsigned long, int> vpn2value;
	int stream_num;
	int ladder_num;
	int ripple_num;
	int no_paatern;

	unsigned long current_time;

};


extern volatile unsigned long long all_has_read ;
extern volatile int times_use ;




extern volatile unsigned long duration_all;

extern struct LSD_vpn table_lsd[LSDSTREAM_SIZE];
extern map<unsigned long, int> ppn2LSD_vpn; 



extern char free_pt_addr_magic     ;
extern char set_page_table_magic   ;
extern char free_page_table_magic  ;
extern char free_page_table_get_clear ;
extern char free_page_table_get_clear_full ;

extern void init_prefetch_structure();

extern cpu_set_t mask_cpu_2;
extern cpu_set_t mask_kt;

extern volatile unsigned long *dev_writeptr;
extern unsigned long *dev_readptr;

extern unsigned char *p_kernel_trace , *p_kernel_trace_buf;
extern unsigned long dev_size, buffer_size; //

extern unsigned long max_kt_buffer_offst;


extern unsigned long hot_physical_number;

extern void print_stride();
