#!/bin/sh

cd merge
mkdir ../build
g++ -std=c++14 -I../libgalaxy/include -lcurl -o ../build/main main.cpp
