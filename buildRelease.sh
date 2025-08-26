#!/bin/bash
export CC="clang-19"
export CXX="clang++-19"

mkdir ./builds/Release
cd ./builds/Release

cmake ../.. -DCMAKE_BUILD_TYPE=Release
make -j
