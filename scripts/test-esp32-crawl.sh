#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir="$(mktemp -d /tmp/spyder-crawl.XXXXXX)"
trap 'rm -rf "$build_dir"' EXIT
dir=firmware/spyder_esp32_ble_crawl
# Only the gait (Walk.*, WalkConfig.h), the sketch .ino and README differ from
# the walk sketch; every other file must stay byte-identical.
for f in BleUart.cpp BleUart.h CommandInputs.cpp CommandInputs.h FourLegSafety.h \
  FourLegTest.cpp FourLegTest.h LegTest.cpp LegTest.h NvSettings.cpp NvSettings.h PcaOutputs.cpp PcaOutputs.h; do
  cmp -s "$dir/$f" "firmware/spyder_esp32_ble_walk/$f" || { echo "FAIL $f differs from spyder_esp32_ble_walk" >&2; exit 1; }
done
flags=(-pthread -std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie -I$dir)
g++ "${flags[@]}" tests/esp32_crawl/crawl_test.cpp $dir/FourLegTest.cpp $dir/LegTest.cpp $dir/Walk.cpp -o "$build_dir/crawl"
"$build_dir/crawl"
# The copied four-leg controller must still pass the four-leg suites.
g++ "${flags[@]}" tests/esp32_four_leg/coordinator_test.cpp $dir/FourLegTest.cpp $dir/LegTest.cpp $dir/Walk.cpp -o "$build_dir/core"
"$build_dir/core"
g++ "${flags[@]}" tests/esp32_four_leg/batch_calibration_test.cpp $dir/FourLegTest.cpp $dir/LegTest.cpp $dir/Walk.cpp -o "$build_dir/batch"
"$build_dir/batch"
g++ "${flags[@]}" tests/leg_core/leg_test.cpp $dir/LegTest.cpp -o "$build_dir/leg"
"$build_dir/leg"
g++ "${flags[@]}" -Itests/esp32_ble_leg/hardware -Itests/esp32_leg/hardware -Itests/esp32_servo/hardware tests/esp32_crawl/sketch_test.cpp \
 $dir/BleUart.cpp $dir/LegTest.cpp $dir/FourLegTest.cpp $dir/Walk.cpp $dir/CommandInputs.cpp \
 $dir/PcaOutputs.cpp $dir/NvSettings.cpp -o "$build_dir/crawl-sketch"
"$build_dir/crawl-sketch"
