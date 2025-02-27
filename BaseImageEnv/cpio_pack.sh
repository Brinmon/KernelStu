#!/bin/bash

# 输出文件名
OUTPUT_FILE="./rootfs.cpio"

# 使用 find 和 cpio 打包
find . | cpio -o --format=newc > "$OUTPUT_FILE"

# 或者你可以使用这个命令：
# find . | cpio -o -H newc > "$OUTPUT_FILE"

# 检查是否成功
if [ $? -eq 0 ]; then
    echo "成功创建 cpio 文件: $OUTPUT_FILE"
else
    echo "创建 cpio 文件时出错"
    exit 1
fi
