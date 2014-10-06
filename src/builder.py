#!/usr/bin/python
import mako
from mako.template import Template
import subprocess
import glob
import sys

libs = sys.argv[1:]
modules = glob.glob('src/modules/*')
build_modules_object = ["ld", '-o', 'obj/modules.o', '-r']
mod_names = [x.split('/')[-1] for x in modules]
compiler = 'gcc'

if subprocess.Popen(['uname'],
                    stdout=subprocess.PIPE).communicate()[0] == 'FreeBSD\n':
    compiler = 'gcc48'

print "Rendering modules.h template"
with open("src/modules.h", "w") as modules_header:
    header_template = Template(filename="src/modules.h.template")
    modules_header.write(header_template.render(modules=mod_names))

print "Rendering modules.c template"
with open("src/modules.c", "w") as modules_header:
    header_template = Template(filename="src/modules.c.template")
    modules_header.write(header_template.render(modules=mod_names))

print "Building OpenBalance modules"
for module in modules:
    name = module.split('/')[-1]
    build_path = "obj/modules/{0}.o".format(name)
    src_path = "src/modules/{0}/{0}.c".format(name)
    build_cmd = [compiler, "-Wall", "-Werror", "-Isrc/", "-g", "-c", "-o", build_path,
                 src_path]
    build_cmd.extend(libs)
    print " ".join(build_cmd)
    if(subprocess.call(build_cmd)):
        print "Failed to build {0} module".format(name)
        sys.exit(1)
    build_modules_object.append('obj/modules/{0}.o'.format(name))

print "Building module definitions"
build_cmd = [compiler, "-Wall", "-Werror", "-Isrc/", "-g", "-c", "-o",
             "obj/module_deps.o", "src/modules.c"]
print " ".join(build_cmd)
if(subprocess.call(build_cmd)):
    print "Failed to build module definitions".format(name)
    sys.exit(1)
build_modules_object.append('obj/module_deps.o')

print "Linking OpenBalance modules"
print " ".join(build_modules_object)
if(subprocess.call(build_modules_object)):
    print "Failed to link modules together"
    sys.exit(1)
