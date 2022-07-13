#include "lightswapinterface.h"
#include "memory_manage.h"
#include "config.h"

// int ltfd,ltfd2,ltfd3;



unsigned long *init_buff = NULL;
struct hmtt_page_state *page_state = NULL;
struct ltarg_page *arg_state = NULL;

unsigned long latency_statics_arr[2][max_latency_len]={0};
unsigned int result_statics_arr[2][lt_max_err_code+1]={0};


int statics_idx[2]={0};

struct lt_queue reclaim_queue={0};

//https://gcc.gnu.org/onlinedocs/gcc/_005f_005fsync-Builtins.html
#define atomic_return_then_inc(x)  __sync_fetch_and_add(x, 1)

#define atomic_return_then_add(x, n)  __sync_fetch_and_add(x, n)

static unsigned long atomic_add_if_not_morethan(unsigned long *x, unsigned long n, unsigned long *y, unsigned long max_num)
{
    unsigned long temp_x = *x;
    unsigned long temp_y = max_num + (*y);
    while(n + temp_x < temp_y){
        if(__sync_bool_compare_and_swap(x, temp_x, temp_x+n)){
            return temp_x;
        }
        temp_x = *x;
        temp_y = max_num + (*y);
    }
    return ULONG_MAX;
}

static inline __u64 get_cycles(void)
{
        __u32 timehi, timelo;
        asm("rdtsc":"=a"(timelo),"=d"(timehi):);
        return (__u64)(((__u64)timehi)<<32 | (__u64)timelo);
}


static void lt_sign_latency(int type, unsigned long latency){
    if(statics_idx[type] > max_latency_len-2)
        return;

    int tempidx = atomic_return_then_inc(&statics_idx[type]);

    latency_statics_arr[type][tempidx] = latency;
}


struct lt_thread_arg{
    unsigned int tid;
};

struct threadpool{
    unsigned int shutdown;
    pthread_mutex_t lock;
    int trying_lock_ref;
    pthread_cond_t queue_not_empty;

    pthread_t thread_ptr[EVICT_PTHREAD_NUM];
    struct lt_thread_arg thread_arg[EVICT_PTHREAD_NUM];
};

struct threadpool lt_evictor;


int make_reclaim_lazy_async(unsigned int pid, unsigned long vpn){
    if(lt_evictor.shutdown == 1){
        return 0;
    }

    int tidx = 0;
    unsigned long ret = 0;

    atomic_return_then_add(&lt_evictor.trying_lock_ref, 1);
    pthread_mutex_lock(&(lt_evictor.lock));

    // leave one to here
    if(reclaim_queue.end - reclaim_queue.__queue_real_begin >= lt_queue_max_size - 1){
        // update queue real begin idx
        
        unsigned long mini_beg = reclaim_queue.beg;
        for(tidx=0;tidx < EVICT_PTHREAD_NUM; tidx++){
            mini_beg=min(mini_beg, reclaim_queue.local_last[tidx]);
        }
        reclaim_queue.__queue_real_begin = mini_beg;

        if(reclaim_queue.end - reclaim_queue.__queue_real_begin >= lt_queue_max_size- 1){
            // just drop this msg. avoid overflow.
            reclaim_queue.drop_full_msg_counter++;

            pthread_mutex_unlock(&(lt_evictor.lock));
            atomic_return_then_add(&lt_evictor.trying_lock_ref, -1);
            return 0;
        }
    }

    // asume not race here

    // maybe we can use the flags to sync??
    unsigned long store_idx = atomic_add_if_not_morethan(&reclaim_queue.end, 1, &reclaim_queue.__queue_real_begin, lt_queue_max_size);

    if(store_idx == ULONG_MAX){
        // just drop this msg. avoid overflow.
        reclaim_queue.drop_full_msg_counter++;
        pthread_mutex_unlock(&(lt_evictor.lock));
        atomic_return_then_add(&lt_evictor.trying_lock_ref, -1);
        return 0;
    }

    reclaim_queue.qwork[qwork_idx(store_idx)].flags  = 1;
    reclaim_queue.qwork[qwork_idx(store_idx)].pid    = pid;
    reclaim_queue.qwork[qwork_idx(store_idx)].page_num = vpn;

    // pthread_mutex_lock(&(lt_evictor.lock));
    pthread_cond_signal(&(lt_evictor.queue_not_empty));
    pthread_mutex_unlock(&(lt_evictor.lock));
    atomic_return_then_add(&lt_evictor.trying_lock_ref, -1);

    return 1;
}

