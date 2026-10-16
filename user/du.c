
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static void
usage(void)
{
    fprintf(2, "usage: du [path] [-a] [-s]\n");
}

static void
print_size_line(uint64 size, char *path)
{
    // uint64 có tối đa 20 chữ số, thêm một byte cho '\0'.
    char buf[21];
    char *p = buf + sizeof(buf);

    *--p = '\0';
    do {
        *--p = '0' + size % 10;
        size /= 10;
    } while (size != 0);

    printf("%s\t%s\n", p, path);
}

// Ghép đường dẫn cha và tên entry con
static char *
make_child_path(char *parent, char *name)
{
    uint plen = strlen(parent);
    uint nlen = strlen(name);

    // Tránh tràn kích thước cấp phát
    if (plen > 0x7fffffffU - nlen - 2U)
        return 0;

    char *child = malloc(plen + nlen + 2);

    if (child == 0)
        return 0;

    strcpy(child, parent);

    if (plen > 0 && parent[plen - 1] != '/') {
        child[plen] = '/';
        child[plen + 1] = '\0';
    }

    strcpy(child + strlen(child), name);

    return child;
}

static int
du_walk(char *path, int show_all, int summary,
        int is_root, uint64 *result)
{
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        fprintf(2, "du: cannot open %s\n", path);
        return -1;
    }

    struct stat st;

    if (fstat(fd, &st) < 0) {
        fprintf(2, "du: cannot stat %s\n", path);
        close(fd);
        return -1;
    }

    // File gốc cần -a hoặc -s; file con vẫn được cộng vào tổng thư mục.
    if (is_root && st.type == T_FILE && !show_all && !summary) {
        fprintf(2, "du: %s is a file; use -a or -s to display its size\n",
                path);
        close(fd);
        return -1;
    }

    uint64 total = 0;

    // TRƯỜNG HỢP 1: Đối tượng không phải thư mục
    if (st.type != T_DIR) {
        total = st.size;
    }

    // TRƯỜNG HỢP 2: Đối tượng là thư mục
    else {
        struct dirent de;
        int n;

        while ((n = read(fd, &de, sizeof(de))) ==
               (int)sizeof(de)) {

            if (de.inum == 0)
                continue;

            char name[DIRSIZ + 1];

            memmove(name, de.name, DIRSIZ);
            name[DIRSIZ] = '\0';

            if (strcmp(name, ".") == 0 ||
                strcmp(name, "..") == 0) {
                continue;
            }

            char *child = make_child_path(path, name);

            if (child == 0) {
                fprintf(2, "du: cannot allocate path\n");
                close(fd);
                return -1;
            }

            // Kết quả dung lượng của một mục con
            uint64 child_size = 0;

            int status = du_walk(child,
                                 show_all,
                                 summary,
                                 0,
                                 &child_size);

            free(child);

            if (status < 0) {
                close(fd);
                return -1;
            }

            // Chống tràn khi cộng hai số uint64
            if (total > (uint64)-1 - child_size) {
                fprintf(2, "du: size overflow\n");
                close(fd);
                return -1;
            }

            total += child_size;
        }

        if (n != 0) {
            fprintf(2, "du: cannot read %s\n", path);
            close(fd);
            return -1;
        }
    }

    close(fd);

    // Ghi kết quả cho hàm gọi
    *result = total;

    // Chỉ in các mục không phải gốc.
    // Gốc sẽ được in ở main().
    if (!is_root && !summary &&
        (show_all || st.type == T_DIR)) {
        print_size_line(total, path);
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    char *path = ".";
    int has_path = 0;
    int show_all = 0;
    int summary = 0;

    // Phân tích tham số
    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-a") == 0) {
            show_all = 1;

        } else if (strcmp(argv[i], "-s") == 0) {
            summary = 1;

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

    uint64 total = 0;

    if (du_walk(path, show_all, summary,
                1, &total) < 0) {
        exit(1);
    }

    // In kết quả của path gốc sau khi duyệt và kiểm tra thành công.
    print_size_line(total, path);

    exit(0);
}
