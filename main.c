#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#ifndef DO_INST
    #define DO_INST 0
#endif

#define USE_HEX_TABLE 1

#if DO_INST
    #define static
    #define INST(x) x
#else
    #define INST(x)
#endif

#if DO_INST
#define arr_len(x) (sizeof(x)/sizeof((x)[0]))
    static int_fast32_t cmp = 0;
    static int_fast32_t hits = 0;
    static int_fast32_t runlens[4096] = {0};
    static int_fast32_t skips[128] = {0};
    static int_fast32_t remainders[64] = {0};
#endif

#ifndef NDEBUG
#define assert_not_hex(X) assert(!lhex[*(X)]);
#define assert_hex(X) assert(lhex[*(X)]);
#define assert_hit(X) __assert_hit((X));
#else
#define assert_not_hex(X)
#define assert_hex(X)
#define assert_hit(X)
#endif

# define likely(x) __builtin_expect((x), 1)
# define unlikely(x) __builtin_expect((x), 0)

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
static bool is_lower_hex(const unsigned char * b) {
    INST(cmp++);
#if USE_HEX_TABLE
    return lhex[*b];
#else
    return (*b >= '0' && *b <= '9') || (*b >= 'a' && *b <= 'f');
#endif
}

void __assert_hit(const unsigned char *buf) {
    for (int i = 0; i < 40; i++) {
        assert_hex(buf+i);
    }
    assert_not_hex(buf+40);
}

// We have a confirmed hit, we just need to print it to stdout, terminating with
// a newline. We don't need to worry about length checking because valid memory
// is implied.
static void print_hit(const unsigned char *buf) {
    INST(hits++);
    assert_hit(buf);
    printf("%.40s\n", buf);
}

// At the start of this function, buf is pointing at a non-hex character and the
// goal is to find the next hex character.
static const unsigned char * scan_skip(const unsigned char *buf, const unsigned char *end) {
    assert_not_hex(buf);
    int_fast32_t skip = 40;
#ifndef NDEBUG
    const unsigned char * io = buf;
#endif
    do {
        while (buf + skip < end && !is_lower_hex(buf+skip)) {
            buf += skip;
        }
        skip /= 2;
    } while (skip > 1 && buf + skip < end);
    assert(io <= buf);
    assert(buf < end);
    return buf+1;
}

// We've jumped to a hex character via a large jump and we need to backtrack to
// find the start of the run the current character is pointing at.
static const unsigned char * find_start(const unsigned char *buf, const unsigned char *end) {
    assert_hex(buf);
    while (buf < end) {
        if (is_lower_hex(end-1)) {
            end--;
        } else {
            break;
        }
    }
    assert(end >= buf);
    assert_hex(end);
    return end;
}

#if DO_INST
static const int runlen(const unsigned char *start, const unsigned char *end) {
    int len = 0;
    // use lhex lookup directly here so we don't double count
    while (len < arr_len(runlens) && start < end && lhex[*start]) {
        len++;
        start++;
    }
    return len;
}
#endif

static const unsigned char * scan_hit_short(const unsigned char *buf, const unsigned char * end);

