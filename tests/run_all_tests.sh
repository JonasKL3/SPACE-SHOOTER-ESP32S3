#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
BUILD="$(mktemp -d)"
trap 'rm -rf "$BUILD"' EXIT
for testcase in test_regressions test_v656_features test_v660_features test_arduino_prototype test_boss_rush_long test_long_gameplay test_fuzz; do
  echo "=== $testcase (ASan + UBSan, C++17) ==="
  g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I stubs "$testcase.cpp" -o "$BUILD/$testcase"
  ASAN_OPTIONS=detect_leaks=0 "$BUILD/$testcase"
done
printf '\nALL TESTS PASSED\n'

echo "=== ESP32 guarded diagnostic code (fake IDF headers + ASan/UBSan) ==="
g++ -std=c++17 -O1 -g -DESP32 -fsanitize=address,undefined -fno-omit-frame-pointer -I stubs test_v656_features.cpp -o "$BUILD/diag_sanity"
ASAN_OPTIONS=detect_leaks=0 "$BUILD/diag_sanity"
