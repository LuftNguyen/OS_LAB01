
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static void
write_checked(int fd, const char *content, int n)
{
    if (write(fd, content, n) != n) {
        fprintf(2, "write error\n");
        close(fd);
        exit(1);
    }
}

static void
create_file(char *name, char *content)
{
    int fd = open(name, O_CREATE | O_WRONLY | O_TRUNC);

    if (fd < 0) {
        fprintf(2, "cannot create %s\n", name);
        exit(1);
    }

    write_checked(fd, content, strlen(content));

    close(fd);
}

// length la so byte noi dung, khong tinh newline; cac loi goi dung length > 0.
// Ghi theo khoi nho de khong dat ca dong dai tren stack.
static void
create_long_file(char *name, int length, char last,
                 int has_newline, char *tail)
{
    int fd = open(name, O_CREATE | O_WRONLY | O_TRUNC);
    if (fd < 0) {
        fprintf(2, "cannot create %s\n", name);
        exit(1);
    }

    char block[128];
    memset(block, 'a', sizeof(block));

    int remaining = length - 1;
    while (remaining > 0) {
        int n = remaining < sizeof(block) ? remaining : sizeof(block);
        write_checked(fd, block, n);
        remaining -= n;
    }

    write_checked(fd, &last, 1);
    if (has_newline)
        write_checked(fd, "\n", 1);
    write_checked(fd, tail, strlen(tail));
    close(fd);
}

int
main(void)
{
    create_file("f1",
                "apple\nbanana\ncarrot\n");

    create_file("f2",
                "apple\nblueberry\ncarrot\ndate\n");

    create_file("empty", "");

    create_file("last1", "hello\n");
    create_file("last2", "hello");

    // Hai file rieng biet co noi dung giong nhau.
    create_file("f1copy", "apple\nbanana\ncarrot\n");

    // Moi moc co ban goc, ban giong nhau va ban khac byte cuoi.
    create_long_file("l511", 511, 'a', 1, "");
    create_long_file("l511_copy", 511, 'a', 1, "");
    create_long_file("l511_diff", 511, 'b', 1, "");
    create_long_file("l512", 512, 'a', 1, "");
    create_long_file("l512_copy", 512, 'a', 1, "");
    create_long_file("l512_diff", 512, 'b', 1, "");
    create_long_file("l1023", 1023, 'a', 1, "");
    create_long_file("l1023_copy", 1023, 'a', 1, "");
    create_long_file("l1023_diff", 1023, 'b', 1, "");
    create_long_file("l1024", 1024, 'a', 1, "");
    create_long_file("l1024_copy", 1024, 'a', 1, "");
    create_long_file("l1024_diff", 1024, 'b', 1, "");

    // Mo rong nhieu lan va EOF khong co newline.
    create_long_file("l8192", 8192, 'a', 1, "");
    create_long_file("l8192_copy", 8192, 'a', 1, "");
    create_long_file("l8192_diff", 8192, 'b', 1, "");
    create_long_file("l1024_nonl", 1024, 'a', 0, "");
    create_long_file("l1024_ndiff", 1024, 'b', 0, "");
    create_long_file("l8192_nonl", 8192, 'a', 0, "");

    // Tai su dung buffer: dong dai roi dong ngan; EOF sau dong dai.
    create_long_file("long_short1", 1024, 'a', 1, "ok\nlast\n");
    create_long_file("long_short2", 1024, 'a', 1, "NO\nlast\n");
    create_long_file("long_eof", 1024, 'a', 1, "");
    create_long_file("big_short", 8192, 'a', 1, "ok\n");

    // Dong rong khac voi file rong, ke ca o giua va cuoi file.
    create_file("blank", "\n");
    create_file("blanks", "\n\n");
    create_file("blank_mid1", "apple\n\nbanana\n");
    create_file("blank_mid2", "apple\nX\nbanana\n");
    create_file("blank_end1", "apple\n");
    create_file("blank_end2", "apple\n\n");

    exit(0);
}