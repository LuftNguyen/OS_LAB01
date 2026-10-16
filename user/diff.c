#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAXLINE 512
#define MAX_CAPACITY (1U << 30)

#define READ_NOMEM (-2)
#define READ_LIMIT (-3)

// 1: đọc được dòng
// 0: EOF
// -1: lỗi đọc
// -2: hết bộ nhớ
// -3: vượt giới hạn dung lượng an toàn
static int
readline(int fd, char **buf, uint *capacity, int *has_newline)
{
    int len = 0;
    char c;

    *has_newline = 0;

    while (1) {
        int n = read(fd, &c, 1);

        if (n < 0) {
            return -1;
        }

        if (n == 0) {
            if (len == 0)
                return 0;

            (*buf)[len] = '\0';
            return 1;
        }

        if (c == '\n') {
            *has_newline = 1;
            (*buf)[len] = '\0';
            return 1;
        }

        if (len >= *capacity - 1) {
            if (*capacity > MAX_CAPACITY / 2)
                return READ_LIMIT;

            uint newcap = *capacity * 2;
            char *newbuf = malloc(newcap);

            if (newbuf == 0)
                return READ_NOMEM;

            memmove(newbuf, *buf, (int)len);
            free(*buf);

            *buf = newbuf;
            *capacity = newcap;
        }

        (*buf)[len] = c;
        len++;
    }
}

static void
usage(void)
{
    fprintf(2, "usage: diff file1 file2 [-q]\n");
}

// Chỉ nhận file thường để tránh đọc dữ liệu thư mục hoặc thiết bị.
static int
open_regular_file(char *path)
{
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        fprintf(2, "diff: cannot open %s\n", path);
        return -1;
    }

    struct stat st;

    if (fstat(fd, &st) < 0) {
        fprintf(2, "diff: cannot stat %s\n", path);
    } else if (st.type == T_DIR) {
        fprintf(2, "diff: %s is a directory\n", path);
    } else if (st.type != T_FILE) {
        fprintf(2, "diff: %s is not a regular file\n", path);
    } else {
        return fd;
    }

    close(fd);
    return -1;
}

int
main(int argc, char *argv[])
{
    char *file1 = 0;
    char *file2 = 0;
    int quiet = 0;

    // PHẦN 1: Phân tích tham số
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            quiet = 1;
        } else if (argv[i][0] == '-') {
            usage();
            exit(2);
        } else if (file1 == 0) {
            file1 = argv[i];
        } else if (file2 == 0) {
            file2 = argv[i];
        } else {
            usage();
            exit(2);
        }
    }

    if (file1 == 0 || file2 == 0) {
        usage();
        exit(2);
    }

    // PHẦN 2: Mở và kiểm tra loại đối tượng của hai file
    int fd1 = open_regular_file(file1);

    if (fd1 < 0) {
        exit(2);
    }

    int fd2 = open_regular_file(file2);

    if (fd2 < 0) {
        close(fd1);
        exit(2);
    }
    // PHẦN 3: Chuẩn bị buffer
    uint cap1 = MAXLINE;
    uint cap2 = MAXLINE;

    char *buf1 = malloc(cap1);
    char *buf2 = malloc(cap2);

    if (buf1 == 0 || buf2 == 0) {
        fprintf(2, "diff: out of memory\n");

        if (buf1 != 0)
            free(buf1);
        if (buf2 != 0)
            free(buf2);

        close(fd1);
        close(fd2);
        exit(2);
    }

    int line_no = 1;
    int different = 0;

    // PHẦN 4: Đọc và so sánh từng dòng
    while (1) {
        int nl1 = 0;
        int nl2 = 0;

        int r1 = readline(fd1, &buf1, &cap1, &nl1);
        int r2 = readline(fd2, &buf2, &cap2, &nl2);

       // Xử lý lỗi đọc, cấp phát hoặc giới hạn dung lượng
        if (r1 < 0 || r2 < 0) {
            if (r1 == READ_NOMEM || r2 == READ_NOMEM)
                fprintf(2, "diff: out of memory\n");
            else if (r1 == READ_LIMIT || r2 == READ_LIMIT)
                fprintf(2, "diff: line too long\n");
            else
                fprintf(2, "diff: read error\n");

            free(buf1);
            free(buf2);

            close(fd1);
            close(fd2);
            exit(2);
        }

        // Cả hai file đã hết
        if (r1 == 0 && r2 == 0) {
            break;
        }

        // Kiểm tra khác biệt
        int mismatch = 0;

        if (r1 != r2) {
            mismatch = 1;
        } else if (r1 == 1 && r2 == 1) {
            if (strcmp(buf1, buf2) != 0 ||
                nl1 != nl2) {
                mismatch = 1;
            }
        }

        if (mismatch) {
            different = 1;

            if (quiet) {
                printf("diff: files differ\n");
                break;
            }

            // In dòng của file thứ nhất
            if (r1 == 0) {
                printf("%s:%d: < EOF\n",
                       file1, line_no);
            } else {
                printf("%s:%d: < %s%s\n",
                       file1, line_no, buf1,
                       nl1 ? "" : " [no newline]");
            }

            // In dòng của file thứ hai
            if (r2 == 0) {
                printf("%s:%d: > EOF\n",
                       file2, line_no);
            } else {
                printf("%s:%d: > %s%s\n",
                       file2, line_no, buf2,
                       nl2 ? "" : " [no newline]");
            }
        }

        line_no++;
    }

    // PHẦN 5: Giải phóng tài nguyên
    free(buf1);
    free(buf2);

    close(fd1);
    close(fd2);

    exit(different ? 1 : 0);
}
