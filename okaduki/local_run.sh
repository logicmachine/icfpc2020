#!/bin/bash

set -e

if [[ $# < 2 ]]; then
  echo "usage: $0 attacker.out defender.out"
  exit 0
fi

atk="$1"
def="$2"

pushd ../libgalaxy/example
keys=$(./room_create.sh)
popd

echo $keys

atk_key=$(echo $keys | cut -d' ' -f2)
def_key=$(echo $keys | cut -d' ' -f4)

uri='https://icfpc2020-api.testkontur.ru/aliens/send?apiKey=b0a3d915b8d742a39897ab4dab931721'

trap 'kill $(jobs -p)' SIGINT
$atk $uri $atk_key > atk.log 2>&1 &
$def $uri $def_key > def.log 2>&1 &
wait
