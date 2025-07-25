#!/bin/bash

ALL=0
# ALL=1
ENGINE=0
# ENGINE=1
RESOURCE=0
# RESOURCE=1

if [ $ALL == 1 ]; then
  ENGINE=1
  RESOURCE=1
fi

if [ $ENGINE == 0 ]; then
  rm -rf build/Release build/Debug build/CSEngine.dir build/FormatMarkers
elif [ $ENGINE == 1 ]; then
  rm -rf build/Release build/Debug build/CSEngine.dir build/FormatMarkers build/CMakeFiles build/_deps build/cmake
fi

if [ $RESOURCE == 1 ]; then
  rm -rf build/Shaders
  rm -rf program/include/resource.hpp program/source/resource.cpp
fi
