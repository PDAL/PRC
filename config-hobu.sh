GENERATOR="Unix Makefiles"

BUILD_TYPE=Release

cmake . -G "$GENERATOR" \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=./package \
  -DPDAL_DIR=/Users/hobu/pdal-build/lib/pdal/cmake  \
      .
