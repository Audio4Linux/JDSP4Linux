FROM ubuntu:16.04

# docker build -t gzip-hpp -f Dockerfile .
# docker run -it --cap-add SYS_PTRACE gzip-hpp

RUN apt-get update -y && \
 apt-get install -y build-essential bash curl git-core ca-certificates software-properties-common python-software-properties --no-install-recommends

RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update -y

ENV WORKINGDIR=/usr/local/src CMAKE_VERSION=3.8.2 PATH=/usr/local/src/mason_packages/.link/bin:${PATH} CXX=clang++ CXXFLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all"
WORKDIR ${WORKINGDIR}

COPY ./ ./

RUN ./scripts/setup.sh --config local.env
RUN /bin/bash -c "source local.env && mason install cmake ${CMAKE_VERSION} && mason link cmake ${CMAKE_VERSION} && which cmake"

CMD /bin/bash -c "source local.env && make debug && make test"
