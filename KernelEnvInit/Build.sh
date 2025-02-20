wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.11.tar.xz
tar -xvf linux-5.11.tar.xz
cd linux-5.11/
sudo apt-get update
sudo apt-get install git fakeroot build-essential ncurses-dev xz-utils qemu flex libncurses5-dev libssl-dev bc bison libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev libelf-dev dwarves zstd
#make menuconfig  命令需要依赖库,下面的
sudo apt-get install libncurses5-dev libncursesw5-dev
make menuconfig

#避免make 的时候报错,直接将.config内的CONFIG_SYSTEM_TRUSTED_KEYS字段置空不然会报错
sed -i 's/^\(CONFIG_SYSTEM_TRUSTED_KEYS=\).*/\1""/' .config
cp .config .config.bak


#最后多核编译就可以了,15.44开始的
make -j$(nproc) bzImage