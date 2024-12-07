#!/bin/bash

export PATH=$PATH:/usr/evl/bin/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/evl/lib/

./evl_latency -p -c 125000 -t 600
#./rt_latency -p -c 125000 -t 600