unsigned long evict_batch(unsigned int evictor_idx, unsigned long task_beg){
    unsigned long ret;
    unsigned long tidx;
    for(tidx=0;tidx<ltarg_max_evict_batch; tidx++){
        arg_state[evictor_idx*ltarg_max_evict_batch + tidx].flags      = reclaim_queue.qwork[qwork_idx(task_beg + tidx)].flags;
        arg_state[evictor_idx*ltarg_max_evict_batch + tidx].pid        = reclaim_queue.qwork[qwork_idx(task_beg + tidx)].pid;
        arg_state[evictor_idx*ltarg_max_evict_batch + tidx].page_num   = reclaim_queue.qwork[qwork_idx(task_beg + tidx)].page_num;
    }

    unsigned long start_c = get_cycles();
    ret = syscall(336,  evictor_idx, tidx);
    unsigned long end_c = get_cycles();
    
    // statics latency
    if(ret > 0)
        lt_sign_latency(lt_evict_idx, (end_c - start_c)/ret);

    // statics the result
    int c1=0,err_code=0;
    for(c1=0;c1<tidx;c1++){
        err_code = arg_state[evictor_idx*ltarg_max_evict_batch + c1].result;
        if(err_code < lt_max_err_code)
            result_statics_arr[lt_evict_idx][err_code]++;
        else{
            printf("LTINFO: result code=%u err!!\n\n", err_code);
        }
    }

    // not need make local_last max here
    // it will update in next round. 
    // if then, there is no task, it will be the max
    return tidx;
}

void *evictor_runtime(void* evictor_arg){
    struct lt_thread_arg* evictor_task = (struct lt_thread_arg*) evictor_arg;

    while(true)
    {
        // no task, wait
        /* Lock must be taken to wait on conditional variable */
        pthread_mutex_lock(&(lt_evictor.lock));
        
        reclaim_queue.local_last[evictor_task->tid] = reclaim_queue.beg;
        unsigned long ret=atomic_add_if_not_morethan(&reclaim_queue.beg, ltarg_max_evict_batch, &reclaim_queue.end, 0);

        if(ret != ULONG_MAX){
            pthread_mutex_unlock(&(lt_evictor.lock));
            evict_batch(evictor_task->tid, ret);
        }else{
            // update the last used idx
            // this evictor do not use any. make it biggest.
            reclaim_queue.local_last[evictor_task->tid] = ULONG_MAX;

            if (lt_evictor.shutdown)
            {
                pthread_mutex_unlock(&(lt_evictor.lock));
                printf("thread 0x%x is exiting\n", pthread_self());
                pthread_exit(NULL);
                return NULL;
            }else{
                pthread_cond_wait(&(lt_evictor.queue_not_empty), &(lt_evictor.lock));
                pthread_mutex_unlock(&(lt_evictor.lock));
            }
        }
    }

    return NULL;
}

int init_thread_pool(){
    lt_evictor.shutdown = 0;
    lt_evictor.trying_lock_ref = 0;

    unsigned int tidx = 0;
    cpu_set_t mask_evict_pool;

    CPU_ZERO(&mask_evict_pool);

    for(tidx=EVICT_CORE_ID; tidx<EVICT_CORE_ID+EVICT_PTHREAD_NUM; tidx++){
        CPU_SET(tidx, &mask_evict_pool);
    }

    for(tidx=0; tidx<EVICT_PTHREAD_NUM; tidx++){
        lt_evictor.thread_arg[tidx].tid = tidx;
        pthread_create(&(lt_evictor.thread_ptr[tidx]), NULL, evictor_runtime, (void *)(&lt_evictor.thread_arg[tidx]));
        
        pthread_setaffinity_np(lt_evictor.thread_ptr[tidx], sizeof(mask_evict_pool), &mask_evict_pool) ;
    }

    if(pthread_mutex_init(&(lt_evictor.lock), NULL) != 0
        || pthread_cond_init(&(lt_evictor.queue_not_empty), NULL) != 0){
        printf("init the lock or cond fail\n");
        return -1;
    }

    return 1;
}

int end_thread_pool(void){

    unsigned int tidx = 0;

    lt_evictor.shutdown = 1;

    // wake all up
    pthread_cond_broadcast(&(lt_evictor.queue_not_empty));

    // wake up the waiting thread
    for (tidx = 0; tidx < EVICT_PTHREAD_NUM; tidx++)
    {
        pthread_join(lt_evictor.thread_ptr[tidx], NULL);
    }

    printf("LTENEDING: finished join\n");

wait_ref:
    pthread_mutex_lock(&(lt_evictor.lock));
    if(lt_evictor.trying_lock_ref > 0){
        printf("LTENEDING: mutex is using, wait %d\n", lt_evictor.trying_lock_ref);
        pthread_mutex_unlock(&(lt_evictor.lock));
        goto wait_ref;
    }

    printf("LTENEDING: finished lock\n");
    pthread_mutex_destroy(&(lt_evictor.lock));

    // will cause bug.
    // just let it go.
    // printf("LTENEDING: finished lcok destroy\n");
    // pthread_cond_destroy(&(lt_evictor.queue_not_empty));

    printf("LTENEDING: finished cond destroy\n");
    printf("LT: end evictor\n");
    return 1;
}


