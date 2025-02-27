#!/bin/sh
pwndbg -q -ex "target remote localhost:1234" \
    -ex "add-symbol-file /home/ub20/KernelStu/CodeKernelDriver/hello.ko $1" \
    -ex "b hello_init" \
    -ex "b core_write" \
    -ex "b core_ioctl" \
    -ex "c"
