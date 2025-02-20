#!/bin/bash

# 检查是否提供了参数
if [ $# -eq 0 ]; then
    echo "用法: $0 <文件1> <文件2> ..."
    exit 1
fi

# 解压磁盘镜像
echo "解压磁盘镜像..."
cpio -idv < ./rootfs.cpio

# 将文件复制到解压后的目录中
echo "添加文件到解压后的目录..."
for file in "$@"; do
    if [ -f "$file" ]; then
        cp "$file" ./
        echo "已添加文件: $file"
    else
        echo "警告: 文件 $file 不存在，跳过。"
    fi
done

# 重新打包磁盘镜像
echo "重新打包磁盘镜像..."
find . | cpio -o -H newc > ./rootfs_new.cpio

echo "操作完成。新的磁盘镜像已保存为 rootfs_new.cpio"