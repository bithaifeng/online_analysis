#ifndef LT_PROFILE_H
#define LT_PROFILE_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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

#include <omp.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
 #include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

int statics_log(int type);

int statics_datatype(int type);
int statics_datatype2(int type);

int init_ltstatics();


int end_ltstatics();


#endif
