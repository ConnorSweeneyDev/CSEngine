#!/bin/bash

ALL=0
# ALL=1
ENGINE=0
# ENGINE=1
SHADER=0
# SHADER=1

if [ $ALL == 1 ]; then
  rm -rf build
fi

if [ $ENGINE == 0 ]; then
  rm -rf build/CSEngine.dir
elif [ $ENGINE == 1 ]; then
  rm -rf build/CSEngine.dir build/_deps build/cmake
fi

if [ $SHADER == 1 ]; then
  rm -rf build/Shaders
fi
