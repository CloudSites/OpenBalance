# OpenBalance

An open source load balancing solution dedicated to code maintainability and making the lives of developers and engineers better.

## Building Instructions

This currently requires v1.0.0-rc1 of [[https://github.com/joyent/libuv/|libuv]

### Building on Ubuntu 14.04

Install apt dependencies

```
# apt-get install -y build-essential libjansson-dev python-mako
```

Build it!
```
# ./configure
# make
```

### Building on FreeBSD 10.0

Install pkg dependencies
```
# pkg install -y gmake gcc jansson py27-mako-1.0.0
```

Build it!
```
# ./configure
# gmake
```

