# Development and verification

Run commands from the repository root unless stated otherwise. These commands do not upload firmware or operate motors. For operating instructions use [bring-up](bring-up.md).


From the repository root on Linux, with Python 3, Node 20+, a C++ compiler, Chromium and Git:

```sh
bash scripts/test-native.sh
npm ci --cache /tmp/spyder-npm-cache
npm test
npm run test:browser
```

The regression suites cover repeated slow nudges, continuous-traffic heartbeats, urgent STOP/Release and stale readbacks, initial-pulse synchronization and demo timing. The integration test compiles the actual C++ control/protocol/storage and connects through the browser serial adapter using host streams, without Arduino hardware. Browser tests use a labelled demo; neither substitutes for physical tests. Playwright uses `/usr/bin/chromium` when present; otherwise run `npx playwright install chromium`, or set `CHROMIUM_PATH` to an installed compatible browser.

Standard PlatformIO build:

```sh
python3 -m venv .venv
.venv/bin/pip install platformio==6.1.18
PLATFORMIO_CORE_DIR="$PWD/.platformio" .venv/bin/pio run -e uno
```

PlatformIO pins `atmelavr@5.1.0` and Servo `1.2.2`. This cloud's proxy denied the PlatformIO registry; that build is not reported as passed. A verified Debian AVR toolchain provides an alternative actual ATmega328P compile:

```sh
python3 scripts/bootstrap-avr.py
python3 scripts/build-avr.py
```

The Linux x86-64 fallback needs `curl`, `gpg`, the Debian archive keyring and `dpkg-deb`. It validates Debian signatures and package checksums, extracts tools under ignored `.tools/`, and pins Servo to its 1.2.2 commit. It uses AVR GCC 14.2 and Arduino AVR core 1.8.7, a separate configuration from PlatformIO. Output: `.pio/local-uno/spyder_uno.hex`. No script uploads or operates motors.

For actual MCU waveform regressions, install the Linux simavr development library and libelf runtime (Debian packages `libsimavr-dev` and `libelf1t64`, or your distribution equivalents), then run `python3 scripts/test-avr.py` after the AVR build. For packages extracted rather than installed, pass `--simavr-root /path/to/package-root`. The runner executes 25 scenarios, including a complete isolated unloaded sweep and writes edge traces under `.pio/avr-tests/`. This simulates ideal electrical conditions; it cannot reproduce USB supply dips or motor mechanics.

See [verification results](verification.md) for exercised checks and remaining hardware work. The selected STL and native Fusion assets are now under [cad/](../cad/README.md); fitted geometry and real motion remain unverified.


## Repository integrity

Run `sha256sum -c cad/SHA256SUMS` to check the selected CAD bytes. The [18 October handover verification](verification/2026-10-18-handover.md) records the organization checks and the unchanged calibration sources. Keep generated toolchains, environments and test output out of Git; the existing `.gitignore` covers them.

## Static console releases

Console 3.2 uses matching `?v=3.2` URLs for the entry module, all internal imports and CSS. Bump that asset version coherently for future browser releases, including browser tests that import the demo for controlled scenarios. Firmware protocol remains SPYDER 3 unless its wire contract changes. `Start-Console.bat` uses an absolute adjacent `web/` path and port 8033; keep it beside that folder in console-only downloads.
