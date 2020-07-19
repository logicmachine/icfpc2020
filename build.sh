#!/bin/sh

cd libgalaxy/example
mkdir ../../build
g++ -std=c++14 -I../include -lcurl -o ../../build/main guruguru.cpp
