1. Compile the code

./compile.sh

OR:

#export CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-

make clean

make && make evl_latency

2. Run the program

./run.sh

OR:

./rt_latency -p -c 125000 -t 600

3. Run the program on Xenomai4 EVL kernel

export PATH=$PATH:/usr/evl/bin/

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/evl/lib/

./evl_latency -p -c 125000 -t 600
