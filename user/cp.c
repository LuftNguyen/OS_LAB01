#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BUFFER_SIZE 512

static int copy_file(int fd, char* buffer, int size){
    int written = 0;
    while(written < size){
        int n = write(fd, buffer + written, size - written);

        if(n <= 0){
            return -1;
        }

        written += n;
    }
    return 0;
}

int main (int argc, char* argv[]){
    char buffer[BUFFER_SIZE];
    int bytes_read;

    if(argc != 3){
        fprintf(2,"usage: cp src dst\n");
        exit(1);
    }

    if(strcmp(argv[1], argv[2]) == 0){
        fprintf(2, "cp: source and destination are the same file\n");
        exit(1);
    }

    int src_fd = open(argv[1], O_RDONLY);
    if(src_fd < 0){
        fprintf(2, "cp: cannot open %s\n", argv[1]);
        exit(1);
    }

    int dst_fd = open(argv[2], O_WRONLY | O_CREATE | O_TRUNC);
    if(dst_fd < 0){
        fprintf(2, "cp: cannot open %s\n", argv[2]);
        close(src_fd);
        exit(1);
    }

    while((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0){
        if(copy_file(dst_fd, buffer, bytes_read) < 0){
            fprintf(2, "cp: write error\n");
            close(src_fd);
            close(dst_fd);
            exit(1);
        }
    }

    if(bytes_read < 0){
        fprintf(2, "cp: read error\n");
        close(src_fd);
        close(dst_fd);
        exit(1);
    }

    close(src_fd);
    close(dst_fd);
    exit(0);
}