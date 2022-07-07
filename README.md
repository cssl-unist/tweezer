# A Log-Structured Merge Tree-aware Message Authentication Scheme for Persistent Key-Value Stores

## Base Version
Rocksdb : 6.14.5
OpenSSL : 1.1.1i


## Environment
- Tested on Ubuntu 18.04 with 64 bit

## Installation
### Install sgxtop with driver
~~~{.sh}
$ cd fortanix-linux-sgx-driver
# Install matching header
$ sudo apt-get install linux-headers-$(uname -r)
$ make

$ sudo mkdir -p "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"    
$ sudo cp isgx.ko "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"    
$ sudo sh -c "cat /etc/modules | grep -Fxq isgx || echo isgx >> /etc/modules"    
$ sudo /sbin/depmod
$ sudo /sbin/modprobe isgx\

#Install sgxtop
$ cd sgxtop
$ ./maintainer.sh
~~~

## Installation using Docker
* Please install docker before proceeding. [Install Docker](https://docs.docker.com/engine/install/ubuntu/)
* Please register [scone container registry](https://sconedocs.github.io/) to pull cross compiler.
~~~{.sh}
# In project root directory
$ docker build -t tweezer:0.0 ./
~~~

## Preparation for testing.
* Compiling db_bench for each instance.
* Each db_bench will installed in \{Root project dir\}/binary
~~~{.sh}
# Prepare vanilla rocksdb
$ script/compile_unmodified.sh

# Prepare tweezer & speicher version of rocksdb.
$ script/compile.sh
~~~

## Run for testing
* You can configure workloads and benchmark using config.csv
~~~{.sh}
# Generate tmpfs for untrusted memory
$ cd script
$ sudo ./generate_tmpfs.sh

$ cd test 
$ python3 csv-to-run.py

# Delete tmpfs
$ cd script
$ sudo ./delete_tmpfs.sh
~~~

## Authors
 - Igjae Kim (UNIST and KAIST) <ijkim@kaist.ac.kr>
 - J. Hyun Kim (UNIST) <ckeiom@unist.ac.kr>
 - Minu Chung (UNIST) <minu0122@unist.ac.kr>
 - Hyungon Moon (UNIST) <hyungon@unist.ac.kr>
 - Sam H. Noh (UNIST) <samhnoh@unist.ac.kr>

## Publications
```
@inproceedings {277820,
title       = {A {Log-Structured} Merge Tree-aware Message Authentication Scheme for Persistent {Key-Value} Stores},
author      = {Igjae Kim and J. Hyun Kim and Minu Chung and Hyungon Moon and Sam H. Noh},
booktitle   = {20th USENIX Conference on File and Storage Technologies (FAST 22)},
year        = {2022},
address     = {Santa Clara, CA},
publisher   = {USENIX Association},
month       = feb,
}
```
