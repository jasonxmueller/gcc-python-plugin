#   Copyright 2011 David Malcolm <dmalcolm@redhat.com>
#   Copyright 2011 Red Hat, Inc.
#
#   This is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see
#   <http://www.gnu.org/licenses/>.

from cpybuilder import *

cu = CompilationUnit()
cu.add_include('gcc-python.h')
cu.add_include('gcc-python-wrappers.h')
cu.add_include('gcc-plugin.h')

modinit_preinit = ''
modinit_postinit = ''

def generate_pretty_printer():
    global modinit_preinit
    global modinit_postinit
    
    pytype = PyTypeObject(identifier = 'gcc_PrettyPrinterType',
                          localname = 'PrettyPrinter',
                          tp_name = 'gcc.PrettyPrinter',
                          struct_name = 'struct PyGccPrettyPrinter',
                          tp_new = 'PyType_GenericNew',
                          tp_dealloc = 'gcc_PrettyPrinter_dealloc',
                          )
    cu.add_defn(pytype.c_defn())
    modinit_preinit += pytype.c_invoke_type_ready()
    modinit_postinit += pytype.c_invoke_add_to_module()
    
generate_pretty_printer()

cu.add_defn("""
int autogenerated_pretty_printer_init_types(void)
{
""" + modinit_preinit + """
    return 1;

error:
    return 0;
}
""")

cu.add_defn("""
void autogenerated_pretty_printer_add_types(PyObject *m)
{
""" + modinit_postinit + """
}
""")



print(cu.as_str())
