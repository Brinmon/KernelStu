#!/bin/bash
set -eo pipefail  # 增强错误检查

# 记录当前工作目录
current_dir=$(pwd)

# 检查参数
if [ $# -eq 0 ]; then
    echo "用法: $0 <文件1> <文件2> ..."
    echo "特殊处理："
    echo "  - 文件名若为 init 自动替换系统初始化脚本"
    echo "  - 文件名若为 rcS 自动替换启动脚本"
    exit 1
fi

# 创建临时目录
temp_dir=$(mktemp -d)
echo "[临时目录] 创建于：$temp_dir"
trap 'rm -rf "$temp_dir"' EXIT

# 解压原始镜像
echo "[解压] 正在解压 rootfs.cpio..."
if [ ! -f ./rootfs.cpio ]; then
    echo "[错误] 找不到 rootfs.cpio 文件" >&2
    exit 1
fi
cpio -idmv < ./rootfs.cpio  -D "$temp_dir" || {
    echo "错误: 解压 rootfs.cpio 失败"
    exit 1
}

declare -A special_files=(
    ["init"]="$temp_dir/init"
    ["rcS"]="$temp_dir/etc/init.d/rcS"
)

has_ko=false
ko_files=()
processed_special=()

# 文件处理流程
echo "[文件处理] 开始处理输入文件..."
for file in "$@"; do
    # 验证文件存在性
    if [ ! -f "$file" ]; then
        echo "[警告] 忽略不存在的文件: $file" >&2
        continue
    fi

    filename=$(basename "$file")
    target_path=""

    # 特殊文件处理
    if [[ -v special_files[$filename] ]]; then
        target_path=${special_files[$filename]}
        echo "[特殊文件] 检测到系统脚本: $filename"
        echo "          目标路径: ${target_path#$temp_dir}"
        
        # 创建目标目录
        mkdir -p "$(dirname "$target_path")"
        
        # 保留原文件属性
        if cp -v --preserve=all "$file" "$target_path"; then
            chmod +x "$target_path"  # 确保可执行
            processed_special+=("$filename")
            continue
        else
            echo "[错误] 复制特殊文件失败: $file" >&2
            exit 1
        fi
    fi

    # 内核模块处理
    if [[ "$filename" == *.ko ]]; then
        has_ko=true
        ko_files+=("$filename")
        target_path="$temp_dir/$filename"
    else
        target_path="$temp_dir/$filename"
    fi

    # 普通文件复制
    if cp -vL --preserve=all "$file" "$target_path"; then
        echo "[成功] 已添加文件: $filename"
    else
        echo "[错误] 文件复制失败: $file" >&2
        exit 1
    fi
done

# 内核模块处理（当没有自定义 rcS 时）
if $has_ko && [[ ! " ${processed_special[@]} " =~ " rcS " ]]; then
    echo "[内核模块] 检测到 .ko 文件，生成启动脚本..."
    rcs_path="$temp_dir/etc/init.d/rcS"
    
    # 创建目录结构
    mkdir -p "$(dirname "$rcs_path")"
    
    # 生成启动脚本
    {
        echo "#!/bin/sh"
        echo "echo '==== 系统启动中 ===='"
        echo "mount -t proc none /proc"
        echo "mount -t sysfs none /sys"
        echo "mount -t devtmpfs none /dev"
        echo "mount -t debugfs none /sys/kernel/debug"
        echo "mount -t tmpfs none /tmp"
        echo "echo -e \"启动耗时: \$(awk '{print \$1}' /proc/uptime) 秒\""
        
        for ko in "${ko_files[@]}"; do
            echo "if insmod /$ko; then"
            echo "    echo '成功加载内核模块: $ko'"
            echo "else"
            echo "    echo '加载失败: $ko' >&2"
            echo "fi"
        done
        
        echo "setsid /bin/cttyhack setuidgid 0 /bin/sh"
        echo "echo '正在关机...'"
        echo "poweroff -f"
    } > "$rcs_path"

    chmod +x "$rcs_path"
    echo "[生成脚本] 启动脚本已创建于 /etc/init.d/rcS"
fi

# 重新打包镜像到当前目录
echo "重新打包磁盘镜像..."
(cd "$temp_dir" && find . -print0 | cpio -o -H newc --null > "$current_dir/rootfs_new.cpio") || {
    echo "错误: 打包 rootfs_new.cpio 失败"
    exit 1
}

echo "操作完成。新的磁盘镜像已保存为 $current_dir/rootfs_new.cpio"