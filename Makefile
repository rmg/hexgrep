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

hexgrep scan-c-fast: main.c
	$(CC) $(FAST_CFLAGS) -o $@ $<
	strip $@

hexgrep-static: main.c
	$(CC) $(FAST_CFLAGS) -static -o $@ $<
	strip $@

all: c.txt go.txt rs.txt grep.txt ripgrep.txt js.txt

scan-c: main.c
	$(CC) $(CFLAGS) -o $@ $<

scan-c-fast-simd: main.c
	$(CC) $(SIMD_CFLAGS) -O3 -DNDEBUG -o $@ $<

scan-c-counters: main.c
	$(CC) $(SIMD_CFLAGS) -O3 -DDO_INST=1 -o $@ $<

scan-c-debug: main.c
	$(CC) $(CFLAGS) -g -o $@ $<

main.s: main.c
	$(CC) $(CFLAGS) -O3 -DNDEBUG -S -mllvm --x86-asm-syntax=intel -o $@ $<

time-simd: scan-c-fast-simd
	time ./scan-c-fast-simd < $(SAMPLE) > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

time: scan-c-fast
	time ./scan-c-fast < $(SAMPLE) > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

counts: scan-c-counters
	time ./scan-c-counters < $(SAMPLE) > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

test: scan-c-debug
	time ./scan-c-debug < $(SAMPLE) > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

debug: scan-c-debug
	lldb ./scan-c-debug $(SAMPLE)

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

docker.txt: $(SAMPLE)
	docker build -t hexgrep:local .
	time docker run --volume $(abspath $(SAMPLE)):/sample --rm -i hexgrep:local sample > $@ || true
	time docker run --volume $(abspath $(SAMPLE)):/sample --rm -i hexgrep:local sample > $@ || true
	time docker run --volume $(abspath $(SAMPLE)):/sample --rm -i hexgrep:local sample > $@ || true

node-red.tar:
	docker pull nodered/node-red-docker:v10
	docker save -o $@ nodered/node-red-docker:v10

node-red2x.tar: node-red.tar
	cat $< $< > $@

node.tar:
	docker pull node:latest
	docker save -o $@ node:latest

raw.tar: node.tar
	cat $< $< > $@

hexgrep-linux: Makefile main.c
	mkdir -p linux
	cp $^ linux/
	docker run --rm --volume $(abspath linux):/work --workdir /work buildpack-deps:xenial make hexgrep-static
	cp linux/hexgrep-static hexgrep-linux
