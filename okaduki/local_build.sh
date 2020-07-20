#!/bin/sh

mkdir -p ../build
# g++ -std=c++14 -DGALAXY_VERBOSE -I../libgalaxy/include -o ../build/main solver.cpp -lcurl
g++ -std=c++14 -DLOCAL_DEBUG -DGALAXY_VERBOSE -I../libgalaxy/include -O2 -o solver solver.cpp -lcurl
