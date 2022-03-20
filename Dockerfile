FROM registry.scontain.com:5050/sconecuratedimages/crosscompilers:ubuntu

ADD Docker /root
WORKDIR /root

ENV CC /opt/scone/cross-compiler/x86_64-linux-musl/bin/x86_64-linux-musl-gcc
ENV CXX /opt/scone/cross-compiler/x86_64-linux-musl/bin/x86_64-linux-musl-g++

RUN apt update && apt install git build-essential cmake -y
RUN git clone https://github.com/gflags/gflags.git && cd gflags && mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=/opt/scone/cross-compiler/x86_64-linux-musl .. && make install
RUN git clone https://github.com/google/snappy.git && cd snappy && mkdir build && cd build && cmake -DSNAPPY_BUILD_TESTS=OFF -DSNAPPY_BUILD_BENCHMARKS=OFF \
	-DCMAKE_INSTALL_PREFIX=/opt/scone/cross-compiler/x86_64-linux-musl .. && make install

RUN git clone -b OpenSSL_1_1_1i https://github.com/openssl/openssl.git
RUN ./setup.sh
