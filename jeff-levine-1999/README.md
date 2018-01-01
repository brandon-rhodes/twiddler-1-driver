
# Jeff Levine’s original Linux driver for the original Twiddler

In case the original ever disappears from the Web,
here are the contents of the archive
[twid-linux.tar.gz](https://www.media.mit.edu/wearables/lizzy/twid-linux.tar.gz)
that can be downloaded from the “Keyboards” page
of the old Lizzy wearable computer page
at the MIT Media Group:

https://www.media.mit.edu/wearables/lizzy/keyboards.html

The archive includes binaries ``a2x`` and ``twiddler``
but I cannot invoke either of them on a modern Linux machine:

    $ file a2x twiddler
    a2x:      Linux/i386 demand-paged executable (QMAGIC)
    twiddler: ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux.so.1, not stripped

    $ ./a2x
    zsh: exec format error: ./a2x

    $ ./twiddler
    zsh: no such file or directory: ./twiddler

    $ ldd ./twiddler
            linux-gate.so.1 =>  (0xf7705000)
            libc.so.5 => not found

I was, however, able to successfully recompile ``twiddler.c``
and the accompanying ``a2x`` utility
and have included the results here as ``twiddler.x86_64`` and ``a2x.x86_64``.

— Brandon Rhodes, 2018 January 1
