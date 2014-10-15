# OpenBalance

This document outlines some of the design goals and principles for OpenBalance.

## Core Libraries

OpenBalance should not rely on libraries that are inefficient, platform
specific or have unclear APIs. Most libraries, even popular ones, do not meet
this criteria to a level of satisfaction. Any libraries that OpenBalance relies
on should be in use because they are the best tool for the job. Library
licensing is also a factor, BSD-style licenses are required to ensure the whole
codebase is as free as possible.

At present only two libraries are used, libuv and jansson. These both have
strong justifications.

Libevent and libev are popular libraries to abstract the
usage of constant time polling offered by various operations systems, such as
epoll() on Linux and kqueue() on BSD variants. Originally OpenBalance was
prototyped using libevent, but the performance was not close enough to par with
some other reverse proxy technologies. Libev offers a better level of
performance and was considered as well, but documentation seemed to be lacking
and while looking for libev resources libuv docs surfaced. Libuv's abstraction
of constant time polling is clean and fast, it's the event library that node.js
pairs with the V8 JavaScript engine for performance and portability. It also
brings with it asynchronous IO and other cross platform goodies.

In a similar fashion, Jannson is a well documented, intuitive JSON library
for C. It's API makes perfect sense in C and it's nearly as easy as using JSON
in most interpreted languages.


## JSON configuration files

There are many types of configuration files and serialization formats. Of them,
JSON is pretty well suited for this type of application.

YAML is fairly popular at the moment, and YAML is arguably more human readable
than JSON, but it's syntax is not always straight forward nor as well
understood as JSON itself. YAML offers some nifty features that are not needed
in OpenBalance.

INI type config files have worked well for a long time, but they lack good
structures for lists and nested associative arrays.

One thing OpenBalance does to take advantage of JSON is in module loading.
OpenBalance looks in the given config file for a root JSON object. It iterates
over the keys of that object loading the matching module, if the value for a
key is an object that object will be passed into the modules configuration
hook. If multiple instances of a module are desired you may supply an array of
objects instead; each will be loaded into a unique instance of the module.

JSON config structures paired with some data grabbing helper functions allows
easy delegation of config handling to the modules themselves.

## Module design

OpenBalance lives to serve it's modules. They are the star of the show and
should be the focus of attention as they are where the useful features come
from.

A custom build process implemented in Python simplifies the process of adding
new modules to the build chain. Make a folder in **src/modules/** for the new
module, create a .c and .h file with the same name as the folder and they will
be added as a target for compilation. Inside the source header define an
instance of an **ob_module_structure** with the name and hooks needed to configure,
start, and stop an instance of that module.

During compilation, the python build script will iterate through the module
directories and generate a NULL terminated array containing the
**ob_module_structure** of each module type. When the configuration file is
parsed each root configuration key is compared to the list of available
modules. Upon matching, an **ob_module** is allocated for each module to be
started. The configuration handler for the module is passed the associated JSON
object and a memory location that can be used by the module to store it's own
configuration structure.

After all modules have been instantiated and configured successfully,
OpenBalance will start each of them in the loaded order by calling the startup
handler for each module instance, allowing the modules to create listening
sockets and start adding event handlers. After all modules have completed
startup the main event loop will kick off and run until termination of the
program is requested. Cleanup functions for each module are ran before program
end to allow properly closing of resources associated with the module instance.
