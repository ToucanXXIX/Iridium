#!/bin/bash
export CC="clang-19"
export CXX="clang++-19"

mkdir ./builds/Debug
cd ./builds/Debug

cmake ../.. -DCMAKE_BUILD_TYPE=Debug
make -j
