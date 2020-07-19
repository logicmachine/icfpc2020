#!/bin/sh

cd ashiba/solver/build
./main "$1/aliens/send" "$2" || echo "run error code: $?"
