rcbin
=====

rcbin lets you embed arbitrary resource data into your executables *after* they've already
been compiled.

Ever wanted to change your version number after compiling your binary, or allow someone
else to embed something into it after it's been built? Trying to embed a scripting language
into native binaries, except you need to embed the script *after* normal compilation?

***rcbin may be what you're looking for!***

For more info, check out `the website <https://refi64.com/proj/rcbin.html>`_. If you want
to build it yourself, read on.

Build instructions
******************

You need `fbuild <https://github.com/felix-lang/fbuild>`_ from the *dev* branch. Run::

  $ fbuild

to configure and build.

If you're cross-compiling for Windows from Linux, instead use::

  $ fbuild --cc=x86_64-w64-mingw32-cc --platform="{'posix', 'mingw'}"
