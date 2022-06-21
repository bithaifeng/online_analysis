rm -f ./trace_collect
#g++ -std=c++11 trace_collect.c prefetch_mine.c  -g -o trace_collect -O0 -lpthread -fpermissive -mcmodel=medium
g++ -std=c++11 trace_collect.c async_evict.c lightswapinterface.c lt_profile.c evict.c prefetch_mine.c page_table.c memory_manage.c rrip.c filter_table.c -w -g -o trace_collect -O2 -lpthread -fpermissive -mcmodel=medium
#g++ trace_collect.c prefetch_mine.c -g -o trace_collect -O3 -lpthread -fpermissive -mcmodel=medium
