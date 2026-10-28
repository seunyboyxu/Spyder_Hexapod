# Ryan walking integration and merge verification — 28 October 2026

Owner request: merge the ESP32 branches, include Ryan's latest work and provide complete runnable Arduino IDE files. Source: RyanFarhat's PR#3, esp32-walk, original commit0447bac4731c6918a18c37bc2db8336168a3b44c (commit authored by Claude for Ryan). It builds on ESP32 cleanup PR#2. The owner explicitly authorized both merges; the legacy board implementation is not restored.

## Integration

Added the walking sketch to the root PlatformIO selector/environment, made it the default build, added an isolated compile helper, included walking in the aggregate test runner and included its BLE adapter in compatibility tests. Updated the sketch index, upload/handover/function guides and saved-calibration instructions. Current walking version is1.1; BLE name Spyder-Walk-BLE. PCA map remains consecutive triples0–11, GPIO21 SDA/GPIO22 SCL/GPIO23 OE; Preferences spyder-four/cal is shared with the four-leg calibration sketch.

## Review finding and correction

Independent review found that original walkFits checked startup/steady samples but omitted stopping and direction-transition paths. A checked calibration with leg3 hip minimum81.0° accepted left followed by halt after601 gait updates, then silently stopped near81.3° instead of returning to standing90°. Earlier legs could already have moved before the last recipient rejected, and the sketch could report Ready.

The new regression failed against the original implementation. Corrected behavior validates a clone of the actual requested stop/direction lifecycle before changing live state, including eventual stops for finite walks. Long finite simulations cap at three future boundaries: enough for arbitrary-phase down-ramp, up-ramp and representative steady behavior; a five-cycle bounded horizon includes stopping. Unsafe halt/direction commands now return ERR with the current motion unchanged; stop/off remain available. Halt can take over6seconds and several more steps, so the docs no longer imply immediate next-step completion.

Every runtime frame validates all four legs before writing any channel. A timing-dependent path rejection holds all legs and raises a one-shot error; the sketch emits ERR rather than Ready. A leg4 foot-minimum74.4° fixture with22ms ticks exercises this condition. The regression failed when all-leg preflight was removed in a temporary build, then passed against the correction. An actual-sketch BLE fixture verifies explicit ERR and absence of Ready.

These are sampled requested-lifecycle checks, not a proof for all real timing or physical trajectories. Runtime frame validation remains necessary. Ryan's gait generator and geometry settings are unchanged.

## Fresh evidence

- `bash scripts/test-esp32.sh`: all six suites passed under ASan/UBSan, including walking/cancellation/transition/runtime-error regressions and all three BLE adapters against the core3 String API model.
- `PLATFORMIO_CORE_DIR="$PWD/.tools/platformio" .tools/pio-venv/bin/pio run -e esp32_ble_walk`: original ESP32 Arduino2.0.17 actual compile/link passed; flash1,178,541/1,310,720bytes, static/global RAM40,124/327,680bytes.
- Arduino CLI compile of the actual complete walking sketch with ESP32 core3.3.2 passed; flash1,140,579/1,310,720bytes, static/global RAM41,544/327,680bytes. This runs Arduino sketch conversion, not just the root C++ selector.
- Dependencies: Adafruit PWM Servo Driver3.0.2 and BusIO1.17.4. Static RAM totals exclude dynamic BLE heap/task stack use. No upload target invoked.
- Independent read-only review reproduced the original defect, reran the corrected walking suite, confirmed the transition/runtime regression fixtures and found no additional material defect in the bounded correction.
- Repository link/whitespace and CAD checksum checks performed before publication; no CAD bytes changed.

## Running on hardware

The delivery ZIP contains the complete spyder_esp32_ble_walk folder, calibration/hardware instructions and source commit/checksum provenance. Extract it, open the matching .ino, keep every companion header/source and upload to ESP32 Dev Module with servo power off. USB115200/Newline; iPhone connects inside a NUS app with notifications enabled. Use leg all → armall → stand → `walk 1` as separate commands only after physically checked calibration, waiting for Ready between movements. If calibration is missing, follow cal all and individual alignment/direction/limit checks before check all, off and save. Do not bypass checks.

No physical board, robot or iPhone is attached here. Actual radio behavior, reset/OE bias, supply integrity, geometry, centre of mass, contact/slip, clearance and balance remain unverified. The known PP3 battery plus5V/3A buck is inadequate for multi-servo testing; replace source/distribution before powered walking. Begin with body support and finite `walk 1`; stop is immediate hold, off/! signal release, halt is a slower validated stop and may be rejected if unsafe. A rejected halt leaves the current walk running: use stop/off. Never rely on nominal positions or mathematical support margins as measured physical balance.
