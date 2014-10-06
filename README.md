# OpenBalance

An open source load balancing solution dedicated to code maintainability and
making the lives of developers and engineers better.

## Building Instructions

This currently requires v1.0.0-rc1 of [libuv](https://github.com/joyent/libuv/)

### Building on Ubuntu 14.04

Install libuv from source

```
# apt-get update
# apt-get install -y libtool autoconf make
# cd /usr/local/src/
# wget http://libuv.org/dist/v1.0.0-rc1/libuv-v1.0.0-rc1.tar.gz
# tar xzf libuv-v1.0.0-rc1.tar.gz
# rm libuv-v1.0.0-rc1.tar.gz
# cd libuv-v1.0.0-rc1/
# sh autogen.sh
# ./configure
# make install
```

Clone down OpenBalance and build it
```
# apt-get install -y git python-mako libjansson-dev
# cd /usr/local/src/
# git clone https://github.com/CloudSites/OpenBalance
# cd OpenBalance/
# ./configure
# make
```

### Building on CentOS 6.5

Install libuv from source

```
# yum install -y libtool
# cd /usr/local/src/
# wget http://libuv.org/dist/v1.0.0-rc1/libuv-v1.0.0-rc1.tar.gz
# tar xzf libuv-v1.0.0-rc1.tar.gz
# rm -f libuv-v1.0.0-rc1.tar.gz
# cd libuv-v1.0.0-rc1/
# sh autogen.sh
# ./configure
# make install
```

Clone down OpenBalance and build it
```
# yum install -y git python-mako jansson-devel
# cd /usr/local/src/
# git clone https://github.com/CloudSites/OpenBalance
# cd OpenBalance/
# ./configure
# make
```

### Building on Gentoo

Install libuv from source

```
# mkdir /usr/local/src
# cd /usr/local/src
# wget http://libuv.org/dist/v1.0.0-rc1/libuv-v1.0.0-rc1.tar.gz
# tar xzf libuv-v1.0.0-rc1.tar.gz
# rm libuv-v1.0.0-rc1.tar.gz
# cd libuv-v1.0.0-rc1/
# sh autogen.sh
# ./configure
# make install
```

Clone down OpenBalance and build it
```
# emerge --sync --quiet
# emerge dev-vcs/git dev-python/mako dev-libs/jansson
# cd /usr/local/src/

# git clone https://github.com/CloudSites/OpenBalance
# cd OpenBalance/
# ./configure
# make
```

### Building on FreeBSD 10.0

Install libuv from source

```
# pkg install -y libtool autotools
# mkdir /usr/local/src
# cd /usr/local/src
# wget http://libuv.org/dist/v1.0.0-rc1/libuv-v1.0.0-rc1.tar.gz
# tar xzf libuv-v1.0.0-rc1.tar.gz
# rm libuv-v1.0.0-rc1.tar.gz
# cd libuv-v1.0.0-rc1/
# sh autogen.sh
# ./configure
# make install
```

Clone down OpenBalance and build it
```
# pkg install -y git py27-mako gmake gcc jansson
# cd /usr/local/src/
# git clone https://github.com/CloudSites/OpenBalance
# cd OpenBalance/
# ./configure
# gmake
```
