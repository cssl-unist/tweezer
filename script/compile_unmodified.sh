#!/bin/bash

proc=$(grep -c processor /proc/cpuinfo)

make -C ./rocksdb-6.14.5-unmodified -j${proc} db_bench DEBUG_LEVEL=0

dir=./binary
if [ ! -d $dir ]; then
    mkdir $dir
fi

cp rocksdb-6.14.5-unmodified/db_bench ${dir}/vanilla
