all: c.txt go.txt rs.txt grep.txt ripgrep.txt js.txt

time: scan-c
	time ./scan-c < super.tar > c.txt
	diff -q c.txt prev.txt || diff c.txt prev.txt | wc -l

scan-c: main.c
	$(CC) -Werror -Wall -O3 -o $@ $<

scan-go: main.go
	go build -o $@ $<

scan-rs: main.rs
	rustc -O -o $@ $<

%.txt: scan-% raw.tar
	time ./scan-$* < raw.tar > $@

js.txt: main.js raw.tar
	time node main.js < raw.tar > $@

grep.txt: raw.tar
	time grep -E -a -o '[0-9a-f]{40}' < raw.tar > $@

ripgrep.txt: raw.tar
	time rg -o -a '[a-f0-9]{40}' < raw.tar > $@

raw.tar:
	docker pull node:latst
	docker save -o $@ node:latest
