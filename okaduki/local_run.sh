#!/bin/bash

set -e

pushd ../libgalaxy/example
keys=$(./room_create.sh)
popd

atk_key=$(echo $keys | cut -d' ' -f2)
def_key=$(echo $keys | cut -d' ' -f4)

uri='https://icfpc2020-api.testkontur.ru/aliens/send?apiKey=b0a3d915b8d742a39897ab4dab931721'

../build/main.old $uri $atk_key > atk.log 2>&1 &
../build/main "$uri" $def_key > def.log 2>&1 &
wait