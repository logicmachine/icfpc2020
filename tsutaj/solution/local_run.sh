#!/bin/bash

set -e

uri='https://icfpc2020-api.testkontur.ru/aliens/send?apiKey=b0a3d915b8d742a39897ab4dab931721'

keys=$(../build/room_creator $uri)
echo $keys

atk_key=$(echo $keys | cut -d' ' -f2)
def_key=$(echo $keys | cut -d' ' -f4)
echo $atk_key
echo $def_key

../build/main "${uri}" $atk_key > atk.log 2>&1 &
../build/main.old "${uri}" $def_key > def.log 2>&1 &
wait
