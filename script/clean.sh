#!/bin/bash

LEVEL=0
# LEVEL=1
# LEVEL=2

if [ $LEVEL == 0 ]; then
  rm -rf build/CSEngine.dir
elif [ $LEVEL == 1 ]; then
  rm -rf build/CSEngine.dir build/_deps build/cmake
elif [ $LEVEL == 2 ]; then
  rm -rf build
fi
