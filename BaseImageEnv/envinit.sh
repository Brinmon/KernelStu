#创建系统所需的基础文件目录结构
mkdir -pv {bin,tmp,sbin,etc,proc,sys,home,lib64,lib/x86_64-linux-gnu,usr/{bin,sbin}}
touch etc/inittab
mkdir etc/init.d
touch etc/init.d/rcS
chmod +x ./etc/init.d/rcS

# 创建 /etc/inittab 并写入内容
cat > etc/inittab << EOF
::sysinit:/etc/init.d/rcS
::askfirst:/bin/ash
::ctrlaltdel:/sbin/reboot
::shutdown:/sbin/swapoff -a
::shutdown:/bin/umount -a -r
::restart:/sbin/init
EOF

# 创建 /etc/init.d/rcS 并写入内容
cat > etc/init.d/rcS << EOF
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs devtmpfs /dev
mount -t tmpfs tmpfs /tmp
mkdir /dev/pts
mount -t devpts devpts /dev/pts

echo -e "\nBoot took \$(cut -d' ' -f1 /proc/uptime) seconds\n"
setsid cttyhack setuidgid 1000 sh

poweroff -d 0 -f
EOF

# 配置用户组和用户
echo "root:x:0:0:root:/root:/bin/sh" > etc/passwd
echo "ctf:x:1000:1000:ctf:/home/ctf:/bin/sh" >> etc/passwd
echo "root:x:0:" > etc/group
echo "ctf:x:1000:" >> etc/group
echo "none /dev/pts devpts gid=5,mode=620 0 0" > etc/fstab