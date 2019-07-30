#!/usr/bin/env node

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
