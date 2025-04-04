#!/bin/sh
qemu-system-x86_64 \
    -m 128M \
    -kernel ./bzImage \
    -initrd  ./rootfs_new.cpio \
    -monitor /dev/null \
    -append "root=/dev/ram rdinit=/sbin/init console=ttyS0 oops=panic panic=1 quiet nokaslr loglevel=7" \
    -cpu kvm64,+smep \
    -smp cores=2,threads=1 \
    -nographic \
    -s 