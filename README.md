# zmq_example
libzmq example code with c for most of zmq patterns

To build an example

g++ -g -o bin/pub_sub pub_sub.c -lzmq -lpthread

To use czmq

g++ -g -o bin/beacon beacon.c -lczmq -lpthread

Some may need

g++ -g -std=c++1z ...

To execuate the example

./bin/pub_sub
