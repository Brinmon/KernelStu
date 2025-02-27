#!/bin/bash
set -e  # 遇到错误立即退出

# 记录当前工作目录
current_dir=$(pwd)

# 检查参数
if [ $# -eq 0 ]; then
    echo "用法: $0 <文件1> <文件2> ..."
    exit 1
fi

# 创建临时目录
temp_dir=$(mktemp -d)
echo "使用临时目录: $temp_dir"
trap 'rm -rf "$temp_dir"' EXIT  # 退出时自动清理

# 解压原始镜像到临时目录
echo "解压磁盘镜像..."
if [ ! -f ./rootfs.cpio ]; then
    echo "错误: rootfs.cpio 文件不存在"
    exit 1
fi
cpio -idmv < ./rootfs.cpio  -D "$temp_dir" || {
    echo "错误: 解压 rootfs.cpio 失败"
    exit 1
}

has_ko=false
ko_files=()

# 复制文件并记录.ko文件
echo "添加文件到临时目录..."
for file in "$@"; do
    if [ -f "$file" ]; then
        if cp -vL "$file" "$temp_dir/"; then
            if [[ "$file" == *.ko ]]; then
                has_ko=true
                ko_files+=("$(basename "$file")")
            fi
        else
            echo "警告: 复制 $file 失败，跳过。"
        fi
    else
        echo "警告: 文件 $file 不存在，跳过。"
    fi
done

# 处理.ko文件
if $has_ko; then
    echo "检测到内核模块，修改启动脚本..."
    rcs_path="$temp_dir/etc/init.d/rcS"
    
    # 确保目录存在
    mkdir -p "$(dirname "$rcs_path")"
    
    # 创建并写入rcS内容
    {
        echo "#!/bin/sh"
        echo "echo \"INIT SCRIPT\""
        echo "mount -t proc none /proc"
        echo "mount -t sysfs none /sys"
        echo "mount -t devtmpfs none /dev"
        echo "mount -t debugfs none /sys/kernel/debug"
        echo "mount -t tmpfs none /tmp"
        echo "echo -e \"Boot took \$(cut -d' ' -f1 /proc/uptime) seconds\""
        for ko in "${ko_files[@]}"; do
            echo "insmod /$ko && echo '已加载模块: $ko' || echo '加载失败: $ko'"
        done
        #进入交互式 shell
        #1.以普通用户启动交互窗口
        # echo "setsid /bin/cttyhack setuidgid 1000 /bin/sh"
        #2.以root用户启动交互窗口
        echo "setsid /bin/cttyhack setuidgid 0 /bin/sh"

        # 关机
        # echo "poweroff -f"
    } > "$rcs_path"

    # 确保可执行权限
    chmod +x "$rcs_path"
    echo "已修改启动脚本：$rcs_path"
fi

# 重新打包镜像到当前目录
echo "重新打包磁盘镜像..."
(cd "$temp_dir" && find . -print0 | cpio -o -H newc --null > "$current_dir/rootfs_new.cpio") || {
    echo "错误: 打包 rootfs_new.cpio 失败"
    exit 1
}

echo "操作完成。新的磁盘镜像已保存为 $current_dir/rootfs_new.cpio"