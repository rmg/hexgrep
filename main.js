#!/usr/bin/env node
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

const NL = Buffer.from('\n');
process.stdin.on('data', read_buf);

let prev = Buffer.alloc(0);
function read_buf(inbuf) {
  let buf = Buffer.concat([prev, inbuf]);
  let off = scan_slice(buf);
  prev = buf.slice(buf.length - off);
}

function is_hex(c) {
  return (c >= 48 //'0'
          && c <= 57 //'9'
         ) || (
         c >= 97 //'a'
         && c <= 102 //'f'
         )
}

function scan_slice(buf) {
  let count = 0;
  for (let i = 0; i< buf.length; i++) {
    if (count === 0 && i+20 < buf.length && !is_hex(buf[i+20])) {
      i += 20;
      continue;
    }
    if (is_hex(buf[i])) {
      count++;
      continue;
    }
    if (count === 40) {
      process.stdout.write(buf.slice(i-40, i));
      process.stdout.write(NL);
    }
    count = 0;
  }
  if (count === 40) {
    process.stdout.write(buf.slice(buf.length-40));
    process.stdout.write(NL);
    count = 0;
  }
  return count > 40 ? 41 : count;
}
