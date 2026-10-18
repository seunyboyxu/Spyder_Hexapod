# Revision 2 verification — 16 October 2026

This is the software verification record from 16 October. For the later CAD upload, repository organization and GitHub handover see [18 October verification](verification/2026-10-18-handover.md). Statements below about local-only work and unavailable CAD describe the earlier stage.

The calibration fixes pass the software checks below. Real-board upload, electrical measurements, servo movement and loaded standing remain unverified. The user confirmed twelve servos were powered through Uno USB and unwanted motion occurred around connection, reset and release. A suitable external servo supply and verified shield isolation are required before powered calibration resumes.

## Fresh evidence

| Check | Outcome |
|---|---|
| `bash scripts/test-native.sh` | Pass with address/undefined-behavior sanitizers: controller, protocol and storage suites, plus repeated-MOVE fractional progress, reversal and STOP regressions. Storage exercises 127 interrupted-write boundaries on both erased and existing records. |
| `npm test` | 27 passed, none skipped: real C++ firmware/Web Serial integration, stale ARM rejection, command/profile validation, heartbeat servicing during command traffic, urgent STOP/RELEASE, stale snapshots and deterministic demo timing. |
| `npm run test:browser` | 10 Chromium tests passed: initial calibration-to-arming synchronization, Release during busy operation, safety readback gating, rejected Release, delayed profile completion, unexpected hold explanation, device pin labels, USB-power warning, twelve-joint calibration/standing and narrow layout. |
| `python3 scripts/build-avr.py` | Actual ATmega328P image compiled: **11,492 bytes flash; 1,015 bytes static RAM; 1,033 bytes available for stack/runtime**. |
| `python3 scripts/test-avr.py --simavr-root /workspace/Spyder-audit/tools` | **24 actual-ELF simulation scenarios passed**. All twelve signals end LOW across 21 release timings; release/re-arm has no abnormal long HIGH interval; twelve steady channels meet 1500 ±15 µs; serial-load ramp readback reaches 1555 µs versus approximately 1557 µs expected. |
| Independent read-only review | No must-fix software issues. Reviewer independently passed native checks and 25 focused session/protocol/demo tests. |
| `git diff --check` | Pass. |
| PlatformIO Uno build | Previously blocked by proxy denial of its registry; not reported as passed. Signed Debian AVR fallback used instead. |
| SonarQube | Unrun: no analysis MCP tools or installed CLI were available. |
| Physical robot acceptance | Unverified; requires the actual board, supply, wiring and mechanics. |

The AVR configuration uses Debian AVR GCC 14.2, AVR libc 2.2.1, Arduino AVR core 1.8.7 and Servo 1.2.2 at commit `8297c322120d9eb21478eb48f1f3d80219b9d800`. Bootstrap verified Debian InRelease signatures, package-index checksum and downloaded package checksums. TLS/signature/checksum verification stayed enabled. Compiler warnings originate in upstream Arduino WString/new; application sources compile without warnings in this configuration.

The simulator is simavr 1.6, using its ATmega328P model at 16 MHz and an ideal 5 V supply. The retained C harness feeds the real UART and captures GPIO edges; the Python runner independently checks its CSV edge traces. Debian simulator packages were verified against the signed package index and extracted under the external `--simavr-root` above. A system simavr development installation can run the same checks without that option. Generated traces/logs are under `.pio/avr-tests/` and are included in the release's `verification/avr/` folder.

## Reproduced failures and implemented fixes

The earlier audit reproduced bugs before production edits. Its report describes the old release, not revision 2.

- AVR Servo detach could leave a signal HIGH when interrupted mid-pulse. Re-arm produced an approximately 68 ms HIGH interval. Release now disables that channel and explicitly drives LOW with interrupts protected; startup sets defined LOW outputs once the sketch starts.
- Full STATUS UART output slowed the movement scheduler. The output sink now services motion and the watchdog while serial bytes drain. The same serial-load scenario improved from 1528 to 1555 µs commanded, starting at 1500 µs.
- Repeated MOVE commands reset fractional progress, so 10 µs/s nudges every 80 ms could stall completely. Compatible targets retain fractions; reversals, cancellation, STOP and release discard them.
- Release was silently ignored while the UI was busy, and STOP waited for prior responses. Both now interrupt ordinary response waits, cancel superseded actions and wait for their own acknowledgement/readback. Request order prevents late older snapshots overwriting newer safety state. Already-written bytes and blocked physical USB writes remain outside this guarantee.
- Foreground command traffic starved heartbeats. The command loop now services due heartbeats before commands and readbacks. Heartbeats never clear latched hold.
- Applying an initial pulse of 1450 µs left the arming field at 1500 µs. The acknowledged calibration now updates the arming suggestion. Demo speed, fractional timing, idle lease and ARM behavior now follow the controller.

Regression checks demonstrated the relevant failures first, then passed after fixes. The independent review also checked overlapping safety controls and stale responses. Production code and tests use **SPYDER 2** handshake/boot messages; the console rejects old firmware. EEPROM and JSON profile formats remain version 1. Replace the full console and upload the matching sketch together.

## Hardware limits

**Pins remain D2–D13.** D13 is also the Uno LED signal. A bootloader can toggle it before this sketch takes control, so keep servo power off during USB connection, reset and upload. The simulator starts the application ELF and does not exercise the user's bootloader. The user explicitly chose to retain D13 and switch servo power off during connection/reset; no A0 remap is applied.

Simulation cannot establish real supply quality, power-on twitch, actual joint position, horn retention, mechanical clearances or loaded balance. First attachment still jumps to its explicit selected pulse because SG90s provide no position feedback. Nudges change outstanding targets, and other armed joints can finish their prior moves; the console explains both behaviors. Follow [bring-up.md](bring-up.md) with the body supported, first one servo, then one leg, then the full assembly.

Custom CAD referenced in the handover was not supplied as inspectable fitted geometry. The handover's backing-pivot/horn-retention concerns and the four occupied stations still need physical inspection. No claim is made that the robot already stands without slipping.

## Environment and delivery

Work remains local in the existing isolated checkout; no worktree, commit, push, flash or hardware motion was performed. The revised ZIP contains source, tests, guides, a compiled HEX, build information, AVR traces and checksums. Keep the earlier ZIP only as a historical version.

The reusable cloud draft already saves `install_script` and `start_skill` for installation, the signed AVR fallback, checks and local static-console startup, plus PlatformIO registry domain additions. Saving a draft does not apply the live proxy settings or publish a snapshot. Publication and fresh-task restoration remain unverified. The optional simulator requires its development library in addition to those saved baseline tools.
