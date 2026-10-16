#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// In hướng dẫn sử dụng
static void
usage(void)
{
    fprintf(2, "usage: tree [path] [-L depth] [-d]\n");
}

// Chuyển chuỗi độ sâu thành số nguyên không âm
// Trả 0 nếu hợp lệ, -1 nếu không hợp lệ
static int parse_depth(char *s, int *result)
{
    int value = 0;

    if (s[0] == '\0')
        return -1;

    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9')
            return -1;

        int digit = s[i] - '0';

        if (value > (2147483647 - digit) / 10)
            return -1;

        value = value * 10 + digit;
    }

    *result = value;
    return 0;
}

// In một mục ở độ sâu tương ứng
static void print_entry(char *name, int level)
{
    for (int i = 0; i < level; i++) {
        printf("  ");
    }

    printf("|-- %s\n", name);
}

// Tạo đường dẫn con trên heap
static char * make_child_path(char *parent, char *name)
{
    uint plen = strlen(parent);
    uint nlen = strlen(name);

    if (plen > 0x7fffffffU - nlen - 2U)
        return 0;

    char *child = malloc(plen + nlen + 2);

    if (child == 0)
        return 0;

    strcpy(child, parent);

    // Tránh thêm hai dấu / liên tiếp
    if (plen > 0 && parent[plen - 1] != '/') {
        child[plen] = '/';
        child[plen + 1] = '\0';
    }

    strcpy(child + strlen(child), name);

    return child;
}

static int tree_walk(char *path, int level,
          int max_depth, int only_dirs)
{
    // Đã tới giới hạn độ sâu thì không đọc tiếp
    if (max_depth >= 0 && level >= max_depth)
        return 0;

    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        fprintf(2, "tree: cannot open %s\n", path);
        return -1;
    }

    struct dirent de;
    int n;

    while ((n = read(fd, &de, sizeof(de))) ==
           (int)sizeof(de)) {

        // Bỏ qua slot rỗng
        if (de.inum == 0)
            continue;

        // Chuyển tên entry thành chuỗi C hợp lệ
        char name[DIRSIZ + 1];

        memmove(name, de.name, DIRSIZ);
        name[DIRSIZ] = '\0';

        // Tránh duyệt lại thư mục hiện tại / cha
        if (strcmp(name, ".") == 0 ||
            strcmp(name, "..") == 0) {
            continue;
        }

        // Tạo đường dẫn đầy đủ của entry
        char *child = make_child_path(path, name);

        if (child == 0) {
            fprintf(2, "tree: cannot allocate path\n");
            close(fd);
            return -1;
        }

        // Mở entry để xác định loại
        int child_fd = open(child, O_RDONLY);

        if (child_fd < 0) {
            fprintf(2, "tree: cannot open %s\n", child);
            free(child);
            close(fd);
            return -1;
        }

        struct stat st;

        if (fstat(child_fd, &st) < 0) {
            fprintf(2, "tree: cannot stat %s\n", child);
            close(child_fd);
            free(child);
            close(fd);
            return -1;
        }

        close(child_fd);

        // Nếu không bật -d, in mọi mục.
        // Nếu bật -d, chỉ in thư mục.
        if (!only_dirs || st.type == T_DIR) {
            print_entry(name, level + 1);
        }

        // Nếu là thư mục, tiếp tục duyệt bên trong
        if (st.type == T_DIR) {
            if (tree_walk(child, level + 1,
                          max_depth, only_dirs) < 0) {
                free(child);
                close(fd);
                return -1;
            }
        }

        // Không còn cần đường dẫn con
        free(child);
    }

    // n == 0 nghĩa là đã đọc hết thư mục.
    // n khác 0 là lỗi hoặc bản ghi không đầy đủ.
    if (n != 0) {
        fprintf(2, "tree: cannot read %s\n", path);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    char *path = ".";
    int has_path = 0;
    int max_depth = -1;
    int only_dirs = 0;

    // Phân tích tham số
    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-d") == 0) {
            only_dirs = 1;

        } else if (strcmp(argv[i], "-L") == 0) {

            // -L bắt buộc có một giá trị ngay sau
            if (i + 1 >= argc ||
                parse_depth(argv[i + 1], &max_depth) < 0) {
                usage();
                exit(1);
            }

            i++; // Bỏ qua tham số độ sâu đã xử lý

        } else if (argv[i][0] == '-') {
            usage();
            exit(1);

        } else if (!has_path) {
            path = argv[i];
            has_path = 1;

        } else {
            usage();
            exit(1);
        }
    }

    // Kiểm tra path tồn tại và lấy loại
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        fprintf(2, "tree: cannot open %s\n", path);
        exit(1);
    }

    struct stat st;

    if (fstat(fd, &st) < 0) {
        fprintf(2, "tree: cannot stat %s\n", path);
        close(fd);
        exit(1);
    }

    close(fd);

    // Với -d, nếu path là file thì không in
    if (only_dirs && st.type != T_DIR)
        exit(0);

    // In gốc đúng một lần
    printf("%s\n", path);

    // Chỉ duyệt đệ quy nếu path là thư mục
    if (st.type == T_DIR) {
        if (tree_walk(path, 0, max_depth, only_dirs) < 0)
            exit(1);
    }

    exit(0);
}
