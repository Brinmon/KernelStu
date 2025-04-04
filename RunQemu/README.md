# Kernel Development Environment with QEMU

## 目录

1. [环境概述](#环境概述)
2. [文件说明](#文件说明)
3. [快速开始](#快速开始)
4. [详细使用指南](#详细使用指南)
   - [启动虚拟机](#启动虚拟机)
   - [调试内核模块](#调试内核模块)
   - [文件系统打包](#文件系统打包)
5. [高级配置](#高级配置)
6. [常见问题](#常见问题)

---

## 环境概述

本仓库提供完整的Linux内核开发与调试环境，包含以下核心组件：

- **QEMU虚拟机**：运行自定义内核和文件系统
- **GDB调试支持**：通过pwndbg进行内核级调试
- **自动打包工具**：支持内核模块和启动脚本的热更新
- **安全加固配置**：启用SMEP/KASLR等安全特性

---

## 文件说明

| 文件名       | 用途说明                              |
|--------------|-------------------------------------|
| `start.sh`   | 启动标准QEMU虚拟机                   |
| `debugk.sh`  | 启动带调试支持的QEMU实例             |
| `gdb.sh`     | 连接GDB进行内核调试                  |
| `repackko.sh`| 文件系统打包工具（支持自动脚本替换） |
| `bzImage`    | 压缩后的Linux内核镜像                |
| `rootfs.cpio`| 初始内存磁盘镜像                     |

---

## 快速开始

### 前置要求
```bash
# 安装依赖
sudo apt install qemu-system-x86 gdb cpio pwndbg
```

### 基础工作流
1. 修改内核模块代码
2. 重新打包文件系统
   ```bash
   ./repackko.sh my_module.ko
   ```
3. 启动虚拟机
   ```bash
   ./start.sh
   ```
4. 调试模块
   ```bash
   ./gdb.sh 0x$(cat /proc/modules | grep my_module | awk '{print $6}')
   ```

---

## 详细使用指南

### 启动虚拟机

#### 标准启动（无调试）
```bash
./start.sh
```

#### 调试模式启动
```bash
./debugk.sh
```
> 该模式会：
> - 开启GDB调试端口（1234）
> - 关闭KASLR地址随机化
> - 保留内核符号信息

### 调试内核模块

1. 在QEMU中获取模块加载地址
   ```bash
   cat /proc/modules | grep hello
   # 输出示例：hello 16384 0 - Live 0xffffffffc0000000 (O)
   ```

2. 启动GDB调试会话
   ```bash
   ./gdb.sh 0xffffffffc0000000
   ```
   > 脚本自动完成：
   > - 连接QEMU调试端口
   > - 加载模块符号信息
   > - 在关键函数设置断点

### 文件系统打包

#### 基本用法
```bash
# 添加普通文件
./repackko.sh my_app config.txt

# 添加内核模块（自动生成启动脚本）
./repackko.sh my_driver.ko
```

#### 高级用法
```bash
# 替换初始化脚本
./repackko.sh custom_init init

# 替换启动脚本并添加模块
./repackko.sh new_rcs rcS network.ko
```

#### 生成的文件结构
```bash
rootfs_new.cpio.gz
├── etc/
│   └── init.d/
│       └── rcS         # 自动生成的启动脚本
├── home/
│   └── my_app         # 用户添加的应用程序
└── lib/
    └── modules/
        └── my_driver.ko  # 内核模块
```

---

## 高级配置

### QEMU参数定制
修改`start.sh`中的参数：
```bash
-m 256M               # 内存大小调整
-smp cores=4          # CPU核心数设置
-cpu kvm64,+smep,+smap # 安全特性配置
```

### 内核启动参数
调整`-append`选项：
```bash
console=ttyS0         # 控制台输出设置
loglevel=7            # 调试日志级别
nokaslr               # 关闭地址随机化（调试用）
```

---

## 常见问题

### Q1: 文件系统打包失败
**现象**：
```
[错误] 解压失败，请检查 cpio 文件完整性
```
**解决方案**：
1. 验证原始文件系统完整性
   ```bash
   cpio -t < rootfs.cpio
   ```
2. 检查磁盘空间
   ```bash
   df -h .
   ```

### Q2: 内核模块加载失败
**现象**：
```
insmod: ERROR: could not insert module my.ko: Invalid module format
```
**解决方案**：
1. 确认内核版本一致
   ```bash
   uname -r
   ```
2. 检查模块编译配置
   ```bash
   make CONFIG_DEBUG_INFO=y
   ```

### Q3: GDB连接超时
**现象**：
```
Remote 'g' packet reply is too long
```
**解决方案**：
1. 确认QEMU已启动调试模式
2. 检查防火墙设置
   ```bash
   sudo ufw allow 1234
   ```

---

> 本环境持续更新，建议定期执行 `git pull` 获取最新改进  
> 遇到新问题请提交Issue并附上相关日志信息