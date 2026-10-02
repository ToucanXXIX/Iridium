#!/bin/bash
export CC="clang-19"
export CXX="clang++-19"

mkdir ./builds
mkdir ./builds/Debug
#cd ./builds/Debug

cmake -S . -B builds/Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cp builds/Debug/compile_commands.json compile_commands.json
cmake --build builds/Debug --parallel

#cmake ../.. -DCMAKE_BUILD_TYPE=Debug
#cp compile_commands.json ../../compile_commands.json
#make -j
