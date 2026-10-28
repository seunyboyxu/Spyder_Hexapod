#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir="$(mktemp -d /tmp/spyder-esp32-leg.XXXXXX)"
trap 'rm -rf "$build_dir"' EXIT
flags=(-std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie -Ifirmware/spyder_esp32_leg_test)
g++ "${flags[@]}" tests/leg_core/leg_test.cpp firmware/spyder_esp32_leg_test/LegTest.cpp -o "$build_dir/core"
"$build_dir/core"
g++ "${flags[@]}" tests/leg_core/inputs_test.cpp firmware/spyder_esp32_leg_test/CommandInputs.cpp -o "$build_dir/inputs"
"$build_dir/inputs"
g++ "${flags[@]}" -Itests/esp32_servo/hardware tests/esp32_leg/output_test.cpp firmware/spyder_esp32_leg_test/PcaLegOutput.cpp -o "$build_dir/output"
"$build_dir/output"
g++ "${flags[@]}" -Itests/esp32_leg/hardware -Itests/esp32_servo/hardware tests/esp32_leg/sketch_test.cpp \
 firmware/spyder_esp32_leg_test/LegTest.cpp firmware/spyder_esp32_leg_test/CommandInputs.cpp \
 firmware/spyder_esp32_leg_test/PcaLegOutput.cpp firmware/spyder_esp32_leg_test/NvSettings.cpp -o "$build_dir/sketch"
"$build_dir/sketch"
