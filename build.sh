#!/bin/sh

cd app
mkdir ../build
g++ -I./include -lcurl -O2 -Wall -I. -o ../build/main main.cpp
