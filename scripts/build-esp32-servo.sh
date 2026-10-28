#!/usr/bin/env bash
# Compile only. No upload target is invoked.
set -euo pipefail
cd "$(dirname "$0")/.."
repo_root="$PWD"
build_dir="$repo_root/.tools/esp32-build"
mkdir -p "$build_dir"
cat > "$build_dir/platformio.ini" <<'CONFIG'
[platformio]
src_dir = ../../firmware/spyder_esp32_servo_test
[env:esp32dev]
platform = espressif32@6.12.0
board = esp32dev
framework = arduino
lib_ldf_mode = deep+
lib_deps = adafruit/Adafruit PWM Servo Driver Library@3.0.2
    adafruit/Adafruit BusIO@1.17.4
monitor_speed = 115200
CONFIG
if [[ -x "$repo_root/.tools/pio-venv/bin/pio" ]]; then
  pio_command="$repo_root/.tools/pio-venv/bin/pio"
elif command -v pio >/dev/null 2>&1; then
  pio_command="$(command -v pio)"
else
  echo 'Install PlatformIO Core (e.g. in .tools/pio-venv), or use Arduino IDE as documented in the sketch README.' >&2
  exit 1
fi
export PLATFORMIO_CORE_DIR="$repo_root/.tools/platformio"
"$pio_command" run --project-dir "$build_dir"
