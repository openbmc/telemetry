#!/bin/bash

set -e

if [[ ! -d boost_1_74_0 ]]; then
    wget https://boostorg.jfrog.io/artifactory/main/release/1.74.0/source/boost_1_74_0.tar.gz
    tar xfz boost_1_74_0.tar.gz
fi

cd boost_1_74_0
if [[ ! -d build ]]; then
    ./bootstrap.sh --with-libraries=coroutine
    ./b2 install --prefix=build
fi

echo -e "Please, set BOOST_ROOT to $(pwd)/build, e.g.:\n" \
        "export BOOST_ROOT=$(pwd)/build"
