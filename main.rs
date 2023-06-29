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

use std::io::{stdin, stdout, Read, Write};

fn hit(needle: &[u8]) {
    let mut out = stdout().lock();
    out.write_all(needle).unwrap();
    out.write_all(&[b'\n']).unwrap();
}

fn scan_slice(inb: &[u8]) -> usize {
    let mut count = 0;
    let len = inb.len();
    let mut i = 0usize;
    while i < len {
        let b = inb[i];
        if count == 0 && i + 20 < len {
            let bs = inb[i + 20];
            if !bs.is_ascii_digit() && !(b'a'..=b'f').contains(&bs) {
                i += 20;
                continue;
            }
        }
        if b.is_ascii_digit() || (b'a'..=b'f').contains(&b) {
            count += 1;
            i += 1;
            continue;
        }
        if count == 40 {
            hit(&inb[i - 40..i]);
        }
        count = 0;
        i += 1;
    }
    if count == 40 {
        hit(&inb[len - 40..]);
        count = 0
    }
    if count > 40 {
        41
    } else {
        count
    }
}

fn sscan(mut input: impl Read) {
    let mut backbuf = vec![0u8; 64 * 4096];
    let bbuf = backbuf.as_mut_slice();
    // let mut bbuf = [0u8; 2*1024*1024];
    let mut off = 0;
    let mut total_read = 0;
    while let Ok(n) = input.read(&mut bbuf[off..]) {
        total_read += n;
        if n == 0 {
            break;
        }
        off = scan_slice(&bbuf[..n]);
        for i in 0..off {
            bbuf[i] = bbuf[n - off + i];
        }
    }
    eprintln!("Total bytes read: {}", total_read);
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() == 2 {
        let file = std::fs::File::open(&args[1]).unwrap();
        sscan(file)
    } else {
        sscan(stdin().lock())
    }
}
