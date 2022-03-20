#!/bin/bash

proc=$(grep -c processor /proc/cpuinfo)

mkdir libs
cd ./openssl/ && ./config && make build_all_generated && \
make -j${proc} libcrypto.a && cp -rf libcrypto.a ../libs/ && \
make -j${proc} libssl.a && cp -rf libssl.a ../libs/
cd ../gperftools/ && ./configure && make -j${proc} tcmalloc_minimal_unittest || true && \
cp .libs/libtcmalloc_minimal.a ../libs/