int compare_ul(const void* a, const void* b)
{
    int arg1 = *(const unsigned long*)a;
    int arg2 = *(const unsigned long*)b;
 
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}



static inline void init_ltls_run(void){
    printf("init entry = %lu\n page state:%lu\n struct ltarg_page:%lu\n\n", sizeof(unsigned long), sizeof(struct hmtt_page_state), sizeof(struct ltarg_page));

    unsigned int lidx=0;
    for(lidx=0; lidx<EVICT_PTHREAD_NUM;lidx++){
        reclaim_queue.local_last[lidx]=ULONG_MAX;
    }

    printf("LTINFO: inited hmttinter\n");

    // init_thread_pool();
}

static inline void end_ltls_run(void){
    // end_thread_pool();

    // statics latency
    double means=0;
    unsigned long idx = 0;
    unsigned long len = statics_idx[lt_fetch_idx];
    printf("\nfetch RESULT code:  \n\
        succeessed                  :%lu \n\
        lt_err_pagenotfound         :%lu \n\
        lt_err_pagenotmapped        :%lu \n\
        lt_err_piderr               :%lu \n\
        lt_err_pidoutofmanagement   :%lu \n\
        lt_err_vpnoutofmanagement   :%lu \n\
        lt_err_ptenotassigned       :%lu \n\
        lt_err_ptenotpresent        :%lu \n\
        lt_err_ptepresent           :%lu \n\
        lt_err_otherschangeit       :%lu \n\
        ",
        result_statics_arr[lt_fetch_idx][0],
        result_statics_arr[lt_fetch_idx][1],
        result_statics_arr[lt_fetch_idx][2],
        result_statics_arr[lt_fetch_idx][3],
        result_statics_arr[lt_fetch_idx][4],
        result_statics_arr[lt_fetch_idx][5],
        result_statics_arr[lt_fetch_idx][6],
        result_statics_arr[lt_fetch_idx][7],
        result_statics_arr[lt_fetch_idx][8],
        result_statics_arr[lt_fetch_idx][9]
    );

    if(len > 0){
        for(idx=0;idx<len;idx++){
            means = means + (latency_statics_arr[lt_fetch_idx][idx] - means)/(idx+1); 
        }
        
        // https://en.cppreference.com/w/c/algorithm/qsort
        qsort(&latency_statics_arr[lt_fetch_idx][0], len, sizeof(unsigned long), compare_ul);

        
        printf("LT fetch %lu statics \nmeans:%6.2f us, min:%6.2f us, max:%6.2f us\n\
                tail latency: \n\
                0.100:%6.2f\n\
                0.200:%6.2f\n\
                0.300:%6.2f\n\
                0.400:%6.2f\n\
                0.500:%6.2f\n\
                0.600:%6.2f\n\
                0.700:%6.2f\n\
                0.800:%6.2f\n\
                0.900:%6.2f\n\
                0.990:%6.2f\n\
                0.999:%6.2f\n",
                len,
                means / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*1)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.1)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.2)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.3)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.4)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.5)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.6)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.7)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.8)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.9)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.99)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_fetch_idx][(unsigned long)((len-1)*0.999)] * 1.0 / lt_cpu_frequence);
    }

    means=0;
    len = statics_idx[lt_evict_idx];

    printf("\nevict RESULT code:  \n\
        succeessed                  :%lu \n\
        lt_err_pagenotfound         :%lu \n\
        lt_err_pagenotmapped        :%lu \n\
        lt_err_piderr               :%lu \n\
        lt_err_pidoutofmanagement   :%lu \n\
        lt_err_vpnoutofmanagement   :%lu \n\
        lt_err_ptenotassigned       :%lu \n\
        lt_err_ptenotpresent        :%lu \n\
        lt_err_ptepresent           :%lu \n\
        lt_err_otherschangeit       :%lu \n\
        ",
        result_statics_arr[lt_evict_idx][0],
        result_statics_arr[lt_evict_idx][1],
        result_statics_arr[lt_evict_idx][2],
        result_statics_arr[lt_evict_idx][3],
        result_statics_arr[lt_evict_idx][4],
        result_statics_arr[lt_evict_idx][5],
        result_statics_arr[lt_evict_idx][6],
        result_statics_arr[lt_evict_idx][7],
        result_statics_arr[lt_evict_idx][8],
        result_statics_arr[lt_evict_idx][9]
        );
    if(len > 0){
        for(idx=0;idx<len;idx++){
            means = means + (latency_statics_arr[lt_evict_idx][idx] - means)/(idx+1); 
        }
        
        qsort(&latency_statics_arr[lt_evict_idx][0], len, sizeof(unsigned long), compare_ul);

        printf("\nLT evict %lu drop %lu mesg\nstatics \nmeans:%6.2f us, min:%6.2f us, max:%6.2f us\n\
                tail latency: \n\
                0.100:%6.2f\n\
                0.200:%6.2f\n\
                0.300:%6.2f\n\
                0.400:%6.2f\n\
                0.500:%6.2f\n\
                0.600:%6.2f\n\
                0.700:%6.2f\n\
                0.800:%6.2f\n\
                0.900:%6.2f\n\
                0.990:%6.2f\n\
                0.999:%6.2f\n",
                len,
                reclaim_queue.drop_full_msg_counter,
                means / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*1)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.1)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.2)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.3)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.4)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.5)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.6)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.7)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.8)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.9)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.99)] * 1.0 / lt_cpu_frequence,
                latency_statics_arr[lt_evict_idx][(unsigned long)((len-1)*0.999)] * 1.0 / lt_cpu_frequence);
    }
}

