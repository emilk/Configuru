#!/bin/bash
set -e # Fail on error

ROOT_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

cd "$ROOT_DIR"
mkdir -p build
cd build

rm -rf *
cmake -DCMAKE_BUILD_TYPE="Debug" -DCONFIGURU_VALUE_SEMANTICS="ON" -DCONFIGURU_IMPLICIT_CONVERSIONS="ON" ..
make
./configuru_test $@

rm -rf *
cmake -DCMAKE_BUILD_TYPE="Debug" -DCONFIGURU_VALUE_SEMANTICS="OFF" -DCONFIGURU_IMPLICIT_CONVERSIONS="ON" ..
make
./configuru_test $@


rm -rf *
cmake -DCMAKE_BUILD_TYPE="Debug" -DCONFIGURU_VALUE_SEMANTICS="ON" -DCONFIGURU_IMPLICIT_CONVERSIONS="OFF" ..
make
./configuru_test $@

rm -rf *
cmake -DCMAKE_BUILD_TYPE="Debug" -DCONFIGURU_VALUE_SEMANTICS="OFF" -DCONFIGURU_IMPLICIT_CONVERSIONS="OFF" ..
make
./configuru_test $@

echo "All tests passed!"
