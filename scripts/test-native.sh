#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/spyder-tests.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT
for source in tests/native/*_test.cpp; do
  name=$(basename "$source" .cpp)
  g++ -std=c++11 -Wall -Wextra -Werror -pedantic -g -fsanitize=undefined,address -fno-omit-frame-pointer -I firmware/spyder_uno "$source" firmware/spyder_uno/*.cpp -o "$build_dir/$name"
  "$build_dir/$name"
done
