all: c.txt go.txt rs.txt grep.txt ripgrep.txt js.txt

SAMPLE ?= raw.tar
CFLAGS :=  -Werror -Wall -march=native -fvectorize
FAST_CFLAGS := $(CFLAGS) -O3 -DNDEBUG
DEBUG_CFLAGS := $(CFLAGS) -g
SIMD_CFLAGS := $(CFLAGS) -DUSE_SIMD=1

scan-c: main.c
	$(CC) $(CFLAGS) -o $@ $<

scan-c-fast: main.c
	$(CC) $(CFLAGS) -O3 -DNDEBUG -o $@ $<

scan-c-fast-simd: main.c
	$(CC) $(SIMD_CFLAGS) -O3 -DNDEBUG -o $@ $<

scan-c-counters: main.c
	$(CC) $(SIMD_CFLAGS) -O3 -DDO_INST=1 -o $@ $<

scan-c-debug: main.c
	$(CC) $(CFLAGS) -g -o $@ $<

main.s: main.c
	$(CC) $(CFLAGS) -O3 -S -mllvm --x86-asm-syntax=intel -o $@ $<

time-simd: scan-c-fast-simd
	time ./scan-c-fast-simd < raw.tar > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

time: scan-c-fast
	time ./scan-c-fast < raw.tar > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

counts: scan-c-counters
	time ./scan-c-counters < raw.tar > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

test: scan-c-debug
	time ./scan-c-debug < raw.tar > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

debug: scan-c-debug
	lldb ./scan-c-debug raw.tar

scan-go: main.go
	go build -o $@ $<

scan-rs: main.rs
	rustc -O -o $@ $<

%.txt: scan-% $(SAMPLE)
	time ./scan-$* < $(SAMPLE) > $@
	time ./scan-$* < $(SAMPLE) > $@
	time ./scan-$* < $(SAMPLE) > $@

js.txt: main.js $(SAMPLE)
	time node main.js < $(SAMPLE) > $@
	time node main.js < $(SAMPLE) > $@
	time node main.js < $(SAMPLE) > $@

grep.txt: $(SAMPLE)
	time grep -E -a -o '[0-9a-f]{40}' < $(SAMPLE) > $@ || true
	time grep -E -a -o '[0-9a-f]{40}' < $(SAMPLE) > $@ || true
	time grep -E -a -o '[0-9a-f]{40}' < $(SAMPLE) > $@ || true

ripgrep.txt: $(SAMPLE)
	time rg -o -a '[a-f0-9]{40}' < $(SAMPLE) > $@ || true
	time rg -o -a '[a-f0-9]{40}' < $(SAMPLE) > $@ || true
	time rg -o -a '[a-f0-9]{40}' < $(SAMPLE) > $@ || true

raw.tar:
	docker pull node:latst
	docker save -o $@ node:latest
