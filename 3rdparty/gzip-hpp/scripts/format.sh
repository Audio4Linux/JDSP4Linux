#!/usr/bin/env bash

set -eu
set -o pipefail

: '

Runs clang-format on the code in include/

Return `1` if there are files to be formatted, and automatically formats them.

Returns `0` if everything looks properly formatted.

'
 # Set up the environment by installing mason and clang++
./scripts/setup.sh --config local.env
source local.env

# Add clang-format as a dep
mason install clang-format ${MASON_LLVM_RELEASE}
mason link clang-format ${MASON_LLVM_RELEASE}

# Run clang-format on all cpp and hpp files in the /src directory
find include/ test/ bench/ -type f -name '*.hpp'  -or -name '*.cpp' \
 | xargs -I{} clang-format -i -style=file {}

# Print list of modified files
dirty=$(git ls-files --modified include/)

if [[ $dirty ]]; then
    echo "The following files have been modified:"
    echo $dirty
    git diff
    exit 1
else
    exit 0
fi