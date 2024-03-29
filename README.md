# Fixed Pattern Scanner

The goal is to extract every 40 character hex string from a source and print
them for later examination to see if they are valid git commit shas.

Something _kinda_ like this, except filtering out hex strings longer than 40
characters.

```
$ grep -o -E '/[0-9a-f]{40}/' < input > maybe-commits.txt
```

The obstacle is this needs to be run over a lot of data and fairly frequently,
so it is probably worth optimizing. Or at least considering optimizing.

## Inspiration

The gold standard for multi-string searching is [Aho-Corasick (and
variants)](https://en.wikipedia.org/wiki/Aho%E2%80%93Corasick_algorithm). It's
very cool and I wish I could use it. The problem is this particular pattern
explodes in to 2^160 strings, which would kind of break a naive dictionary
approach. But hey, if you notice the repetition in the pattern you'll realize it
can basically be collapsed to a single string! Now we've basically optimized
away the very property that makes Aho-Corasick fast: variance.

Since we know that the input pattern can essentially be collapsed to a single
string-ish, we can look at single-string searches. The gold standard here is
[Boyer-Moore (and
variants)](https://en.wikipedia.org/wiki/Boyer%E2%80%93Moore_string-search_algorithm).
These algorithms work by combining a couple cool tricks. The first is they
pre-compute a jump table for the needle so that given any character you can
immediately make the optimal shift in the haystack. The second clever trick is
to search from the end of the needle and move backwards, which lets you skip up
to the entire needle length in the best cases.

## Implementation

Naive implementations were done (in order) in Rust, Go, and JavaScript (node).
After implementing the same naive implementation in C I started down the path
that brought me here. In all cases, there are no dependencies other than the
relevant compiler/runtime; just standard i/o.

By applying some of the Boyer-Moore approach to a pattern instead of a string,
we can gain a couple features. We don't need to pre-process the needle since
each byte of the needle is effectively identical, but we can still make use of
the skipping because we do have a known fixed length.

In theory, what we end up with is a special case of what a good regular
expression engine would generate, but without the overhead of supporting other
types of patterns.

## Times

These times are best of 3 runs of piping sample file that is representative of
my intended workload (tar file generated by `docker save`).

The times themselves don't matter much; I'm more interested in how the different
implementations compare to each other.

### 329MB Sample

| Time | real | user | system |
|------|------|------|--------|
| grep | 0m8.105s | 0m7.194s | 0m0.893s |
| ripgrep | 0m0.821s | 0m0.735s | 0m0.071s |
| simple (Go) | 0m2.242s | 0m1.797s | 0m0.774s |
| simple (Rust) | 0m0.922s | 0m0.675s | 0m0.244s |
| simple (Node) | 0m3.713s | 0m3.113s | 0m0.732s |
| custom (C) | **0m0.123s** | **0m0.055s** | **0m0.066s** |

### 986MB Sample (Sample 1 x3)

| Time | real | user | system |
|------|------|------|--------|
| grep | 0m18.034s | 0m15.713s | 0m2.257s |
| ripgrep | 0m1.709s | 0m1.541s | 0m0.147s |
| simple (Go) | 0m1.737s | 0m1.594s | 0m0.142s |
| skip (Go) | 0m0.338s | 0m0.187s | 0m0.152s |
| simple (Rust) | 0m1.461s | 0m1.325s | 0m0.131s |
| skip (Rust) | 0m0.231s | 0m0.105s | 0m0.124s |
| simple (Node) | 0m6.458s | 0m6.043s | 0m0.627s |
| skip (Node) | 0m1.368s | 0m1.062s | 0m0.686s |
| custom (C) | **0m0.222s** | **0m0.079s** | **0m0.141s** |

By comparing the times you can see that each implementation is more or less
*O(n)* (or *O(nm)*;  since they are all using the same needle size it
essentially becomes a constant). The difference is in the constant factor. On
this sample, the custom search only actually looks at 1/9 of the bytes being
processed thanks to aggressive skipping.

## License

Copyright 2019 Ryan Graham

All source code licensed under Apache-2.0. See [LICENSE](LICENSE).
