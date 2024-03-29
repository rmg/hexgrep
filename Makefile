# Copyright 2019 Ryan Graham
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

SAMPLE ?= raw.tar
CFLAGS :=  -Werror -Wall
FAST_CFLAGS := $(CFLAGS) -O3 -DNDEBUG
DEBUG_CFLAGS := $(CFLAGS) -g
SIMD_CFLAGS := $(CFLAGS) -DUSE_SIMD=1
TIME_CMD = command time -p -a -o $@.times

hexgrep scan-c-fast: main.c
	$(CC) $(FAST_CFLAGS) -o $@ $<
	strip $@

hexgrep-static: main.c
	$(CC) $(FAST_CFLAGS) -static -o $@ $<
	strip $@

all: c.txt go.txt rs.txt grep.txt ripgrep.txt js.txt

scan-c: main.c
	$(CC) $(FAST_CFLAGS) -o $@ $<

scan-c-fast-simd: main.c
	$(CC) $(SIMD_CFLAGS) -O3 -DNDEBUG -o $@ $<

scan-c-counters: main.c
	$(CC) $(SIMD_CFLAGS) -O3 -DDO_INST=1 -o $@ $<

scan-c-debug: main.c
	$(CC) $(CFLAGS) -g -o $@ $<

main.s: main.c
	$(CC) $(CFLAGS) -O3 -DNDEBUG -S -mllvm --x86-asm-syntax=intel -o $@ $<

time-simd: scan-c-fast-simd
	$(TIME_CMD) ./scan-c-fast-simd < raw.tar > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

time: scan-c-fast
	$(TIME_CMD) ./scan-c-fast < raw.tar > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

counts: scan-c-counters
	$(TIME_CMD) ./scan-c-counters < raw.tar > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

test: scan-c-debug
	$(TIME_CMD) ./scan-c-debug < raw.tar > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

debug: scan-c-debug
	lldb ./scan-c-debug raw.tar

scan-go: main.go
	go build -o $@ $<

scan-rs: main.rs
	rustc -O -o $@ $<

times.md:
	{ \
		printf '\n|     |  real |  user | system |\n'; \
		printf   '|-----|-------|-------|--------|\n'; \
		for t in *.txt.times; do \
			printf '| %3s | %5s | %5s | %6s |\n' $$(basename -s .txt.times $$t) $$(awk -f times.awk $$t); \
		done; \
	} > $@

%.txt: scan-% $(SAMPLE)
	$(TIME_CMD) ./scan-$* < $(SAMPLE) > $@
	$(TIME_CMD) ./scan-$* < $(SAMPLE) > $@
	$(TIME_CMD) ./scan-$* < $(SAMPLE) > $@

js.txt: main.js $(SAMPLE)
	$(TIME_CMD) node main.js < $(SAMPLE) > $@
	$(TIME_CMD) node main.js < $(SAMPLE) > $@
	$(TIME_CMD) node main.js < $(SAMPLE) > $@

grep.txt: $(SAMPLE)
	$(TIME_CMD) grep -E -a -o '[0-9a-f]{40}' < $(SAMPLE) > $@ || true
	$(TIME_CMD) grep -E -a -o '[0-9a-f]{40}' < $(SAMPLE) > $@ || true
	$(TIME_CMD) grep -E -a -o '[0-9a-f]{40}' < $(SAMPLE) > $@ || true

ripgrep.txt: $(SAMPLE)
	$(TIME_CMD) rg -o -a '[a-f0-9]{40}' < $(SAMPLE) > $@ || true
	$(TIME_CMD) rg -o -a '[a-f0-9]{40}' < $(SAMPLE) > $@ || true
	$(TIME_CMD) rg -o -a '[a-f0-9]{40}' < $(SAMPLE) > $@ || true

docker.txt: $(SAMPLE)
	docker build -t hexgrep:local .
	$(TIME_CMD) docker run --volume $(abspath ${SAMPLE}):/sample --rm -i hexgrep:local sample > $@ || true
	$(TIME_CMD) docker run --volume $(abspath ${SAMPLE}):/sample --rm -i hexgrep:local sample > $@ || true
	$(TIME_CMD) docker run --volume $(abspath ${SAMPLE}):/sample --rm -i hexgrep:local sample > $@ || true

raw.tar:
	docker pull node:latest
	docker save -o $@ node:latest

hexgrep-linux: Makefile main.c
	mkdir -p linux
	cp $^ linux/
	docker run --rm --volume $(abspath linux):/work --workdir /work buildpack-deps:xenial make hexgrep-static
	cp linux/hexgrep-static hexgrep-linux
