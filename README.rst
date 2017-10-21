rcbin
=====

rcbin lets you embed arbitrary resource data into your executables *after* they've already
been compiled.

Ever wanted to change your version number after compiling your binary, or allow someone
else to embed something into it after it's been built? Trying to embed a scripting language
into native binaries, except you need to embed the script *after* normal compilation?

***rcbin may be what you're looking for!***

Build instructions
******************

Due to some complications regarding recent Fbuild changes, CMake is temporarily replacing
Fbuild for building. Just build it like you would any other CMake project::

  $ mkdir build
  $ cd build
  $ cmake .. -G Ninja
  $ ninja

or Windows/MSVC equivalent.

Usage
*****

Here's an example:

.. code-block:: c

  #include <rcbin.h>

  // RCBIN_IMPORT is used to define a resource that rcbin will later include. This will define a
  // function myresource() that can be used to load the resource.
  RCBIN_IMPORT(myresource)

  int main() {
      if (!rcbin_init()) {
          puts("Failed to initialize rcbin");
          return 1;
      }

      // Load our resource by calling myresource().
      rcbin_entry res = myresource();
      // If res.data != NULL, then the resource was loaded successfully. The data is in res.data,
      // and the size in res.sz.
      if (res.data != NULL) {
          printf("myresource: %.*s\n", (int)res.sz, res.data);
          return 0;
      } else {
          // Otherwise, fail.
          puts("Failed to get myresource!");
          return 1;
      }
  }

Now, after building your program, run rcbin on it::

  $ rcbin my_program myresource =message

*rcbin* takes the program to modify and an even number of arguments. First comes the
resource name, then its contents preceded by an equals sign. This command will define
*myresource* to be equal to the string *message*.

If you want to read the contents of a file into a resource instead, use ``@``::

  $ rcbin my_program myresource @my_file

This will define *myresource* to be equal to the contents of *my_file*.

Need help?
**********

Open up an issue, or `ping me on Twitter <https://twitter.com/refi_64>`_.
