#!/bin/bash
set -euo pipefail

find -type d | while read d; do
	if [ $d == . ]; then
		continue
	fi
	echo $d
	cd $d
	../../build/fbindgen in.h < filters.txt
	rm includes.txt
	diff ref.rs out.rs
	rm out.rs
	cd ..
done
