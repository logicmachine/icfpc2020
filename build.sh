#!/bin/sh

cd tsutaj/solution/
mkdir ../build
g++ -std=c++14 -I ../../libgalaxy/include -o ../build/main solver.cpp -lcurl
