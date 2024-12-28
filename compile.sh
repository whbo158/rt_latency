#!/bin/bash

#export CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-

make clean
make
make evl_latency
