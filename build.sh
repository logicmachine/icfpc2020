#!/bin/sh

cd libgalaxy/example
mkdir ../../build
g++ -std=c++11 -I../include -lcurl -o ../../build/main idle.cpp
