#!/bin/sh

cd build
#./main "$@" || echo "run error code: $?"
./main "$1/aliens/send" "$2" || echo "run error code: $?"
