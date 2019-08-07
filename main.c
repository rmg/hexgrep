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

#if __linux__
// allow readahead(2)
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#if DO_PAGE_ALIGNED
#include <stdlib.h>
#endif
#include <sys/stat.h>
#include <sys/mman.h>

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

    // Can't possibly match, we don't have enough room left. Since we know we'll
    // scan these on the next pass it is better to return early instead of
    // scanning them twice.
    if (unlikely(buf + 40 >= end)) {
        return buf;
    }

    // We know offset 0 is a hex because that's why we're here.
    // We know offset 40 needs to be a non-hex otherwise we're in a 41+ run.
    // We know 1-39 all need to be hex characters.
    // Rather than checking them in linear order, we use statistics to determine
    // the optimal order to check for an early exit.
    // TODO: accept this ordering as input
    // TOOD: extra credit, generate counts from first block
    const int checks[39] = {
        5,
        32,
        31,
        6,
        1,
        30,
        7,
        2,
        33,
        3,
        29,
        38,
        20,
        26,
        23,
        27,
        21,
        22,
        10,
        24,
        28,
        25,
        11,
        12,
        13,
        4,
        8,
        14,
        34,
        15,
        35,
        36,
        16,
        18,
        37,
        39,
        17,
        9,
        19,
    };
    for (int i = 0; i < sizeof(checks)/sizeof(checks[0]); i++) {
        if (!is_lower_hex(buf+checks[i])) {
            INST(runlens[runlen(buf, end)]++);
            return scan_skip(buf+checks[i], end);
        }
    }

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
        } else {
            if (count == 40) {
                print_hit(buf-count);
            }
            count = 0;
        }
        buf++;
    }
    if (count == 40) {
        print_hit(buf-count);
        count = 0;
    }
    return count;
}

#if USE_MMAP
static int mmap_scan(int fd) {
    struct stat st;
    int32_t remainder = 0;

    if (0 > fstat(fd, &st)) {
        perror("fstat failed");
        return 1;
    }
// MAP_POPULATE is only available on Linux, but it is also practically required
// on Linux to make mmap() even half as fast as read() for our usage.
#if ! __linux__
#define MAP_POPULATE 0
#endif
    const unsigned char * buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE, fd, 0);
    if (buf == MAP_FAILED) {
        perror("mmap failed");
        return 2;
    }
    // madvise helps when the file isn't already cached, but that's about it
    if (0 > madvise((void*)buf, st.st_size, MADV_SEQUENTIAL|MADV_WILLNEED)) {
        perror("madvise failed");
        return 3;
    }
    remainder = scan_slice_fast(buf, buf+st.st_size);
    if (remainder >= 40) {
        scan_all_slow((buf+ st.st_size) - remainder, buf+st.st_size);
    }
    if (munmap((void*)buf, st.st_size) != 0) {
        return 4;
    }
    close(fd);
    return 0;
}
#endif

static void fd_readahead(int fd, off_t offset, size_t len) {
#ifndef DO_READAHEAD
    return;
#endif
#if __linux__
    if (0 > readahead(fd, offset, len)) {
        perror("readahead");
    }
#elif __APPLE__
    struct radvisory radvise = { .ra_offset = offset, .ra_count = len };
    if (0 > fcntl(fd, F_RDADVISE, &radvise)) {
        perror("inner radvise");
    }
#endif
    return;
}

static void fd_advise(int fd) {
#ifndef DO_ADVISE
    return;
#endif
#if __linux__
    // same advise as would give mmap
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED);
#elif __APPLE__
    // enable readahead on this file
    fcntl(fd, F_RDAHEAD, 1);
#endif
}

int main(int argc, const char *argv[]) {
#if DO_PAGE_ALIGNED
    int page_size = getpagesize();
    const size_t MAX_BUF = page_size * 64;
    unsigned char *buf;
    if (0 > posix_memalign((void**)&buf, page_size, MAX_BUF)) {
        perror("memalign");
        return 20;
    }
#else
    // 256k sounds nice.
    // 64*4k pages or 128*2k pages or 512*512b fs blocks
    // small enough for stack allocation
    // small enough to fit in cache on modern CPU
    const size_t MAX_BUF = 64*4096;
    unsigned char buf[MAX_BUF];
#endif
    int_fast32_t remainder = 0;
    size_t total_read = 0;
    ssize_t nread = 0;
    INST(int scans = 0);
    int_fast32_t max_scan = 0;
    int fd = 0;
    if (argc > 1) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror("open failed");
            return fd;
        }
#if USE_MMAP
        return mmap_scan(fd);
#endif
    }

    // tell the OS to expect sequential reads
    fd_advise(fd);

    assert(fd >= 0);
    while ((nread = read(fd, buf+remainder, MAX_BUF-remainder)) > 0) {
        total_read += nread;
        // tell kernel to queue up the next read while we process the current one
        if (nread == MAX_BUF-remainder) {
            fd_readahead(fd, total_read, MAX_BUF);
        }
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
        if (i <=40 || runlens[i])
            dprintf(2, "%5d  %10d%s\n", i, runlens[i], i==40 ? " *" : "");
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
