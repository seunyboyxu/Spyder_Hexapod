#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir="$(mktemp -d /tmp/spyder-walk.XXXXXX)"
trap 'rm -rf "$build_dir"' EXIT
dir=firmware/spyder_esp32_ble_walk
flags=(-pthread -std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie -I$dir)
g++ "${flags[@]}" tests/esp32_walk/walk_test.cpp $dir/FourLegTest.cpp $dir/LegTest.cpp $dir/Walk.cpp -o "$build_dir/walk"
"$build_dir/walk"
# The copied four-leg controller must still pass the four-leg suites.
g++ "${flags[@]}" tests/esp32_four_leg/coordinator_test.cpp $dir/FourLegTest.cpp $dir/LegTest.cpp $dir/Walk.cpp -o "$build_dir/core"
"$build_dir/core"
g++ "${flags[@]}" tests/esp32_four_leg/batch_calibration_test.cpp $dir/FourLegTest.cpp $dir/LegTest.cpp $dir/Walk.cpp -o "$build_dir/batch"
"$build_dir/batch"
g++ "${flags[@]}" tests/leg_core/leg_test.cpp $dir/LegTest.cpp -o "$build_dir/leg"
"$build_dir/leg"
g++ "${flags[@]}" -Itests/esp32_ble_leg/hardware -Itests/esp32_leg/hardware -Itests/esp32_servo/hardware tests/esp32_walk/sketch_test.cpp \
 $dir/BleUart.cpp $dir/LegTest.cpp $dir/FourLegTest.cpp $dir/Walk.cpp $dir/CommandInputs.cpp \
 $dir/PcaOutputs.cpp $dir/NvSettings.cpp -o "$build_dir/walk-sketch"
"$build_dir/walk-sketch"
