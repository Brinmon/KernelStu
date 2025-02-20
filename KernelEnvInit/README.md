因为Build.sh默认编译的就是x86_64架构的内核,所以不需要配置选项

编译结果:
```
ub20@ub20:~/KernelStu/linux-5.11$ ls /home/ub20/KernelStu/linux-5.11/arch/x86_64/boot/bzImage
/home/ub20/KernelStu/linux-5.11/arch/x86_64/boot/bzImage
ub20@ub20:~/KernelStu/linux-5.11$ ls /home/ub20/KernelStu/linux-5.11/vmlinux
/home/ub20/KernelStu/linux-5.11/vmlinux
```

vmlinux：原始内核文件
在当前目录下提取到vmlinux，为编译出来的原始内核文件

bzImage：压缩内核镜像
在当前目录下的arch/x86（其他架构都有）/boot/目录下提取到bzImage，为压缩后的内核文件，适用于大内核