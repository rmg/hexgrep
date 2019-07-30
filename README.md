# Fixed Pattern Scanner

The goal is to extract every 40 character hex string from a source and print
them for later examination to see if they are valid git commit shas.

```
$ grep -o -E '/[0-9a-f]{40}/' < input > maybe-commits.txt
```

The obstacle is this needs to be run over a lot of data and fairly frequently.

## Times

### grep

```
time grep -E -a -o '[0-9a-f]{40}' < raw.tar > grep.txt

real    0m8.492s
user    0m7.396s
sys     0m1.033s
```

### ripgrep

```
time rg -o -a '[a-f0-9]{40}' < raw.tar > ripgrep.txt

real    0m0.845s
user    0m0.734s
sys     0m0.084s
```

### Custom C

```
time ./scan-c < raw.tar > c.txt

real    0m0.838s
user    0m0.761s
sys     0m0.073s
```

### Custom Go

```
time ./scan-go < raw.tar > go.txt

real    0m2.242s
user    0m1.797s
sys     0m0.774s

```

### Custom Rust

```
time ./scan-rs < raw.tar > rs.txt

real    0m0.952s
user    0m0.682s
sys     0m0.261s
```

### Custom Node.js

```
time node main.js < raw.tar > js.txt

real    0m3.783s
user    0m3.144s
sys     0m0.787s
```
