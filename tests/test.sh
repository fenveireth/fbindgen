#!/bin/bash
set -euo pipefail

find -type d | while read d; do
  if [ $d == . ]; then
    continue
  fi
  echo $d
	pushd $d
  ../../build/fbindgen in.h < filters.txt
  diff ref.rs out.rs
	popd
done
