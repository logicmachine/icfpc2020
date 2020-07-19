#!/bin/sh

cd app
mkdir ../build
g++ -std=c++14 -I../libgalaxy/include -o ../build/main main.cpp -lcurl
