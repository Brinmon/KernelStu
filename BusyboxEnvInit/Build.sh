wget https://busybox.net/downloads/busybox-1.33.0.tar.bz2
tar -jxvf busybox-1.33.0.tar.bz2
cd busybox-1.33.0/
#需要手动配置静态编译,不然配置和使用都比较麻烦
make menuconfig
make install

