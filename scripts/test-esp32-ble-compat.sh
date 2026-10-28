#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir="$(mktemp -d /tmp/spyder-ble-compat.XXXXXX)"
trap 'rm -rf "$build_dir"' EXIT
for variant in spyder_esp32_ble_leg_test spyder_esp32_ble_four_leg_test; do
 g++ -pthread -std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie \
  -I"firmware/$variant" -Itests/esp32_ble_compat/hardware -Itests/esp32_ble_leg/hardware -Itests/esp32_servo/hardware \
  tests/esp32_ble_leg/ble_test.cpp "firmware/$variant/BleUart.cpp" -o "$build_dir/$variant"
 "$build_dir/$variant"
done