// We're currently at a hex character that is part of a 41+ character run. We
// want to get to the end of it and switch to the regular non-hex skipping code
// path as efficiently as possible.
static const unsigned char * scan_hit_long(const unsigned char *buf, const unsigned char *end) {
    assert_hex(buf);
    assert_hex(buf-40);
    assert_not_hex(buf-41);

    INST(runlens[runlen(buf-40, end)]++);
    // special case.. if we don't have enough room left in the buffer, just return
    if (unlikely(buf+30 >= end)) {
        return buf;
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

    assert(buf +30 < end);

    if (!is_lower_hex(buf+30)) {
        return scan_skip(buf+30, end);
    }

    // if we saw a hex at i+30 it is most likely part of the next run and we
    // need to find where it starts.

    const unsigned char * start = find_start(buf, buf+30);
    if (start == buf) {
        // we have found a run of 70+ hex characters.. these are rare.
        // It is more efficient overall to resort to a byte-by-byte scan to find the end of it.
        buf += 30;
        while (buf < end && is_lower_hex(buf)) {
            buf++;
        }
        return buf;
    }
    assert_hex(start);
    assert_not_hex(start-1);
    return scan_hit_short(start, end);
}

// We are at the first hex character. The goal is to determine as efficiently as
// possible if this is a 40 hex character run terminated by a non-hex, something
// shorter, or something longer.
static const unsigned char * scan_hit_short(const unsigned char *buf, const unsigned char * end) {
    assert_hex(buf);

#define NEED_HEX(N) if (!is_lower_hex(buf+N)) { INST(runlens[runlen(buf, end)]++); return scan_skip(buf+N, end); }

    // Can't possibly match, we don't have enough room left. Since we know we'll
    // scan these on the next pass it is better to return early instead of
    // scanning them twice.
    if (unlikely(buf + 40 >= end)) {
        return buf;
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

    if (unlikely(!is_lower_hex(buf+40))) {
        print_hit(buf);
        return scan_skip(buf+40, end);
    }
    return scan_hit_long(buf+40, end);
}

// Start point and main loop of optimized buffer scan. The range must be long
// enough to accommodate aggressive skips.
static int_fast32_t scan_slice_fast(const unsigned char *buf, const unsigned char * end) {
#ifndef NDEBUG
    const unsigned char * prev;
#endif

    do {
        assert(prev = buf);
        if (is_lower_hex(buf)) {
            buf = scan_hit_short(buf, end);
            assert(buf > prev);
            assert(buf <= end);
        } else {
            buf = scan_skip(buf, end);
            assert(buf > prev);
            assert(buf <= end);
        }
    } while (buf < end - 41);
    assert(buf <= end);
    return end-buf;
}

// Start point and main loop for unoptimized buffer scan. The algorithm here
// performs no aggressive skipping because it is only intended to be used when
// the buffer is short enough that we can't do any fancy skip tricks.
static int_fast32_t scan_all_slow(const unsigned char *buf, const unsigned char * end) {
    int_fast32_t count = 0;
    while (buf < end) {
        if (is_lower_hex(buf)) {
            count++;
            buf++;
            continue;
        }
        if (count == 40) {
            print_hit(buf-count);
        }
        count = 0;
    }
    if (count == 40) {
        print_hit(buf-count);
        count = 0;
    }
    return count;
}

static const size_t MAX_BUF = 64*1024;

int main(int argc, const char *argv[]) {
    unsigned char buf[MAX_BUF];
    int_fast32_t remainder = 0;
    size_t total_read = 0;
    ssize_t nread = 0;
    int_fast32_t scans = 0;
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
        remainder = scan_slice_fast(buf, buf+max_scan);
        if (likely(remainder)) {
            memcpy(buf, buf+max_scan-remainder, remainder);
        }
        scans++;
        INST(remainders[remainder]++);
    }
    if (remainder >= 40) {
        scan_all_slow(buf+max_scan-remainder, buf+max_scan);
    }
    dprintf(2, "Bytes read:  %10zu (blocks: %d)\n", total_read, scans);

#if DO_INST
    // 115209474/1033491456 are the results from a sample file
    dprintf(2, "Comparisons: %10d (%d)\n", cmp, cmp-115209474);
    dprintf(2, "Remainders copied to next run:\n");
    for (int i = 0; i < arr_len(remainders); i++)
        if (remainders[i])
            dprintf(2, "Remainder: %2d: %10d\n", i, remainders[i]);
    dprintf(2, "Hex string counts by length:\n");
    for (int i = 0; i < arr_len(runlens); i++)
        if (runlens[i])
            dprintf(2, " [%4d]  %10d%s\n", i, runlens[i], i==40 ? " *" : "");
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
