#!/bin/bash
set -e # Fail on error

ROOT_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

touch "$ROOT_DIR/*.cpp"

cd "$ROOT_DIR"
mkdir -p build
cd build

cmake ..
make

./configuru_test $@
