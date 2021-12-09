#!/usr/bin/env bash

set -eu
set -o pipefail

export MASON_RELEASE="${MASON_RELEASE:-v0.18.0}"
export MASON_LLVM_RELEASE="${MASON_LLVM_RELEASE:-5.0.1}"

PLATFORM=$(uname | tr A-Z a-z)
if [[ ${PLATFORM} == 'darwin' ]]; then
  PLATFORM="osx"
fi

MASON_URL="https://s3.amazonaws.com/mason-binaries/${PLATFORM}-$(uname -m)"

llvm_toolchain_dir="$(pwd)/.toolchain"

function run() {
    local config=${1}
    # unbreak bash shell due to rvm bug on osx: https://github.com/direnv/direnv/issues/210#issuecomment-203383459
    # this impacts any usage of scripts that are source'd (like this one)
    if [[ "${TRAVIS_OS_NAME:-}" == "osx" ]]; then
      echo 'shell_session_update() { :; }' > ~/.direnvrc
    fi

    #
    # COMPILER TOOLCHAIN
    #

    # We install clang++ without the mason client for a couple reasons:
    # 1) decoupling makes it viable to use a custom branch of mason that might
    #    modify the upstream s3 bucket in a such a way that does not give
    #    it access to build tools like clang++
    # 2) Allows us to short-circuit and use a global clang++ install if it
    #    is available to save space for local builds.
    GLOBAL_CLANG="${HOME}/.mason/mason_packages/${PLATFORM}-$(uname -m)/clang++/${MASON_LLVM_RELEASE}"
    GLOBAL_LLVM="${HOME}/.mason/mason_packages/${PLATFORM}-$(uname -m)/llvm/${MASON_LLVM_RELEASE}"
    if [[ -d ${GLOBAL_LLVM} ]]; then
      echo "Detected '${GLOBAL_LLVM}/bin/clang++', using it"
      local llvm_toolchain_dir=${GLOBAL_LLVM}
    elif [[ -d ${GLOBAL_CLANG} ]]; then
      echo "Detected '${GLOBAL_CLANG}/bin/clang++', using it"
      local llvm_toolchain_dir=${GLOBAL_CLANG}
    elif [[ -d ${GLOBAL_CLANG} ]]; then
      echo "Detected '${GLOBAL_CLANG}/bin/clang++', using it"
      local llvm_toolchain_dir=${GLOBAL_CLANG}
    elif [[ ! -d ${llvm_toolchain_dir} ]]; then
      BINARY="${MASON_URL}/clang++/${MASON_LLVM_RELEASE}.tar.gz"
      echo "Downloading ${BINARY}"
      mkdir -p ${llvm_toolchain_dir}
      curl -sSfL ${BINARY} | tar --gunzip --extract --strip-components=1 --directory=${llvm_toolchain_dir}
    fi

    #
    # MASON
    #

    function setup_mason() {
      local install_dir=${1}
      local mason_release=${2}
      mkdir -p ${install_dir}
      curl -sSfL https://github.com/mapbox/mason/archive/${mason_release}.tar.gz | tar --gunzip --extract --strip-components=1 --directory=${install_dir}
    }

    setup_mason $(pwd)/.mason ${MASON_RELEASE}

    #
    # ENV SETTINGS
    #

    echo "export PATH=${llvm_toolchain_dir}/bin:$(pwd)/.mason:$(pwd)/mason_packages/.link/bin:"'${PATH}' > ${config}
    echo "export CXX=${CXX:-${llvm_toolchain_dir}/bin/clang++}" >> ${config}
    echo "export MASON_RELEASE=${MASON_RELEASE}" >> ${config}
    echo "export MASON_LLVM_RELEASE=${MASON_LLVM_RELEASE}" >> ${config}
    # https://github.com/google/sanitizers/wiki/AddressSanitizerAsDso
    RT_BASE=${llvm_toolchain_dir}/lib/clang/${MASON_LLVM_RELEASE}/lib/$(uname | tr A-Z a-z)/libclang_rt
    if [[ $(uname -s) == 'Darwin' ]]; then
        RT_PRELOAD=${RT_BASE}.asan_osx_dynamic.dylib
    else
        RT_PRELOAD=${RT_BASE}.asan-x86_64.so
    fi
    echo "export MASON_LLVM_RT_PRELOAD=${RT_PRELOAD}" >> ${config}
    SUPPRESSION_FILE="/tmp/leak_suppressions.txt"
    # Add suppressions as needed
    #echo "leak:__strdup" > ${SUPPRESSION_FILE}
    echo "export ASAN_SYMBOLIZER_PATH=${llvm_toolchain_dir}/bin/llvm-symbolizer" >> ${config}
    echo "export MSAN_SYMBOLIZER_PATH=${llvm_toolchain_dir}/bin/llvm-symbolizer" >> ${config}
    echo "export UBSAN_OPTIONS=print_stacktrace=1" >> ${config}
    if [[ -f ${SUPPRESSION_FILE} ]]; then
        echo "export LSAN_OPTIONS=suppressions=${SUPPRESSION_FILE}" >> ${config}
    fi
    echo "export ASAN_OPTIONS=symbolize=1:abort_on_error=1:detect_container_overflow=1:check_initialization_order=1:detect_stack_use_after_return=1" >> ${config}
    echo 'export MASON_SANITIZE="-fsanitize=address,undefined -fno-sanitize=vptr,function"' >> ${config}
    echo 'export MASON_SANITIZE_CXXFLAGS="${MASON_SANITIZE} -fno-sanitize=vptr,function -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fno-common"' >> ${config}
    echo 'export MASON_SANITIZE_LDFLAGS="${MASON_SANITIZE}"' >> ${config}

    exit 0
}

function usage() {
  >&2 echo "Usage"
  >&2 echo ""
  >&2 echo "$ ./scripts/setup.sh --config local.env"
  >&2 echo "$ source local.env"
  >&2 echo ""
  exit 1
}

if [[ ! ${1:-} ]]; then
  usage
fi

# https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash
for i in "$@"
do
case $i in
    --config)
    if [[ ! ${2:-} ]]; then
      usage
    fi
    shift
    run $@
    ;;
    -h | --help)
    usage
    shift
    ;;
    *)
    usage
    ;;
esac
done
