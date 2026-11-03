#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT="${TMPDIR:-/tmp}/aideco-generator-smoke"

c++ -std=c++23 -Wall -Wextra -Wpedantic \
    "$ROOT/src/LocalGenerator.cpp" \
    "$ROOT/src/DecorationPlacer.cpp" \
    "$ROOT/tests/LocalGeneratorSmoke.cpp" \
    -o "$OUT"
"$OUT"
echo "Local generator + decoration placement tests passed."
