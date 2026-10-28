#!/usr/bin/env bash
# All published ESP32 host checks; no upload or motor actuation.
set -euo pipefail
cd "$(dirname "$0")/.."
for suite in servo leg ble-leg four-leg walk ble-compat; do
  bash "scripts/test-esp32-$suite.sh"
done