// must init shared mem before call
int init_ltls_inter(void){
#ifndef USING_FASTSWAP
    init_buff = memory_buffer_start_addr;
    page_state = (struct hmtt_page_state *)page_state_array;
    arg_state = (struct ltarg_page *)evict_engine_start_addr;
#endif    

    init_ltls_run();

    // init_ltls_accessstatics();
    return 0;
}

int end_ltls_inter(void){
    end_ltls_run();
    // end_ltls_accessstatics();
    return 0;
}

int get_ltls_statics(void){
    printf("\nLTINFO: total:%8ld, (u:%8ld f:%8ld)\n\n",
                        init_buff[0], init_buff[1], init_buff[2]);
    return init_buff[1];
}

// reset the idx for recounter
int reset_statics(void){
    statics_idx[lt_fetch_idx] = 0;
    statics_idx[lt_evict_idx] = 0;
    printf("LTINFO: reset statics idx:%lx, %lx\n", statics_idx[lt_fetch_idx], statics_idx[lt_evict_idx]);
    return 0;
}


int lt_queue_size(void){
    return reclaim_queue.end - reclaim_queue.beg;
}


int lt_printer_limiter = 1;
int make_reclaim_page(unsigned int evictor_idx, unsigned int len){
    int tidx = 0;
    unsigned long ret = 0;

    if( (evictor_idx >= EVICT_PTHREAD_NUM && evictor_idx != ltarg_max_evictor_size) || (len == 0)){
        printf("LTINFO: evictor_idx:%d overflow. max:%d\n", evictor_idx, ltarg_max_evictor_size);
        return 0;
    }

    unsigned long start_c = get_cycles();
    ret = syscall(336,  evictor_idx, len);
    unsigned long end_c = get_cycles();
    
    // statics latency
    if(ret > 0)
        lt_sign_latency(lt_evict_idx, (end_c - start_c)/ret);


    if(ltarg_max_evictor_size == evictor_idx){
        result_statics_arr[lt_evict_idx][0]+=ret;

        return ret;
    }

    // statics the result
    unsigned int c1=0,err_code=0;
    for(c1=0; c1<len; c1++){
        err_code = arg_state[evictor_idx*ltarg_max_evict_batch + c1].result;
        if(err_code < lt_max_err_code)
            result_statics_arr[lt_evict_idx][err_code]++;
        else{
            printf("LTINFO: result code=%u err!!\n\n", err_code);
        }
    }


    return ret;
}

int make_fetch_page(unsigned int pid, unsigned long vpn){
    unsigned long ret = 0;

#ifdef exit_prefetch_when_nomem
    if(init_buff[2] < MIN_PAGE_NUM/2){
        result_statics_arr[lt_fetch_idx][lt_err_nomem]++;
        return 0;
    }
#endif

    unsigned long start_c = get_cycles();
    ret = syscall(335,  pid, vpn<<12, 0);
    unsigned long end_c = get_cycles();

    if(ret > 10)
        lt_sign_latency(lt_fetch_idx, (end_c - start_c));

    if(ret < lt_max_err_code)
        result_statics_arr[lt_fetch_idx][ret]++;
    else{
        result_statics_arr[lt_fetch_idx][0]++;
    }
    return ret;
}



