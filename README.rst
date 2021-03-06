gcc-python
==========

This is a plugin for GCC, which links against libpython, and (I hope) allows
you to invoke arbitrary Python scripts from inside the compiler.  The aim is to
allow you to write GCC plugins in Python.

The plugin is Free Software, licensed under the GPLv3 (or later).

It's still at the "experimental proof-of-concept stage"; expect crashes and
tracebacks (I'm new to insides of GCC, and I may have misunderstood things).

It's already possible to use this to add additional compiler errors/warnings,
e.g. domain-specific checks, or static analysis.  One of my goals for this is
to "teach" GCC about the common mistakes people make when writing extensions
for CPython, but it could be used e.g. to teach GCC about GTK's
reference-counting semantics, or about locking in the Linux kernel, or about
signal-safety in APIs.

Other ideas include visualizations of code structure.   Given a `gcc.CFG`
instance, `gccutils.render_to_dot(cfg)` and `gccutils.invoke_dot(cfg)` will
use graphviz and eog to plot a handy visualization of a control flow graph,
showing the source code interleaved with GCC's ``GIMPLE`` internal
representation.

Requirements
------------

* GCC: 4.6 or later (it uses APIs that weren't exposed to plugins in 4.5)

* Python: tested with 2.7 and 3.2; it may work with earlier versions

* "six": The libcpychecker code uses the "six" Python compatibility library to
  smooth over Python 2 vs Python 3 differences, both at build-time and
  run-time:

     http://pypi.python.org/pypi/six/

Usage
-----
I use::

    make

to build the plugin and run the tests

You can also use::

   make demo

to demonstrate the new compiler errors.

All of my coding so far has been on Fedora 15 x86_64, using::

    gcc-plugin-devel-4.6.0-0.15.fc15.x86_64

and I don't know to what extent it will be compatible with other versions and
architectures.

The code also makes some assumptions about the Python version you have
installed (grep for "PyRuntime" in the .py files).  I've been using::

    python-devel-2.7.1-5.fc15.x86_64
    python-debug-2.7.1-5.fc15.x86_64
    python3-debug-3.2-0.9.rc1.fc15.x86_64
    python3-devel-3.2-0.9.rc1.fc15.x86_64

but you may have to hack up the `PyRuntime()` invocations in the code to get
it to build on other machines.  Ultimately I want the plugin to be buildable
against multiple python versions, so there could be a python27.so,
python27-debug.so, python-32mu.so, python-32-dmu.so, etc (c.f. PEP-3149)

There isn't an installer yet.  In theory you should be able to add these
arguments to the gcc invocation::

    gcc -fplugin=python.so -fplugin-arg-python-script=PATH_TO_SCRIPT.py OTHER_ARGS

and have it run your script as the plugin starts up.

The plugin automatically adds the absolute path to its own directory to the
end of its `sys.path`, so that it can find support modules, such as gccutils.py
and `libcpychecker`.

The exact API is still in flux; you can currently connect to events by
registering callbacks e.g. to be called for each function in the source at
different passes.

It exposes GCC's various types as Python objects, within a "gcc" module.  You
can see the API by running::

    import gcc
    help(gcc)

from within a script.


Overview of the code
--------------------
This is currently three projects in one:

gcc-python-*: the plugin for GCC.  The entrypoint (`init_plugin`) is in
gcc-python.c

libcpychecker and cpychecker.py: a Python library (and a driver script),
written for the plugin, in which I'm building new compiler warnings to
help people find bugs in CPython extension code.

cpybuilder: a handy module for programatically generating C source code for
CPython extensions.  I use this both to generate parts of the GCC plugin, and
also in the selftests for the cpychecker script.  (I initially attempted to use
Cython for the former, but wrapping the "tree" type hierarchy required more
programatic control)

Coding style: Python and GCC each have their own coding style guide for C.
I've chosen to follow Python's (PEP-7), as I prefer it (although my code is
admittedly a mess in places).

You'll find API documentation within the "docs" directory, written in the
reStructuredText format (as is this file, in fact).  If you have Sphinx
installed (http://sphinx.pocoo.org/), you can regenerate these docs using::

   make html

within the `docs` directory.  Sphinx is the `python-sphinx` package on a
Fedora/RHEL box.


Debugging
---------

gcc can launch subprocesses, so it can be fiddly to debug.

When debugging, I've generally been adding "-v" to the gcc command line
(verbose), so that it outputs the commands that it's running.  I can then use
this to launch::

   $ gdb --args PROGRAM WITH ARGS

This approach to obtaining a debuggable process doesn't seem to work in the
presence of `ccache`, in that it writes to a temporary directory with a name
that embeds the process ID each time, which then gets deleted.  I've worked
around this by uninstalling ccache, but apparently setting::

   CCACHE_DISABLE=1

before invoking `gcc -v` ought to also work around this.

I've also been running into this error from gdb::

  [Thread debugging using libthread_db enabled]
  Cannot find new threads: generic error

Apparently this happens when debugging a process that uses dlopen to load a
library that pulls in libpthread (as does gcc when loading in my plugin), and
a workaround is to link cc1 with -lpthread

The workaround I've been using (to avoid the need to build my own gcc) is to
use LD_PRELOAD, either like this::

   LD_PRELOAD=libpthread.so.0 gdb --args ./cc1 ...

or this::

   (gdb) set environment LD_PRELOAD libpthread.so.0

Handy tricks

Given a (PyGccTree *) named "self"::

   (gdb) call debug_tree(self->t)

will use GCC's prettyprinter to dump the embedded (tree*) and its descendants
to stderr; it can help to put a breakpoint on that function too, to explore the
insides of that type.

Enjoy!
David Malcolm <dmalcolm@redhat.com>
