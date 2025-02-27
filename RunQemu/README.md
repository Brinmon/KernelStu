#!/bin/sh
# 使用Bourne shell执行脚本

qemu-system-x86_64 \  # 启动QEMU模拟x86_64架构的虚拟机
    -m 128M \  # 分配128MB内存给虚拟机
    -kernel ./bzImage \  # 指定Linux内核镜像文件（bzImage）
    -initrd  ./rootfs.cpio \  # 指定初始内存磁盘（initrd）作为临时根文件系统
    -monitor /dev/null \  # 禁用QEMU监视器交互（输出重定向到黑洞设备）
    -append "root=/dev/ram rdinit=/sbin/init console=ttyS0 oops=panic panic=1 loglevel=3 quiet kaslr" \  # 内核启动参数：
        # root=/dev/ram: 使用RAM磁盘作为根文件系统
        # rdinit=/sbin/init: 指定初始进程为/sbin/init
        # console=ttyS0: 控制台输出重定向到串口ttyS0
        # oops=panic panic=1: 内核错误时立即崩溃并1秒后重启
        # loglevel=3 & quiet: 仅显示错误日志，抑制启动信息
        # kaslr: 启用内核地址空间随机化（安全防护）
    -cpu kvm64,+smep \  # 模拟支持KVM加速的64位CPU，并启用SMEP（防内核执行用户空间代码）
    -smp cores=2,threads=1 \  # 分配2个CPU核心，每个核心1线程（共2个vCPU）
    -nographic \  # 禁用图形界面，直接使用当前终端作为控制台
    -s  # 开启GDB调试端口（默认TCP:1234），允许通过gdb连接调试内核