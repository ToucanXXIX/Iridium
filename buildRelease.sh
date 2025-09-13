#!/bin/bash
export CC="clang-19"
export CXX="clang++-19"

mkdir ./builds
mkdir ./builds/Release
cd ./builds/Release

cmake ../.. -DCMAKE_BUILD_TYPE=Release
cp compile_commands.json ../../compile_commands.json
make -j
