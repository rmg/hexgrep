#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#define DO_INST 0
#define USE_HEX_TABLE 1

#if DO_INST
    #define STATIC
    #define INST(x) x
    #define DO_DUMP
#else
    #define INST(x)
    #define STATIC static
#endif

#if DO_INST
    static int_fast32_t cmp = 0;
    static int_fast32_t hits = 0;
    static int_fast32_t short_runs = 0;
    static int_fast32_t long_runs = 0;
    static int_fast32_t short_run[41] = {0};
    static int_fast32_t long_run[41] = {0};
    static int_fast32_t skips[128] = {0};
#endif

#ifndef NDEBUG
#define assert_not_hex(X) assert(!lhex[(unsigned char)X]);
#define assert_hex(X) assert(lhex[(unsigned char)X]);
#define assert_hit(X) __assert_hit((X));
#else
#define assert_not_hex(X)
#define assert_hex(X)
#define assert_hit(X)
#endif

#ifdef DO_DUMP
STATIC void dump_buf(char *buf, size_t MAX, int_fast32_t i) {
    dprintf(2, "buf [%d/%zu] [", i, MAX);
    for (int_fast32_t o = -20; o < 20; o++) {
        if (i + o > 0 && i + o < MAX) {
            if (o) {
                dprintf(2, " %x ", buf[i+o]);
            } else {
                dprintf(2, " <%x> ", buf[i+o]);
            }
        }
    }
    dprintf(2, "]\n");
}
#endif

#if USE_HEX_TABLE
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
#endif

// nobody write a git sha using uppercase hex, it's always lowercase
STATIC bool is_lower_hex(unsigned char b) {
    INST(cmp++);
#if USE_HEX_TABLE
    return lhex[b];
#else
    return (b >= '0' && b <= '9') || (b >= 'a' && b <= 'f');
#endif
}

void __assert_hit(const char *buf) {
    for (int i = 0; i < 40; i++) {
        assert_hex(buf[i]);
    }
    assert_not_hex(buf[40]);
}

STATIC void print_hit(const char *buf) {
    INST(hits++);
    assert_hit(buf);
    printf("%.40s\n", buf);
}

STATIC int_fast32_t scan_skip(char *buf, size_t MAX, int_fast32_t i) {
    assert_not_hex(buf[i]);

    int_fast32_t skip = 40;
#ifndef NDEBUG
    int_fast32_t io = i;
#endif
    do {
        while (i + skip < MAX && !is_lower_hex(buf[i+skip])) {
            i += skip;
        }
        skip /= 2;
    } while (skip > 1 && i + skip < MAX);
    assert(io <= i);
    if (i >= MAX) {
        INST(dump_buf(buf, MAX, i));
    }
    assert(i < MAX);
    return i+1;
}

STATIC int_fast32_t find_start(char *buf, size_t MAX, int_fast32_t MIN, int_fast32_t i) {
    assert_hex(buf[i]);
    while (i > MIN) {
        if (is_lower_hex(buf[i-1])) {
            i--;
        } else {
            break;
        }
    }
    assert(i >= MIN);
    assert_hex(buf[i]);
    return i;
}

STATIC int_fast32_t scan_hit_short(char *buf, size_t MAX, int_fast32_t i);


STATIC int_fast32_t scan_hit_long(char *buf, size_t MAX, int_fast32_t i) {
    assert_hex(buf[i]);

    // special case.. if we don't have enough room left in the buffer, just return
    if (i+30 >= MAX) {
        return i;
    }

    // given we are at 41 in length already, then we know the closest the next
    // valid run of 40 starts at would be i+2 to i+42
    // interesting to know, but not particularly useful.

    // what is useful is knowing that the majority of hex strings over 40
    // characters long are sha256 digests, being 64 digits long. We also know
    // that there's also a significant number of runs longer than that, but that
    // they drop off dramatically.

    // a hex string of 50 characters is extremely unlikely if we hit a non-hex
    // at 50 we know that the current run ends before then and that any runs
    // between here and there are too short to care about.

    assert(i+30 < MAX);

    if (!is_lower_hex(buf[i+30])) {
        return scan_skip(buf, MAX, i+30);
    }

    // if we saw a hex at i+30 it is most likely part of the next run and we
    // need to find where it starts.

    int_fast32_t start = find_start(buf, MAX, i, i+30);
    if (start == i) {
        // wow, it really was part of the current run!
        return scan_hit_long(buf, MAX, i+30);
    }
    assert_hex(buf[start]);
    assert_not_hex(buf[start-1]);
    return scan_hit_short(buf, MAX, start);
}

