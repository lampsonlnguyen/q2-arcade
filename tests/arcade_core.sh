#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/q2-arcade-core-$$"
trap 'rm -f "$out"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic core/arcade_core.c tests/arcade_core_test.c -o "$out"
"$out"
