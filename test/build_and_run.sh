#!/bin/bash
set -e # Fail on error

COMPILER="clang" # TODO: argument

ROOT_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

cd "$ROOT_DIR"
mkdir -p build
cd build

if [ "$COMPILER" = "clang" ]; then
    echo "Using Clang"
    export CC="/usr/bin/clang-3.7"
    export CXX="/usr/bin/clang++-3.7"
    export CCACHE_CPP2="yes"
fi

cmake ..
make

./configuru_test $@
