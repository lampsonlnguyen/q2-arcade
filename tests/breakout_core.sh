#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/q2-breakout-core-$$"
trap 'rm -f "$out"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic core/breakout_core.c tests/breakout_core_test.c -o "$out"
"$out"
