all: c.txt go.txt rs.txt grep.txt ripgrep.txt js.txt

scan-c: main.c
	$(CC) -O -o $@ $<

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
