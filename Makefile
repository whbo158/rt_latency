# Author: Hongbo Wang(whbo158@qq.com)

CC = $(CROSS_COMPILE)gcc

SRC = rt_latency.c

all: rt_latency

CFLAGS += -I/usr/evl/include -I/usr/evl/ker_inc/include
LDFLAGS += -levl -L/usr/evl/lib

rt_latency: $(SRC)
	$(CC) $< -o rt_latency -lpthread

evl_latency: $(SRC)
	$(CC) $< -o evl_latency -lpthread -DXENOMAI_EVL_LIB $(CFLAGS) $(LDFLAGS)

clean:
	rm -fr *.o rt_latency evl_latency
