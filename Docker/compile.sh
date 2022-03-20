#!/bin/bash

proc=$(grep -c processor /proc/cpuinfo)

make -C tweezer -j${proc} db_bench DEBUG_LEVEL=0
make -C speicher_V1 -j${proc} db_bench DEBUG_LEVEL=0
