#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/q2-game-framework-$$"
trap 'rm -f "$out"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -pedantic core/game_framework.c tests/game_framework_test.c -o "$out"
"$out"
