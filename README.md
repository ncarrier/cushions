# cushions - custom stream handlers integrated by overriding native symbols

## Overview

**libcushions** aims at adding support for url-like schemes to the glibc's
fopen function.
By using linker flags or **LD\_PRELOAD** tricks, this can be done without
modifying or even without recompiling the program, adding transparent support
for archives files, or web urls, for example.

**IT IS NOT FUNCTIONAL YET**

## Architecture

The central part is **libcushions** itself, it proposes a public API

--- WIP ---

## Usage example

These two lines will simplify the next commands:

        $ export CUSHIONS_LOG_LEVEL=4
        $ export LD_LIBRARY_PATH=.

Build the relevant bits and create an example file:

        $ make examples
        ...
        cc example/cp.o libcushions.so -Wl,--wrap=fopen -o example/cp
        cc example/cp.o -o example/cp_no_wrap
        $ dd if=/dev/urandom of=tutu bs=1024 count=1024
        1024+0 records in
        1024+0 records out
        1048576 bytes (1.0 MB, 1.0 MiB) copied, 0.0106583 s, 98.4 MB/s

**example/cp** is built with the --wrap linker option, the fopen glibc symbol
will be overriden by that of libcushions, the program will use it directly.

        $ ./example/cp file://tutu file://toto
        $ md5sum tutu toto
        91451e93db0faa7997536d7aa606cfe3  tutu
        91451e93db0faa7997536d7aa606cfe3  toto

On the contrary, **example/cp\_no\_wrap** has been compiled without even knowing
about libcushions, but using **LD\_PRELOAD**, it's functionality can be added
transparently:

        $ ./example/cp_no_wrap file://tutu file://toto
        ./example/cp_no_wrap: fopen src: No such file or directory
        $ rm toto
        $ LD_PRELOAD=./libcushions.so ./example/cp_no_wrap file://tutu file://toto
        $ md5sum tutu toto
        91451e93db0faa7997536d7aa606cfe3  tutu
        91451e93db0faa7997536d7aa606cfe3  toto

