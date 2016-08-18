#!/bin/bash
set -e # Fail on error

ROOT_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

touch "$ROOT_DIR/*.cpp"

cd "$ROOT_DIR"
mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE="Debug" -DCONFIGURU_VALUE_SEMANTICS="ON" -DCONFIGURU_IMPLICIT_CONVERSIONS="ON" ..
make
./configuru_test $@

cmake -DCMAKE_BUILD_TYPE="Debug" -DCONFIGURU_VALUE_SEMANTICS="OFF" -DCONFIGURU_IMPLICIT_CONVERSIONS="ON" ..
make
./configuru_test $@


cmake -DCMAKE_BUILD_TYPE="Debug" -DCONFIGURU_VALUE_SEMANTICS="ON" -DCONFIGURU_IMPLICIT_CONVERSIONS="OFF" ..
make
./configuru_test $@

cmake -DCMAKE_BUILD_TYPE="Debug" -DCONFIGURU_VALUE_SEMANTICS="OFF" -DCONFIGURU_IMPLICIT_CONVERSIONS="OFF" ..
make
./configuru_test $@
