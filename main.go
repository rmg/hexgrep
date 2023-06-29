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
package main

import (
	"io"
	"os"
)

const BUF = 64 * 4096

func searchWrite(buf []byte, out io.Writer) int {
	count := 0
	bl := len(buf)
	for i := 0; i < bl; i++ {
		b := buf[i]
		if count == 0 && i+20 < bl {
			bs := buf[i+20]
			if !(bs >= '0' && bs <= '9') && !(bs >= 'a' && bs <= 'f') {
				i += 20
				continue
			}
		}
		switch {
		case b >= '0' && b <= '9':
			fallthrough
		case b >= 'a' && b <= 'f':
			count += 1
		default:
			if count == 40 {
				right := i
				left := right - count
				out.Write(buf[left:right])
				out.Write([]byte{'\n'})
			}
			count = 0
		}
	}
	if count == 40 {
		right := len(buf) - 1
		left := right - count
		out.Write(buf[left:right])
		out.Write([]byte{'\n'})
		count = 0
	}
	if count > 40 {
		return 40
	}
	return count
}

func scan(input io.Reader) {
	curr := make([]byte, BUF)
	reads := make([]int, 0)
	off := 0
	for {
		n, err := input.Read(curr[off:])
		reads = append(reads, n)
		if err == io.EOF {
			break
		}
		off = searchWrite(curr[:off+n], os.Stdout)
		if off > 0 {
			copy(curr[:off], curr[n-off:n])
		}
	}
}

func main() {
	scan(os.Stdin)
}
