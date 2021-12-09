#!/usr/bin/env bash

set -eu
set -o pipefail

# http://clang.llvm.org/docs/UsersManual.html#profiling-with-instrumentation
# https://www.bignerdranch.com/blog/weve-got-you-covered/

# automatically setup environment

./scripts/setup.sh --config local.env
source local.env

make clean
export CXXFLAGS="-fprofile-instr-generate -fcoverage-mapping"
export LDFLAGS="-fprofile-instr-generate"
mason install llvm-cov ${MASON_LLVM_RELEASE}
mason link llvm-cov ${MASON_LLVM_RELEASE}
make debug
rm -f *profraw
rm -f *gcov
rm -f *profdata
LLVM_PROFILE_FILE="code-%p.profraw" make test
CXX_MODULE="./build/unit-tests"
llvm-profdata merge -output=code.profdata code-*.profraw
llvm-cov report ${CXX_MODULE} -instr-profile=code.profdata -use-color
llvm-cov show ${CXX_MODULE} -instr-profile=code.profdata src/*.cpp -filename-equivalence -use-color
llvm-cov show ${CXX_MODULE} -instr-profile=code.profdata src/*.cpp -filename-equivalence -use-color --format html > /tmp/coverage.html
echo "open /tmp/coverage.html for HTML version of this report"
