#!/bin/sh

cd ashiba/solver
mkdir build
g++ main.cpp -lcurl -I../../libgalaxy/include -std=c++11 -o ./build/main
