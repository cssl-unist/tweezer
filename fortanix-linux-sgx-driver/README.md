Intel(R) Software Guard Extensions for Linux\* OS
================================================

# linux-sgx-driver

Introduction
------------
Intel(R) Software Guard Extensions (Intel(R) SGX) is an Intel technology for application developers seeking to protect select code and data from disclosure or modification.

The Linux SGX software stack is comprised of the Intel(R) SGX driver, the Intel(R) SGX SDK, and the Intel(R) SGX Platform Software. The Intel(R) SGX SDK and Intel(R) SGX PSW are hosted in the [linux-sgx](https://github.com/01org/linux-sgx) project.

The [linux-sgx-driver](https://github.com/01org/linux-sgx-driver) project hosts the out-of-tree driver for the Linux Intel(R) SGX software stack, which will be used until the driver upstreaming process is complete. 

Within the linux-sgx-driver project, two versions of the out-of-tree driver are provided. Both versions are compatible with the linux-sgx PSW and SDK:
- SGX 2.0 Linux Driver (sgx2)
  * The sgx2 branch of the linux-sgx-driver project contains the SGX 2.0 Linux Driver. This driver has additional support for SGX 2.0-based features available in upcoming CPUs. This driver has the same behavior as the SGX 1.5 Linux Driver (master) on CPUs without SGX 2.0 support.
- SGX 1.5 Linux Driver (master)
  * The master branch of the linux-sgx-driver project tracks the proposed upstream version of the SGX 1.5 driver and does not yet support SGX 2.0-based features.


License
-------
See License.txt for details.

Contributing
-------
Starting from 05/2017, we are importing the sgx driver code from the in-kernel sgx repository located at git-hub: https://github.com/jsakkine-intel/linux-sgx.git. Any contribution should be done there. Future versions of the sgx driver code will be imported later on. The motivation behind this decision is to maintain a single source code of the sgx linux driver.
An additional directory inker2ext/ has been created, it contains a script and a patch file that can be used in order to separately generate the code base of the sgx external module; it can be used in case someone wants the newest sgx driver as an external module and does not want to wait for next update.

The sgx2 branch hosts an initial implementation supporting SGX 2.0. This patch is maintained in inker2ext/sgx2.patch in the 2.0 branch and will be periodically rebased to take updates from the linux-sgx-driver:master branch. Contributions for this patch should be managed directly through the linux-sgx-driver project on Github.

Documentation
-------------
- [Intel(R) SGX for Linux\* OS](https://01.org/intel-softwareguard-extensions) project home page on [01.org](http://01.org)
- [Intel(R) SGX Programming Reference](https://software.intel.com/en-us/articles/intel-sdm)

Build and Install the Intel(R) SGX Driver
-----------------------------------------

### Prerequisites
- Ensure that you have the following required operating systems:  
  * Ubuntu* 16.04.3 LTS Desktop 64bits
  * Ubuntu* 16.04.3 LTS Server 64bits
  * Red Hat Enterprise Linux Server release 7.4 64bits
  * CentOS 7.4.1708 64bits
  * SUSE Linux Enterprise Server 12 64bits
- Ensure that you have the following required hardware:  
  * 6th Generation Intel(R) Core(TM) Processor or newer
- Configure the system with the **SGX hardware enabled** option.
- To build the driver, the version of installed kernel headers must match the active kernel version on the system.
  * On Ubuntu
     * To check if matching kernel headers are installed:
        ```
        $ dpkg-query -s linux-headers-$(uname -r)
        ```
     * To install matching headers:
        ```
        $ sudo apt-get install linux-headers-$(uname -r)
        ```
  * On CentOS and RHEL
     * To check if matching kernel headers are installed:
        ```
        $ ls /usr/src/kernels/$(uname -r)
        ``` 
     * To install matching headers:
        ```
        $ sudo yum install kernel-devel
        ```
     * After the above command, if the matching headers are still missing in /usr/src/kernels, try update kernel and reboot usig commands below. Then choose updated kernel on boot menu.
        ```
        $ sudo yum install kernel
        $ sudo reboot
        ```


**Note:** Refer to the *"Intel® SGX Resource Enumeration Leaves"* section in the [Intel SGX Programming reference guide](https://software.intel.com/en-us/articles/intel-sdm) to make sure your cpu has the SGX feature.


### Build the Intel(R) SGX Driver
**Note:** To use the SGX 2.0 driver, checkout or download the sgx2 branch and then follow the build instructions.

To build Intel(R) SGX driver, change the directory to the driver path and enter the following command:
```
$ make
```
You can find the driver *isgx.ko* generated in the same directory.

### Install the Intel(R) SGX Driver
To install the Intel(R) SGX driver, enter the following command with root privilege:
```
$ sudo mkdir -p "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"    
$ sudo cp isgx.ko "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"    
$ sudo sh -c "cat /etc/modules | grep -Fxq isgx || echo isgx >> /etc/modules"    
$ sudo /sbin/depmod
$ sudo /sbin/modprobe isgx
```
On Red Hat Enterprise Linux Server or CentOS, need to run below command on each reboot
```
$ sudo /sbin/modprobe isgx
``` 
On SUSE, need to add '--allow-unsupported' flag when executing 'modprobe' command during the SGX driver intallation and on each reboot
```
$ sudo /sbin/modprobe isgx --allow-unsupported
``` 

### Uninstall the Intel(R) SGX Driver
Before uninstall the Intel(R) SGX driver, make sure the aesmd service is stopped. See the topic, Start or Stop aesmd Service, on how to stop the aesmd service.  
To uninstall the Intel(R) SGX driver, enter the following commands: 
```
$ sudo /sbin/modprobe -r isgx
$ sudo rm -rf "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"
$ sudo /sbin/depmod
$ sudo /bin/sed -i '/^isgx$/d' /etc/modules
```

Statistics and Monitoring
-------------------------

Usage statistics are available in /proc/sgx_enclaves and /proc/sgx_stats.
sgx_enclaves provides a list of all of the enclaves currently configured
on the system.  sgx_stats provides statistics about system usage.

sgx_enclaves columns, in order:
* pid/tgid of controlling process
* ID of enclave (these are monotonically increasing)
* max size of enclave from SECS
* pages added to the enclave (not necessarily resident)
* pages resident

sgx_stats columns, in order:
* number of enclaves created
* number of enclaves removed (column 1 - column 2 is the number of
  enclaves running)
* total pages added to by removed enclaves (current info available
  from sgx_enclaves)
* total pages swapped into enclaves (including adds)
* total pages swapped out/removed of enclaves
* total size of enclave in 4K pages
* number of pages used for va swap space
* number of pages free (column 6 - column 7 - column 8 is pages in use)
