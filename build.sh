#!/bin/sh

cd okaduki
mkdir ../build
g++ -std=c++14 -I../include -lcurl -o ../build/main solver.cpp
