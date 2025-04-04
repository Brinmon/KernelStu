#!/bin/sh
qemu-system-x86_64 \
    -m 128M \
    -kernel bzImage \
    -initrd rootfs.cpio \
    -append 'root=/dev/ram console=ttyS0 oops=panic panic=1' \
    -monitor /dev/null \
    -cpu kvm64,+smep \
    -monitor /dev/null \
    -smp cores=1,threads=1 \
    -netdev user,id=t0, -device e1000,netdev=t0,id=nic0 \
    -nographic