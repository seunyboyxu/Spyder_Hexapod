#!/usr/bin/env bash
# PC tests for the standalone walking sketch (firmware/spyder_walk).
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/spyder-walk-tests.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -g -fsanitize=undefined,address -fno-omit-frame-pointer -I firmware/spyder_walk tests/walk/walk_test.cpp firmware/spyder_walk/Walk.cpp -o "$build_dir/walk_test"
"$build_dir/walk_test"
