#!/bin/bash -e
# Builds and tests PDAL
source ./scripts/ci/common.sh

mkdir -p _build || exit 1
cd _build || exit 1

if [[ "$CXX" == "g++" ]]
then
    export CXX="g++-4.8"
fi

cmake \
    -G "$PDAL_CMAKE_GENERATOR" \
    ..

if [[ $PDAL_CMAKE_GENERATOR == "Unix Makefiles" ]]
then
    make -j ${NUMTHREADS}
else
    # Don't use ninja's default number of threads becuase it can
    # saturate Travis's available memory.
    ninja -j ${NUMTHREADS}
fi

ctest -V --output-on-failure .
