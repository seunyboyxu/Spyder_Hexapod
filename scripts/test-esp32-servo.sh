#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir="$(mktemp -d /tmp/spyder-esp32-test.XXXXXX)"
trap 'rm -rf "$build_dir"' EXIT
g++ -std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie \
 -Ifirmware/spyder_esp32_servo_test tests/esp32_servo/bench_test.cpp firmware/spyder_esp32_servo_test/ServoBench.cpp -o "$build_dir/bench-test"
"$build_dir/bench-test"
g++ -std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie \
 -Itests/esp32_servo/hardware -Ifirmware/spyder_esp32_servo_test tests/esp32_servo/output_test.cpp \
 firmware/spyder_esp32_servo_test/PcaOutput.cpp firmware/spyder_esp32_servo_test/ServoBench.cpp -o "$build_dir/output-test"
"$build_dir/output-test"
g++ -std=c++11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie \
 -Itests/esp32_servo/hardware -Ifirmware/spyder_esp32_servo_test tests/esp32_servo/sketch_test.cpp \
 firmware/spyder_esp32_servo_test/PcaOutput.cpp firmware/spyder_esp32_servo_test/ServoBench.cpp -o "$build_dir/sketch-test"
"$build_dir/sketch-test"
