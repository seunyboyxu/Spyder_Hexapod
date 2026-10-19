# Revision 3 serial and software reference

Serial: **115200 baud**, ASCII, newline terminated, optional CRLF. Each command is `id VERB arguments`, with request ID 1–65535. Joint IDs are 0–11; the [current wiring table](project-state.md) maps them to D2–D13. Source of truth: [Protocol.cpp](../firmware/spyder_uno/Protocol.cpp) and [Control.cpp](../firmware/spyder_uno/Control.cpp).

A command returns `ACK id` or `ERR id code`. HELLO first returns `SPYDER 3`; boot emits `BOOT SPYDER 3`. The console rejects old firmware. Error codes include INVALID, RANGE, STATE, CALIBRATION, POSE, OVERFLOW and STORAGE; malformed framing/ID can use error ID 0.

| Verb / arguments | Meaning / constraints |
|---|---|
| `HELLO` | Version check; does not move or renew the lease. |
| `STATUS` | Read state; no automatic output enable. |
| `PING` | Renew heartbeat; never clear hold. |
| `RESUME` | Explicitly clear hold and renew lease; cancelled targets do not restart. |
| `ARM joint pulse` | Deliberately attach one detached joint at the supplied pulse, within its limits and with a current lease. Can jump from unknown physical position. |
| `CENTER joint` | Unloaded mode only: all outputs must be released, or the same bench joint active. Requires Resume/current lease. Attach only that output at nominal 1500 µs, or approach center at 100 µs/s if already attached. Clear its travel-check flag/standing record in RAM; preserve leg limits. |
| `SWEEP joint minimum maximum speed` | Requires the same bench joint centered and not already sweeping/held. Validate 1000 ≤ minimum < 1500 < maximum ≤ 2000 and speed 10–100 µs/s. One pass center → minimum → maximum → center, then hold. |
| `MOVE joint pulse speed` | Set an attached joint's target; requires resumed state, configured electrical limits and speed 10–100 µs/s. Travel-checked marking is recorded separately and does not gate individual calibration moves. |
| `CONFIG joint min max initial direction checked` | All outputs must be detached. Hard pulse envelope 1000–2000 µs; min < max, initial within range, direction ±1, checked 0/1. Invalidates standing record. |
| `STOP` | Latch hold at current commanded pulses and discard transitions; attachments remain. |
| `RELEASE` | Detach all signals, drive LOW and discard transitions; does not switch supply off or clear an existing hold. |
| `CAPTURE` | Record commanded standing pulses only if resumed and all twelve joints are checked, attached and stationary. |
| `POSE p0 ... p11` | Import a standing record while detached; every joint must be marked checked and each pulse inside its limits. No motion. |
| `STAND speed` | Approach a valid standing record; requires all twelve attached, resumed and checked. No balance/foot placement planning. |
| `SAVE` / `LOAD` | EEPROM settings, detached only; neither auto-arms. |

For a connectivity check with servo power off, send `1 HELLO` then `2 STATUS`. Do not use ARM as a port check or send a full movement script copied from documentation.

STATUS returns `BEGIN id stopped standingValid benchJoint sweepPhase`, twelve rows of `JOINT id index pin min max initial direction calibrated attached commanded target standing`, `END id`, then ACK. The console accepts only a complete matching snapshot. Those pulse values are commands, not measured positions. Rows are streamed while the scheduler runs; a snapshot is not a simultaneous hardware measurement.

The controller evaluates a 1500 ms lease, including detached operation, and latches hold on expiry. Its 20 ms scheduler caps late processing at one normal step to avoid catch-up jumps. The browser normally sends heartbeats every 250 ms and reads status every 750 ms, also servicing due heartbeats during foreground traffic. ACK timeout is 2500 ms. Urgent STOP/Release bypass ordinary response waits; complete USB writes already submitted retain transport order and cannot be recalled.

EEPROM version 1 uses semantic validation and a CRC with the commit marker written last. An interrupted save can discard the previous record; it is not a two-bank transactional store. Exported JSON version 1 contains twelve calibration records, an optional standing pulse array, four physical leg labels and a front description. Labels are profile/browser metadata, not EEPROM data. Partial interrupted import can leave settings changed; inspect state before retrying.

Architecture: `Control` is independent of Arduino hardware; `Outputs` is adapted to Servo by the sketch. `Protocol` validates and dispatches; `Storage` handles EEPROM bytes through a store abstraction. The browser separates protocol validation, serial/session authority, UI and demo. No upstream gait, inverse kinematics or physical feedback is used by this calibration implementation.

Unloaded mode reports `benchJoint=-1` when inactive, otherwise its sole attached joint index. Phase 0 means holding/recentering, 1 toward minimum, 2 toward maximum and 3 back to center. Existing JOINT limits remain the saved leg limits; only the selected unloaded output may command outside them within 1000–2000 µs. Normal ARM/MOVE/pose commands are blocked until Release exits the mode. STOP and watchdog expiry clear the phase; Resume never restarts the cycle. EEPROM/JSON schema stays version 1. See [unloaded instructions](unloaded-servo-calibration.md).