STATIC int_fast32_t scan_hit_short(char *buf, size_t MAX, int_fast32_t i) {
    assert_hex(buf[i]);

#define NEED_HEX(N) if (!is_lower_hex(buf[i+N])) { INST(short_runs++); INST(short_run[N]++); return scan_skip(buf, MAX, i+N); }

    // Can't possibly match, we don't have enough room left. Since we know we'll
    // scan these on the next pass it is better to return early instead of
    // scanning them twice.
    if (i+40 >= MAX) {
        return i;
    }

    // Unrolled checking the next 40 bytes (must be terminated). We know the
    // most frequent lengths of short hex strings, so we for those first by
    // looking at N+1
    NEED_HEX(3);
    NEED_HEX(33);
    NEED_HEX(4);
    NEED_HEX(6);
    NEED_HEX(32);

    // TODO: the rest of these could be tuned but it likely won't make too much
    // difference since the first 2 cover the majority of cases anyway.
    NEED_HEX(1);
    NEED_HEX(2);
    // NEED_HEX(3);
    // NEED_HEX(4);
    NEED_HEX(5);
    // NEED_HEX(6);
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
    // NEED_HEX(32);
    // NEED_HEX(33);
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

STATIC int_fast32_t scan_slice_fast(char *buf, size_t MAX, int_fast32_t count) {
    int_fast32_t i = 0, io = 0;

    do {
        io = i;
        if (is_lower_hex(buf[i])) {
            i = scan_hit_short(buf, MAX, i);
            assert(i > io);
        } else {
            i = scan_skip(buf, MAX, i);
            assert(i > io);
        }
    } while (i < MAX - 41);
    if (count > 40) {
        count = 41;
    }
    i -= count;
    return MAX-i;
}

STATIC int_fast32_t scan_all_slow(char *buf, size_t MAX) {
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

STATIC const size_t MAX_BUF = 64*1024;

int main(int argc, char *argv[]) {
    char buf[MAX_BUF];
    int_fast32_t remainder = 0;
    int_fast32_t count = 0;
    size_t total_read = 0;
    ssize_t nread = 0;
    int_fast32_t max_scan = 0;
    int fd = 0;
    if (argc > 1) {
        fd = open(argv[1], O_RDONLY);
    }
    assert(fd >= 0);
    while ((nread = read(fd, buf+remainder, MAX_BUF-remainder)) > 0) {
        total_read += nread;
        max_scan = nread + remainder;
        if (max_scan < 41) {
            remainder = max_scan;
            continue;
        }
        remainder = scan_slice_fast(buf, max_scan, count);
        // INST(dprintf(2, "remainder:  %10d (%d)\n", remainder, hits));
        if (remainder) {
            memmove(buf, buf+max_scan-remainder, remainder);
        }
    }
    if (remainder >= 40) {
        scan_all_slow(buf+max_scan-remainder, remainder);
    }
    dprintf(2, "Bytes read:  %10zu\n", total_read);

#if DO_INST
    // 115209474/1033491456 are the results from a sample file
    dprintf(2, "Comparisons: %10d (%d)\n", cmp, cmp-115209474);
    dprintf(2, "Short runs:  %10d\n", short_runs);
    for (int i = 1; i < 40; i++) {
        dprintf(2, "Short runs:  %10d (%d)\n", short_run[i], i);
    }
    dprintf(2, "Exact runs:  %10d\n", hits);
    dprintf(2, "Long runs:   %10d\n", long_runs);
    for (int i = 1; i < 40; i++) {
        dprintf(2, "Long  runs:  %10d (+%d)\n", long_run[i], i);
    }
#endif

    return nread;
}



/*
Bytes read:  1033491456
Comparisons:  134030479 (0)

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

Exact runs:      184227
Long runs:       276738

Long  runs:        5562 (+1)
Long  runs:        1007 (+2)
Long  runs:         863 (+3)
Long  runs:         901 (+4)
Long  runs:         951 (+5)
Long  runs:         882 (+6)
Long  runs:         997 (+7)
Long  runs:        1052 (+8)
Long  runs:         824 (+9)
Long  runs:         864 (+10)
Long  runs:         885 (+11)
Long  runs:         903 (+12)
Long  runs:        1014 (+13)
Long  runs:         808 (+14)
Long  runs:         918 (+15)
Long  runs:         914 (+16)
Long  runs:         779 (+17)
Long  runs:         889 (+18)
Long  runs:         649 (+19)
Long  runs:         547 (+20)
Long  runs:        2207 (+21)
Long  runs:       16573 (+22)
Long  runs:        4678 (+23)
Long  runs:      146793 (+24)
Long  runs:       78801 (+25)
Long  runs:        5180 (+26)
Long  runs:          54 (+27)
Long  runs:           3 (+28)
Long  runs:           0 (+29)
Long  runs:           0 (+30)
Long  runs:           0 (+31)
Long  runs:           0 (+32)
Long  runs:           0 (+33)
Long  runs:           0 (+34)
Long  runs:           0 (+35)
Long  runs:           6 (+36)
Long  runs:          15 (+37)
Long  runs:           6 (+38)
Long  runs:         213 (+39)

 */
