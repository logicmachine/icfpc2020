#!/bin/sh

cd okaduki
mkdir -p ../build
g++ -std=c++14 -I../libgalaxy/include -o ../build/main solver.cpp -lcurl
