#!/bin/bash
export CC="clang-19"
export CXX="clang++-19"

mkdir ./builds
mkdir ./builds/Debug
cd ./builds/Debug

cmake ../.. -DCMAKE_BUILD_TYPE=Debug
cp compile_commands.json ../../compile_commands.json
make -j
