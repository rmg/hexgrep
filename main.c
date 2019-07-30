#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void hit(char *buf) {
    printf("%.40s\n", buf);
}

off_t scan_slice(char *buf, size_t MAX) {
    int count = 0;
    for (int i = 0; i < MAX; i++) {
        const char b = buf[i];
        if ((b >= '0' && b <= '9') || (b >= 'a' && b <= 'f')) {
            count++;
            continue;
        }
        if (count == 40) {
            hit(buf + i - count);
        }
        count = 0;
    }
    if (count == 40) {
        hit(buf+MAX-40);
    }
    return count > 40 ? 41 : count;
}

const size_t MAX_BUF = 32*1024;

int main(int argc, char *argv[]) {
    char buf[MAX_BUF];
    off_t off = 0;
    size_t total_read = 0;
    ssize_t nread = 0;
    while ((nread = read(0, buf+off, MAX_BUF-off)) > 0) {
        total_read += nread;
        off_t max_scan = nread + off;
        off = scan_slice(buf, max_scan);
        if (off) {
            memmove(buf, buf+max_scan-off, off);
        }
    }
    dprintf(2, "Total bytes read: %zu\n", total_read);
    return 0;
}
