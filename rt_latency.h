/*
 * Author: Hongbo Wang(whbo158@qq.com)
 */

#ifndef __RT_LATENCY_H__
#define __RT_LATENCY_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <time.h> /* clock_gettime() */
#include <limits.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/mman.h> /* mlockall() */
#include <sys/stat.h>
#include <sys/prctl.h>


#define CYCLE_TIME (125000) /* unit: ns */
#define NSEC_PER_SEC (1000000000)

#define CLOCK_TYPE CLOCK_MONOTONIC  /* CLOCK_REALTIME */

#define EVL_CLK_ID(x) (((x) == CLOCK_REALTIME) ? EVL_CLOCK_REALTIME : EVL_CLOCK_MONOTONIC)
#define EVL_CLK_ID2(x) evl_get_clock_id(EVL_CLK_ID(x))

#ifdef XENOMAI_EVL_LIB
#define rt_printf evl_printf
#else
#define rt_printf printf
#endif

typedef struct _stat_val_t {
	int32_t val;
	int32_t min;
	int32_t max;
	uint32_t cnt;
	float mean;
	uint32_t min_idx;
	uint32_t max_idx;
} stat_val_t;

#define diff_time(pa, pb) (((pb)->tv_sec - (pa)->tv_sec) * NSEC_PER_SEC + (pb)->tv_nsec - (pa)->tv_nsec)

#define timespec_add(a, b, r)                          \
	do {                                                      \
		(r)->tv_sec = (a)->tv_sec + (b)->tv_sec;           \
		(r)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;        \
		if ((r)->tv_nsec >= NSEC_PER_SEC) {                \
			++(r)->tv_sec;                             \
			(r)->tv_nsec -= NSEC_PER_SEC;              \
		}						    \
	} while (0)


#endif
