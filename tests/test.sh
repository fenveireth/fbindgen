#!/bin/bash
set -euo pipefail

find -type d | while read d; do
  if [ $d == . ]; then
    continue
  fi
  echo $d
  ../build/fbindgen $d/in.h < $d/filters.txt > $d/out.rs
  diff $d/ref.rs $d/out.rs
done
