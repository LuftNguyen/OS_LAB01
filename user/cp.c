#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "user/user.h"

#define BUFFER_SIZE 512

static int write_all(int fd, char* buffer, int size){
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

    int src_fd = open(argv[1], O_RDONLY);
    if(src_fd < 0) {
        fprintf(2, "cp: cannot open %s\n", argv[1]);
        exit(1);
    }

    struct stat src_st;

    if(fstat(src_fd, &src_st) < 0) {
        fprintf(2, "cp: cannot stat %s\n", argv[1]);
        close(src_fd);
        exit(1);
    }

    if(src_st.type != T_FILE) {
        fprintf(2, "cp: %s is not a regular file\n", argv[1]);
        close(src_fd);
        exit(1);
    }

    int check_fd = open(argv[2], O_WRONLY | O_CREATE);

    if(check_fd < 0) {
        fprintf(2, "cp: cannot open %s\n", argv[2]);
        close(src_fd);
        exit(1);
    }

    struct stat dst_st;

    if(fstat(check_fd, &dst_st) < 0) {
        fprintf(2, "cp: cannot stat %s\n", argv[2]);
        close(src_fd);
        close(check_fd);
        exit(1);
    }

    if(dst_st.type != T_FILE) {
        fprintf(2, "cp: %s is not a regular file\n", argv[2]);
        close(src_fd);
        close(check_fd);
        exit(1);
    }

    if(src_st.dev == dst_st.dev && src_st.ino == dst_st.ino) {
        fprintf(2, "cp: %s and %s are the same file\n", argv[1], argv[2]);
        close(src_fd);
        close(check_fd);
        exit(1);
    }

    close(check_fd);

    int dst_fd = open(argv[2], O_WRONLY | O_CREATE | O_TRUNC);

    if(dst_fd < 0) {
        fprintf(2, "cp: cannot open %s\n", argv[2]);
        close(src_fd);
        exit(1);
    }

    while((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0){
        if(write_all(dst_fd, buffer, bytes_read) < 0){
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