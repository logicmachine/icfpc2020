#!/bin/sh

cd tsutaj/build
./main "$1/aliens/send" "$2" || echo "run error code: $?"
