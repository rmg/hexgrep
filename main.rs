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

use std::io::{Read, Write,stdin, stdout};
use std::str;

fn hit(needle: &[u8]) {
	stdout().write(&needle).ok();
	stdout().write(&[b'\n']).ok();
}

fn scan_slice(inb: &[u8]) -> usize {
	let mut count = 0;
	let len = inb.len();
	for (i, &b) in inb.into_iter().enumerate() {
		if b >= b'0' && b <= b'9' || b >= b'a' && b <= b'f' {
			count += 1;
			continue
		}
		if count == 40 {
			hit(&inb[i-40..i]);
		}
		count = 0
	}
	if count == 40 {
		hit(&inb[len-40..]);
		count = 0
	}
	if count > 40 { 41 } else { count }
}

fn sscan(mut input: impl Read) {
	let mut backbuf = vec![0u8; 64*1024*1024];
	let bbuf = backbuf.as_mut_slice();
	// let mut bbuf = [0u8; 2*1024*1024];
	let mut off = 0;
	let mut total_read = 0;
	while let Ok(n) = input.read(&mut bbuf[off..]) {
		total_read += n;
		if n == 0 {
			break
		}
		off = scan_slice(&bbuf[..n]);
		for i in 0..off {
			bbuf[i] = bbuf[n-off+i];
		}
	}
	eprintln!("Total bytes read: {}", total_read);
}

fn main() {
	sscan(stdin().lock())
}
