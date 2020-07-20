#!/bin/sh

mkdir -p ../build
g++ -std=c++14 -I ../../libgalaxy/include -o ../build/room_creator ../../libgalaxy/example/room_creator.cpp -lcurl
g++ -std=c++14 -DGALAXY_VERBOSE -I ../../libgalaxy/include -o ../build/main solver.cpp -lcurl
