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
// use std::str;

fn hit(needle: &[u8]) {
	stdout().write(&needle).ok();
	stdout().write(&[b'\n']).ok();
}

const IS_HEX : [bool;256] = [
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
];

#[derive(Debug)]
enum ScanState<T> {
	Matched(T),
	Skipping,
}

fn scan_slice(buf: &[u8]) -> usize {
	let len = buf.len();
	let mut i = 0;
	let mut state = ScanState::Matched(0);
	while i < len {
		state = if IS_HEX[buf[i] as usize] {
			match state {
				ScanState::Matched(n) => {
					ScanState::Matched(n+1)
				},
				ScanState::Skipping => {
					ScanState::Matched(1)
				}
			}
		} else {
			match state {
				ScanState::Matched(40) => {
					hit(&buf[i-40..i]);
					ScanState::Skipping
				},
				_ => ScanState::Skipping
			}
		};
		i += 1
	}
	if let ScanState::Matched(count) = state {
		if count > 40 {
			41
		} else {
			count
		}
	} else {
		0
	}
}

fn sscan(mut input: impl Read) {
	// let mut backbuf = vec![0u8; 64*1024*1024];
	// let bbuf = backbuf.as_mut_slice();
	let mut buf = [0u8; 64*1024];
	let mut off = 0;
	let mut total_read = 0;
	let mut blocks = 0;
	while let Ok(n) = input.read(&mut buf[off..]) {
		total_read += n;
		if n == 0 {
			break
		}
		off = scan_slice(&buf[..n]);
		blocks += 1;
		if off > 0 {
			eprintln!("Remainder: {}", off)
		}
		for i in 0..off {
			// eprintln!("copy: {}/{}", i, off);
			buf[i] = buf[(n-off)+i];
		}
	}
	if off >= 40 {
		eprintln!("Need to scan remainder: {}", off)
	}
	eprintln!("Total bytes read: {} (blocks: {})", total_read, blocks);
}

fn main() {
	sscan(stdin().lock())
}
