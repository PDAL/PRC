#!/bin/bash -e
# Installs requirements for PRC
source ./scripts/ci/common.sh

sudo apt-key adv --recv-keys --keyserver keyserver.ubuntu.com 16126D3A3E5C1192
sudo apt-get update -y
sudo apt-get install software-properties-common -y
sudo apt-get install python-software-properties -y
sudo add-apt-repository ppa:ubuntugis/ppa -y
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo add-apt-repository ppa:boost-latest/ppa -y
sudo apt-get update -qq

# Install g++-4.8 (even if we're building clang) for updated libstdc++
sudo apt-get install g++-4.8

sudo apt-get install boost1.55

if [[ $PDAL_CMAKE_GENERATOR == "Ninja" ]]
then
    # Need newer cmake for Ninja generator
    wget http://www.cmake.org/files/v2.8/cmake-2.8.12.2.tar.gz
    tar -xzf cmake-2.8.12.2.tar.gz
    cd cmake-2.8.12.2
    ./bootstrap
    make
    sudo make install
    cd ..

    git clone https://github.com/martine/ninja.git
    cd ninja
    git checkout release
    ./bootstrap.py
    sudo ln -s "$PWD/ninja" /usr/local/bin/ninja
    cd ..
else
    sudo apt-get install cmake
fi

sudo apt-get install \
    libgdal-dev \
    libgeos-dev \
    libgeos++-dev \
    libpq-dev \
    libproj-dev \
    python-numpy \
    libxml2-dev \
    libflann-dev \
    libtiff4-dev \
    libhpdf-dev

# install libgeotiff from sources
wget http://download.osgeo.org/geotiff/libgeotiff/libgeotiff-1.4.0.tar.gz
tar -xzf libgeotiff-1.4.0.tar.gz
cd libgeotiff-1.4.0
./configure --prefix=/usr && make && sudo make install
cd $TRAVIS_BUILD_DIR

# install PDAL from sources
git clone https://github.com/PDAL/PDAL.git
cd PDAL
mkdir -p _build || exit 1
cd _build || exit 1
cmake \
    -G "$PDAL_CMAKE_GENERATOR" \
    ..

if [[ $PDAL_CMAKE_GENERATOR == "Unix Makefiles" ]]
then
    make -j ${NUMTHREADS}
    sudo make install
else
    ninja -j ${NUMTHREADS}
    sudo ninja install
fi
cd $TRAVIS_BUILD_DIR

gcc --version
clang --version
