GENERATOR="Unix Makefiles"

BUILD_TYPE=Release

cmake -G "$GENERATOR" \
  -DPDAL_INCLUDE_DIR=../pdal/include \
  -DPDAL_LIBRARY=../pdal/lib/libpdal.dylib \
  -DCMAKE_INSTALL_PREFIX=./package \
      .
