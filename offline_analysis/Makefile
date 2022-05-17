
all:
	make clean
	make main


build:
	make clean
	make main

main:
#	g++ -std=c++11 -fopenmp -O0 -g -o  sar  sar.c rrip.c linux_evict.c evict_control.c
	g++ -std=c++11 -fopenmp -O0 -g -o  sar  sar.c rrip.c linux_evict.c evict_control.c read_data.c
#	g++ -std=c++11 -fopenmp -O0 -g -o  sar  sar.c rrip.c evict_control.c

clean:
	rm -rf sar

