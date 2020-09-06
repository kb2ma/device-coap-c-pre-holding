#!/bin/sh

# Build dependencies
#
#   build_deps.sh <build-csdk>
#
#   Options:
#   build-csdk: 1 to build EdgeX C SDK, 0 to skip
set -e -x

BUILD_CSDK=$1

if [ -d deps ]
then
  exit 0
fi

# get tinydtls and libcoap from source repository and build
mkdir deps
cd /device-coap/deps

# for tinydtls
# also see https://github.com/eclipse/tinydtls/pull/18 for cmake PR
#
# git clone https://github.com/eclipse/tinydtls.git
# cd tinydtls
# git checkout develop
# git reset --hard b0e230d
# autoreconf --force --install
# ./configure
# make && make install


git clone https://github.com/obgm/libcoap.git
cd libcoap
# This version includes the most recent known good tinydtls, as of 2020-08-12
git reset --hard 1739507

# patch for bug in coap_config.h.in
patch -p1 < /device-coap/scripts/config_h_in_patch

mkdir -p build && cd build
cmake .. -DDTLS_BACKEND=tinydtls -DUSE_VENDORED_TINYDTLS=OFF -DENABLE_TESTS=OFF \
         -DENABLE_EXAMPLES=OFF -DENABLE_DOCS=OFF -DCMAKE_BUILD_TYPE=Release \
         -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_LIBDIR=lib
make && make install

# get c-sdk from edgexfoundry and build
if [ "$BUILD_CSDK" = "1" ]
then
  cd /device-coap/deps

  git clone https://github.com/PJK/libcbor
  sed -e 's/-flto//' -i libcbor/CMakeLists.txt
  cmake -DCMAKE_BUILD_TYPE=Release -DCBOR_CUSTOM_ALLOC=ON libcbor
  make
  make install

  wget https://github.com/edgexfoundry/device-sdk-c/archive/v1.3.0-dev.3.zip
  unzip v1.3.0-dev.3.zip
  cd device-sdk-c-1.3.0-dev.3
  ./scripts/build.sh
  cp -rf include/* /usr/include/
  cp build/release/c/libcsdk.so /usr/lib/
fi

rm -rf /device-coap/deps

