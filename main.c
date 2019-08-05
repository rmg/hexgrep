/*
Copyright 2019 Ryan Graham

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
    INST(runlens[40]++);
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
            skip = 40;
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
    // sorted by frequency of lengths seen in primary sample
    // TODO: accept this ordering as input
    // TOOD: extra credit, generate counts from first block
    NEED_HEX(5); // 103098
    NEED_HEX(32); // 70630
    NEED_HEX(31); // 63754
    NEED_HEX(6); // 30347
    NEED_HEX(1); // 29874
    NEED_HEX(30); // 19119
    NEED_HEX(7); // 17701
    NEED_HEX(2); // 13914
    NEED_HEX(33); // 4749
    NEED_HEX(3); // 4510
    NEED_HEX(29); // 3329
    NEED_HEX(38); // 1763
    NEED_HEX(20); // 1532
    NEED_HEX(26); // 1512
    NEED_HEX(23); // 1395
    NEED_HEX(27); // 1384
    NEED_HEX(21); // 1364
    NEED_HEX(22); // 1270
    NEED_HEX(10); // 1239
    NEED_HEX(24); // 1223
    NEED_HEX(28); // 1091
    NEED_HEX(25); // 1048
    NEED_HEX(11); // 1024
    NEED_HEX(12); // 839
    NEED_HEX(13); // 734
    NEED_HEX(4); // 603
    NEED_HEX(8); // 494
    NEED_HEX(14); // 429
    NEED_HEX(34); // 375
    NEED_HEX(15); // 306
    NEED_HEX(35); // 192
    NEED_HEX(36); // 175
    NEED_HEX(16); // 165
    NEED_HEX(18); // 69
    NEED_HEX(37); // 62
    NEED_HEX(39); // 60
    NEED_HEX(17); // 19
    NEED_HEX(9); // 13
    NEED_HEX(19); // 7

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
    INST(size_t total_read = 0);
    ssize_t nread = 0;
    INST(int scans = 0);
    int_fast32_t max_scan = 0;
    int fd = 0;
    if (argc > 1) {
        fd = open(argv[1], O_RDONLY);
    }
    assert(fd >= 0);
    while ((nread = read(fd, buf+remainder, MAX_BUF-remainder)) > 0) {
        INST(total_read += nread);
        max_scan = nread + remainder;
        if (max_scan < 41) {
            remainder = max_scan;
            continue;
        }
        remainder = scan_slice_fast(buf, buf+max_scan);
        if (likely(remainder)) {
            memcpy(buf, buf+max_scan-remainder, remainder);
        }
        INST(scans++);
        INST(remainders[remainder]++);
    }
    if (remainder >= 40) {
        scan_all_slow(buf+max_scan-remainder, buf+max_scan);
    }
    INST(dprintf(2, "Bytes read:  %10zu (blocks: %d)\n", total_read, scans));

#if DO_INST
    // 63907898/1033491456 are the results from a sample file
    dprintf(2, "Comparisons: %10d (%d)\n", cmp, cmp-63273413);
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
Bytes read:  1033491456 (blocks: 15773)
Comparisons:   63273413 (0)
Remainders copied to next run:
Remainder:  0:       1510
Remainder:  1:        354
Remainder:  2:        515
Remainder:  3:        405
Remainder:  4:        384
Remainder:  5:       2336
Remainder:  6:        399
Remainder:  7:        406
Remainder:  8:        481
Remainder:  9:        356
Remainder: 10:       1403
Remainder: 11:        388
Remainder: 12:        641
Remainder: 13:        428
Remainder: 14:        380
Remainder: 15:       3043
Remainder: 16:        462
Remainder: 17:        440
Remainder: 18:        516
Remainder: 19:        432
Remainder: 20:         21
Remainder: 21:         24
Remainder: 22:         26
Remainder: 23:         38
Remainder: 24:         21
Remainder: 25:         30
Remainder: 26:         21
Remainder: 27:         25
Remainder: 28:         26
Remainder: 29:         27
Remainder: 30:         18
Remainder: 31:         24
Remainder: 32:         19
Remainder: 33:         17
Remainder: 34:         27
Remainder: 35:         17
Remainder: 36:         26
Remainder: 37:         22
Remainder: 38:         19
Remainder: 39:         14
Remainder: 40:         13
Remainder: 41:         19
Hex string counts by length:
 [   1]       29924
 [   2]       13413
 [   3]        4437
 [   4]         595
 [   5]      103127
 [   6]       30461
 [   7]       17669
 [   8]         479
 [   9]          14
 [  10]        1236
 [  11]        1022
 [  12]         830
 [  13]         733
 [  14]         418
 [  15]         297
 [  16]         167
 [  17]          19
 [  18]          68
 [  19]           6
 [  20]        1513
 [  21]        1295
 [  22]        1224
 [  23]        1392
 [  24]        1191
 [  25]        1055
 [  26]        1542
 [  27]        1391
 [  28]        1076
 [  29]        3300
 [  30]       19262
 [  31]       63679
 [  32]       70641
 [  33]        4764
 [  34]         384
 [  35]         198
 [  36]         190
 [  37]          62
 [  38]        1773
 [  39]          61
 [  40]      184227 *
 [  41]        5562
 [  42]        1008
 [  43]         868
 [  44]         904
 [  45]         947
 [  46]         880
 [  47]         998
 [  48]        1047
 [  49]         819
 [  50]         861
 [  51]         884
 [  52]         904
 [  53]        1013
 [  54]         812
 [  55]         916
 [  56]         916
 [  57]         783
 [  58]         889
 [  59]         651
 [  60]         548
 [  61]        2205
 [  62]       16575
 [  63]        4677
 [  64]      146794
 [  65]       78805
 [  66]        5175
 [  67]          54
 [  68]           3
 [  76]           6
 [  77]          15
 [  78]           6
 [  80]          24
 [  84]           6
 [  96]          36
 [ 128]          33
 [ 131]           6
 [ 132]          12
 [ 134]           3
 [ 135]           3
 [ 138]           6
 [ 156]           6
 [ 169]           6
 [ 176]           6
 [ 192]           6
 [ 232]           6
 [ 288]           6
 [ 308]          18
 [ 309]          24
 [ 906]           6
 */
