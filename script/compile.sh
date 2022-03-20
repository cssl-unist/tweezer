#!/bin/bash

source script/determine_sgx_device.sh
determine_sgx_device
docker rm tweezer
docker run $MOUNT_SGXDEVICE --name tweezer tweezer:0.0 /root/compile.sh
mkdir -p binary
docker cp tweezer:/root/tweezer/db_bench binary/tweezer
docker cp tweezer:/root/speicher_V1/db_bench binary/speicher
