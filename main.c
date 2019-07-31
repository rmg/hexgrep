#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define INST(x) x
// #define INST(x)

static const bool lhex[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
INST(static int_fast32_t cmp = 0);
INST(static uint_fast32_t hits = 0);
INST(static uint_fast32_t short_runs = 0);
INST(static uint_fast32_t long_runs = 0);
INST(static uint_fast32_t short_run[41] = {0});

static void print_hit(char *buf) {
    INST(hits++);
    printf("%.40s\n", buf);
}

// #define assert_not_hex(x)
static void assert_not_hex(unsigned char b) {
    assert(!lhex[b]);
}

// #define assert_hex(x)
static void assert_hex(unsigned char b) {
    assert(lhex[b]);
}

// nobody write a git sha using uppercase hex, it's always lowercase
static bool is_lower_hex(unsigned char b) {
    INST(cmp++);
    return lhex[b];
    // return (b >= '0' && b <= '9') || (b >= 'a' && b <= 'f');
}

static int_fast32_t scan_skip(char *buf, size_t MAX, int_fast32_t i) {
    assert_not_hex(buf[i]);

    int_fast32_t skip = 40, io = i;
    do {
        while (i + skip < MAX && !is_lower_hex(buf[i+skip])) {
            i += skip;
        }
        skip /= 2;
    } while (skip > 1 && i + skip < MAX);
    assert(io <= i);
    // if (io >= i) {
    //     dprintf(2, "bad skip: %d >= %d, MAX: %zu, skip: %d, char: ['%c', *'%c', '%c']\n", io, i, MAX, skip, buf[i-1], buf[i], buf[i+1]);
    // }
    assert(i < MAX);
    // if (i+1 == MAX)
    return i+1;
}

// static int_fast32_t scan_hit(char *buf, size_t MAX, int_fast32_t i) {
//     int_fast32_t right = i+40;
//     int_fast32_t count = 0;


//     bool maybe_terminated = !is_lower_hex(buf[i+41]);
//     if (maybe_terminated) {
//         while (right > i) {
//             if (is_lower_hex(buf[right])) {
//                 count++;
//                 right--;
//                 continue;
//             }
//             return scan_skip(buf, MAX, right);
//         }
//         if (count == 40 && right == i) {
//             print_hit(i);
//         }
//     }

// }

int_fast32_t scan_hit_long(char *buf, size_t MAX, int_fast32_t i) {
    assert_hex(buf[i]);
    // just blindly eat all the hexes..
    while (i < MAX) {
        if (is_lower_hex(buf[i])) {
            i++;
            continue;
        }
        break;
    }
    return i;
}


/*
Long runs:       276738
Exact runs:      184227
Short runs:     3763173
Short runs:      611334 (2)
Short runs:      181262 (32)
Short runs:      137846 (3)
Short runs:      133480 (5)
Short runs:      104525 (31)
Short runs:      101972 (4)
*/

// static
int_fast32_t scan_hit_short(char *buf, size_t MAX, int_fast32_t i) {
    assert_hex(buf[i]);

#define NEED_HEX(N) if (!is_lower_hex(buf[i+N])) return scan_skip(buf, MAX, i+N)

    // unrolled checking the next 41 bytes (must be terminated)
    // ordered for optimal early return based on most frequent lengths
    // of short runs
    NEED_HEX(32);
    NEED_HEX(2);
    NEED_HEX(1);
    NEED_HEX(3);
    NEED_HEX(4);
    NEED_HEX(5);
    NEED_HEX(6);
    NEED_HEX(7);
    NEED_HEX(8);
    NEED_HEX(9);
    NEED_HEX(10);
    NEED_HEX(11);
    NEED_HEX(12);
    NEED_HEX(13);
    NEED_HEX(14);
    NEED_HEX(15);
    NEED_HEX(16);
    NEED_HEX(17);
    NEED_HEX(18);
    NEED_HEX(19);
    NEED_HEX(20);
    NEED_HEX(21);
    NEED_HEX(22);
    NEED_HEX(23);
    NEED_HEX(24);
    NEED_HEX(25);
    NEED_HEX(26);
    NEED_HEX(27);
    NEED_HEX(28);
    NEED_HEX(29);
    NEED_HEX(30);
    NEED_HEX(31);
    NEED_HEX(33);
    NEED_HEX(34);
    NEED_HEX(35);
    NEED_HEX(36);
    NEED_HEX(37);
    NEED_HEX(38);
    NEED_HEX(39);

    if (!is_lower_hex(buf[i+40])) {
        print_hit(buf+i);
        return scan_skip(buf, MAX, i+40);
    }
    return scan_hit_long(buf, MAX, i+40);
}

#if 1
// i is a hit
//static
int_fast32_t scan_hit(char *buf, size_t MAX, int_fast32_t i) {
    // if (i + 41 > MAX) {
    //     // we don't have enough room to get a hit
    // }
    assert_hex(buf[i]);
    // good skip, decent odds
    if (!is_lower_hex(buf[i+33])) {
        return scan_skip(buf, MAX, i+33);
    }
    // short skip, excellent odds
    if (!is_lower_hex(buf[i+3])) {
        return scan_skip(buf, MAX, i+3);
    }
    // long skip, even odds
    if (!is_lower_hex(buf[i+39])) {
        return scan_skip(buf, MAX, i+39);
    }
    // mm
    // if (is_lower_hex(buf[i + 41])) {
    //     return i+41;
    //     // dprintf(2, "too long: %41s\n", buf+i);
    //     // find next possible starting point
    //     // return scan_skip(buf, MAX, i+40);
    // }
    int_fast32_t count = 0;
    while (i < MAX) {
        if (is_lower_hex(buf[i])) {
            count++;
            i++;
            continue;
        }
        break;
    }
    if (count == 40) {
        print_hit(buf+i-40);
    }
    return i;

    // // if i is a hit and i+41 is a hit,
    // // then i..40 is too long or too short

    // int_fast32_t count = 1;
    // while (i+count < MAX && is_lower_hex(buf[i+count])) {
    //     count++;
    // }
    // if (count == 40) {
    //     print_hit(buf);
    //     // return i;
    // }
    // return i+count+1;

    //  skip; skip--) {
    // for (int_fast32_t skip = 40; skip; skip--) {
    //     if (!is_lower_hex(buf[i+skip])) {
    //         return scan_skip(buf, MAX, i+skip);
    //     }
    // }
    // print_hit(buf);
    // return i+41;
}


static int_fast32_t scan_slice_fast(char *buf, size_t MAX, int_fast32_t count) {
    int_fast32_t i = 0, io = 0; //, r = 0, l = 0;

    do {
        io = i;
        if (is_lower_hex(buf[i])) {
            i = scan_hit_short(buf, MAX, i);
            assert(i > io);
        } else {
            i = scan_skip(buf, MAX, i);
            assert(i > io);
        }

        // if (count == 0) {
        //     while (i+40 < MAX && !is_lower_hex(buf[i+40])) {
        //         i += 40;
        //     }
        //     while (i+20 < MAX && !is_lower_hex(buf[i+20])) {
        //         i += 20;
        //     }
        // }
        // hit a non-hex
        // if we just ended a 40-hex run, print it
        // if (count == 40) {
        //     print_hit(buf + i - count);
        // }
        // count = 0;
        // i = scan_skip(buf, MAX, i);
    } while (i < MAX - 41);
    if (count > 40) {
        count = 41;
    }
    i -= count;
    return MAX-i;
    // return MAX-i-count;
    // return count;
}

#else
static int_fast32_t scan_slice_fast(char *buf, size_t MAX, int_fast32_t count) {
    int_fast32_t i = 0; //, r = 0, l = 0;

    for (; i +41 < MAX; ) {
        // eat all the hexes!
        if (is_lower_hex(buf[i])) {
            assert_hex(buf[i]);
            count++;
            i++;
            // if (count < 40 && !is_lower_hex(buf[i+40-count])) {
            //     i = scan_skip(buf, MAX, i+40-count);
            //     count = 0;
            // }
            continue;
        }
        // if (count == 0) {
        //     while (i+40 < MAX && !is_lower_hex(buf[i+40])) {
        //         i += 40;
        //     }
        //     while (i+20 < MAX && !is_lower_hex(buf[i+20])) {
        //         i += 20;
        //     }
        // }
        // hit a non-hex
        // if we just ended a 40-hex run, print it
        if (count < 40) {
            INST(short_runs++);
            INST(short_run[count]++);
        } if (count == 40) {
            print_hit(buf + i - count);
        } else if (count> 40) {
            INST(long_runs++);
        }
        count = 0;
        i = scan_skip(buf, MAX, i);
    }
    if (count > 40) {
        count = 41;
    }
    assert(i <= MAX);
    i -= count;
    assert(i <= MAX);
    return MAX-i;
    // return MAX-i-count;
    // return count;
}
#endif



static int_fast32_t scan_all_slow(char *buf, size_t MAX) {
    int_fast32_t count = 0;
    for (int_fast32_t i = 0; i < MAX; i++) {
        if (is_lower_hex(buf[i])) {
            count++;
            continue;
        }
        if (count == 40) {
            print_hit(buf+i-count);
        }
        count = 0;
    }
    if (count == 40) {
        print_hit(buf+MAX-count);
        count = 0;
    }
    return count;
}

const size_t MAX_BUF = 64*1024;

int main(int argc, char *argv[]) {
    char buf[MAX_BUF];
    int_fast32_t remainder = 0;
    int_fast32_t count = 0;
    size_t total_read = 0;
    ssize_t nread = 0;
    int_fast32_t max_scan = 0;
    int fd = 0;
    // bool dangling_hit = false;
    if (argc > 1) {
        fd = open(argv[1], O_RDONLY);
    }
    assert(fd >= 0);
    while ((nread = read(fd, buf+remainder, MAX_BUF-remainder)) > 0) {
        total_read += nread;
        max_scan = nread + remainder;
        remainder = scan_slice_fast(buf, max_scan, count);
        // dprintf(2, "remainder:  %10d (%d)\n", remainder, hits);
        // if (hits == 104418) {
        //     dprintf(2, "interesting index: %lu\n", total_read - remainder);
        // }
        if (remainder) {
            memmove(buf, buf+max_scan-remainder, remainder);
        }
        // dangling_hit = (remainder > 0 && remainder <= 40);
        // if (dangling_hit) {
        //     memmove(buf, buf+max_scan-remainder, remainder);
        //     count = 0;
        // } else {
        //     count = 41;
        // }
    }
    if (remainder >= 40) {
        scan_all_slow(buf+max_scan-remainder, remainder);
    }
    // if (dangling_hit) {
    //     print_hit(buf+max_scan-40);
    // }
    dprintf(2, "Bytes read:  %10zu\n", total_read);
    INST(dprintf(2, "Comparisons: %10d (%d)\n", cmp, cmp-134030479));
    INST(dprintf(2, "Short runs:  %10d\n", short_runs));
    for (int i = 1; i < 40; i++) {
        INST(dprintf(2, "Short runs:  %10d (%d)\n", short_run[i], i));
    }
    INST(dprintf(2, "Long runs:   %10d\n", long_runs));
    INST(dprintf(2, "Exact runs:  %10d\n", hits));
    return nread;
}



/*
Bytes read:  1033491456
Comparisons:  134030479 (0)
Long runs:       276738
Exact runs:      184227
Short runs:     3763173
Short runs:      611334 (2)
Short runs:      181262 (32)
Short runs:      137846 (3)
Short runs:      133480 (5)
Short runs:      104525 (31)
Short runs:      101972 (4)
Short runs:       45380 (6)
Short runs:       36667 (7)
Short runs:       25538 (30)
Short runs:       11223 (8)
Short runs:       10240 (39)
Short runs:        7884 (33)
Short runs:        5501 (38)
Short runs:        4668 (29)
Short runs:        3288 (10)
Short runs:        2720 (26)
Short runs:        2158 (27)
Short runs:        2077 (21)
Short runs:        2061 (23)
Short runs:        2007 (20)
Short runs:        1927 (22)
Short runs:        1877 (25)
Short runs:        1788 (24)
Short runs:        1767 (13)
Short runs:        1753 (12)
Short runs:        1695 (9)
Short runs:        1670 (11)
Short runs:        1443 (28)
Short runs:        1176 (16)
Short runs:        1057 (1)
Short runs:         912 (14)
Short runs:         792 (18)
Short runs:         738 (19)
Short runs:         694 (15)
Short runs:         588 (36)
Short runs:         549 (34)
Short runs:         489 (35)
Short runs:         444 (17)
Short runs:         413 (37)
 */
