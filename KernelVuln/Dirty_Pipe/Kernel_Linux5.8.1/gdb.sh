#!/bin/sh
pwndbg -q -ex  "target remote localhost:1234" \
    -ex "add-auto-load-safe-path /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1" \
    -ex "file /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/vmlinux" \
    -ex "b do_splice" \
    -ex "c" 
    # -ex "b open" \
    # -ex "b pipe" \

    # -ex "b copy_page_to_iter_pipe" \

#-ex "b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/fs/pipe.c:463" \ #通过pipe_write向管道写入数据 \
# b  /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/fs/splice.c:1174 #wait_for_space等待是否有空闲空间
# b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/lib/iov_iter.c:388 #获取到的buf是被构造出来的还是新的？
# b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/fs/pipe.c:466 #查看获取到的buf是哪一个？是旧的还是新的？在漏洞利用的时候获取到的是序号是16
# 调试漏洞的断点位置！
# pwndbg -q -ex  "target remote localhost:1234" \
#     -ex "add-auto-load-safe-path /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1" \
#     -ex "file /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/vmlinux" \
#     -ex "b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/fs/open.c:1184" \ #open打开的文件结构体，查看file \
#     -ex "b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/fs/pipe.c:882" \ #pipe创建的管道结构体，查看结构体地址 \
#     -ex "b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/fs/pipe.c:536" \ #pipe_write为管道结构体赋予可以合并标记 \
#     -ex "b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/mm/filemap.c:1995" \ #splice获取到的文件结构体，查看file \
#     -ex "b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/mm/filemap.c:2029" \ #generic_file_buffered_read获取只读文件的page \
#     -ex "b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/lib/iov_iter.c:372" \ #文件结构体的page直接替换了管道结构体的page未重新初始化是否可以续写 \
#     -ex "b /home/ub20/KernelStu/KernelEnvInit/linux-5.8.1/linux-5.8.1/fs/pipe.c:463" \ #向管道写入数据，发现可以在管道page续写，但是由于该page实际指向了只读文件的实际page，所以可以实现文件越权写 \
#     -ex "c" 