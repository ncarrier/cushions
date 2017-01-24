# cushions - custom stream handlers integrated by overriding native symbols

## Overview

**libcushions** aims at adding support for url-like schemes to the glibc's
fopen function.
By using linker flags or **LD\_PRELOAD** tricks, this can be done without
modifying or even without recompiling the program, adding transparent support
for archives files, or web urls, for example.

**IMPORTANT NOTICE:** **libcushions**, while already potentially useful in some
use cases, should be considered as a proof of concept.

## Architecture

The first part is **libcushions** itself, it proposes a minimalistic public
API with the *cushions.h* header. The *cushions\_fopen* can be used directly in
a program, or one can use the `-Wl,--wrap=fopen` flag at compilation to
transparently replace fopen calls on the fly with *\_\_wrap\_fopen*, which in
turn, will call *cushions\_fopen*.  
The third possibility is to set the *LD\_PRELOAD* environment variable to the
path to *libcushions.so*, calls to fopen in the program executed, will be
replaced by calls to cushions_fopen transparently and with no compilation step
required.

The second central part is the url scheme handlers. They use the
*cushions\_handler.h* API to implement support for url schemes, by registering
an handler with *cushions\_handler\_register()*. Some handlers are provided and
are listed in the scheme support table at the end of this README.

## Usage example

These two following lines will simplify the next commands:

        $ export LD_LIBRARY_PATH=.
        $ export CUSHIONS_HANDLERS_PATH=handlers_dir

Build the relevant bits and create an example file:

        $ make all examples
        ...
        cc example/cp.c libcushions.so -Wl,--wrap=fopen -o example/cpw
        cc example/cp.c -o example/cp
        $ dd if=/dev/urandom of=tutu bs=1024 count=1024
        1024+0 records in
        1024+0 records out
        1048576 bytes (1.0 MB, 1.0 MiB) copied, 0.0106583 s, 98.4 MB/s

**example/cpw** is built with the --wrap linker option, the fopen glibc symbol
will be overriden by that of libcushions, the program will use it directly.

        $ ./cpw file://tutu file://toto
        $ md5sum tutu toto
        91451e93db0faa7997536d7aa606cfe3  tutu
        91451e93db0faa7997536d7aa606cfe3  toto

On the contrary, **example/cp** has been compiled without even knowing about
libcushions, but using **LD\_PRELOAD**, it's functionality can be added
transparently:

        $ ./cp file://tutu file://toto
        ./cp: fopen src: No such file or directory
        $ rm toto
        $ LD_PRELOAD=./libcushions.so ./cp file://tutu file://toto
        $ md5sum tutu toto
        91451e93db0faa7997536d7aa606cfe3  tutu
        91451e93db0faa7997536d7aa606cfe3  toto

Now a more interesting example, our cpw program will download a file from an
http web site and compress it on the fly in the lzop format:

        $ python -m SimpleHTTPServer &
          Serving HTTP on 0.0.0.0 port 8000 ...
        $ ./cpw http://localhost:8000/src/cushions.c lzo://plop.lzo
        $ lzop -df plop.lzo
        $ md5sum plop src/cushions.c
          de9cc9b40dd030524aacf08d910cf4f0  plop
          de9cc9b40dd030524aacf08d910cf4f0  src/cushions.c
        $ rm -f plop plop.lzo
        $ fg # bring back HTTP server to foreground and kill it
        <Ctrl + C>

Some other programs will be able to use libcushions unmodified, for example, if
you want to:

 * compute the md5sum of the www.perdu.com webpage

        $ LD_PRELOAD=libcushions.so md5sum http://www.perdu.com/

 * execute a lua script from a webpage:

        $ LD_PRELOAD=libcushions.so lua http://download.redis.io/redis-stable/deps/lua/test/hello.lua

## Scheme support table

| handler | description                                    | scheme | read support                                                                                        | write support                                                                                       |
|:-------:|:-----------------------------------------------|:------:|:---------------------------------------------------------------------------------------------------:|:---------------------------------------------------------------------------------------------------:|
| bzip2   | supports for bzip2 compressed file format      | bzip2  | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> |
| curl    | URL transfers support, on top of libcurl       | ftp    | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  |
|         |                                                | http   | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  |
|         |                                                | https  | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  |
|         |                                                | scp    | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  |
|         |                                                | sftp   | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  |
|         |                                                | smb    | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  |
|         |                                                | smbs   | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  |
|         |                                                | tftp   | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  |
| file    | noop                                           | file   | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> |
| lzo     | supports for lzop compressed file format       | lzo    | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> |
| mem     | defers real writes at file close by buffering  | mem    | <img src="https://www.iconfinder.com/icons/32141/download/png/128" alt="no" style="width: 20px;"/>  | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> |
| sock    | opens a client socket                          | csock  | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> |
|         | opens a server socket                          | ssock  | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> | <img src="https://www.iconfinder.com/icons/32133/download/png/128" alt="yes" style="width: 20px;"/> |

[cross]: https://www.iconfinder.com/icons/32141/download/png/128 "no"
