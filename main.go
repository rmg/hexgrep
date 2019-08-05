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
	"fmt"
	"io"
	"log"
	"os"
	"runtime"
	"runtime/pprof"
)

const BUF = 64 * 1024 * 1024

func searchWrite(buf []byte, out io.Writer) int {
	count := 0
	for i, b := range buf {
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

func pmain() {
	c, err := os.Create("sha1scan.cpu.prof")
	if err != nil {
		log.Fatal("could not create CPU profile: ", err)
	}
	defer c.Close()
	if err := pprof.StartCPUProfile(c); err != nil {
		log.Fatal("could not start CPU profile: ", err)
	}

	scan(os.Stdin)

	pprof.StopCPUProfile()
	runtime.GC() // get up-to-date statistics

	for _, p := range pprof.Profiles() {
		m, err := os.Create(fmt.Sprintf("sha1scan.%s.prof", p.Name()))
		if err != nil {
			log.Fatalf("could not create %s profile: ", p.Name(), err)
		}
		defer m.Close()
		if err := p.WriteTo(m, 1); err != nil {
			log.Fatal("could not write %s profile: ", p.Name(), err)
		}
	}
}
