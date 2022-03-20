# sgxtop

This is Fortanix's SGX monitoring application, built to interoperate
with our SGX driver extensions that export SGX usage information.

It'll list the enclave count, overall enclave memory use, paging rates,
and the enclaves in use, along with their memory usage and information
about the owning process.

# Usage


``` sh
$ sgxstat
$ sgxtop
```

# Build

Build dependencies: autoconf automake m4

Set up the makefile, etc., from the autotools:

``` sh
$ ./maintainer.sh
$ ./configure
$ make
$ sudo make install
```

# Driver Installation

In order to use this, you'll need to rebuild your isgx driver using
our monitoring extensions.  You'll then need to stop the Intel aesm
daemon which is probably running at least one or two enclaves on
your system, then remove the current driver, install the new one,
and restart the aesm daemon.

Here are the instructions to make it work on Ubuntu 16.04:

``` sh
$ git clone https://github.com/fortanix/linux-sgx-driver
$ cd linux-sgx-driver
$ make
$ sudo make install
$ sudo systemctl stop aesmd
$ sudo rmmod isgx
$ sudo insmod isgx.ko
$ sudo systemctl start aesmd
```

# Documentation

Check the man page for more details:

``` sh
$ nroff -man sgxtop.1
```

# Contributing

We gratefully accept bug reports and contributions from the community.
By participating in this community, you agree to abide by [Code of Conduct](./CODE_OF_CONDUCT.md).
All contributions are covered under the Developer's Certificate of Origin (DCO).

## Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
have the right to submit it under the open source license
indicated in the file; or

(b) The contribution is based upon previous work that, to the best
of my knowledge, is covered under an appropriate open source
license and I have the right under that license to submit that
work with modifications, whether created in whole or in part
by me, under the same open source license (unless I am
permitted to submit under a different license), as indicated
in the file; or

(c) The contribution was provided directly to me by some other
person who certified (a), (b) or (c) and I have not modified
it.

(d) I understand and agree that this project and the contribution
are public and that a record of the contribution (including all
personal information I submit with it, including my sign-off) is
maintained indefinitely and may be redistributed consistent with
this project or the open source license(s) involved.

# License

This project is primarily distributed under the terms of the Mozilla Public License (MPL) 2.0, see [LICENSE](./LICENSE) for details.
