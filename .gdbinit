# Put this in ~/.gdbinit:
# set auto-load safe-path /path/to/ubloxcfg

show auto-load safe-path
python
import os, sys, glob

print("\n\n----------- .gdbinit ---------")
print('Using Python ' + ' '.join([sys.executable] + sys.version.split('\n')))
sys.path.insert(0, '/usr/share/gcc/python')
sys.path.insert(0, '/home/flip/git/ubloxcfg/3rdparty/gdb')
sys.path.insert(0, os.getcwd() + '/3rdparty/gdb')
#/usr/share/gcc/python/libstdcxx

# Natvis (see also debug config in ubloxcfg.code-workspace)
print('- loading natvis4gdb')
import natvis4gdb
natvis4gdb.register()

# STL pretty printer
print('- loading libstdcxx pretty printer')
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers(None)

# GLM pretty printer
print('- loading GLM pretty printer')
import glm_pp

# JSNO pretty printer
print('- loading json pretty printer')
import nlohmann_json

end

set print pretty on
set print object on
set print static-members on
set print vtbl on
set print demangle on
set demangle-style gnu-v3
set print sevenbit-strings off
set pagination off
set history filename ~/.gdb_history
set history remove-duplicates unlimited
set history save

# Enable ImGui Natvis
#add-natvis 3rdparty/imgui/imgui.natvis

# https://sourceware.org/gdb/onlinedocs/gdb/Skipping-Over-Functions-and-Files.html
# skip STL ctor/dtor
skip -rfunction ^std::__cxx11::(allocator|basic_string)<.*>::
skip -rfunction ^std::__cxx11::([a-zA-z0-9_]+)<.*>::
# skip trivial ImGui functions
skip -rfunction Im(Vec2|Vec4|Strv|Vector|Span)::.+
# skip trivial Ff functions
skip -rfunction Ff::Vec2::.+
# Print skip info
info skip

python
print("\n\n\n----------- done ---------\n\n\n")

end