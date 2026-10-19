# Unloaded servo centering and range test

**Revision 3, 19 October 2026:** the owner reports 9 g SG90s and some MG90Ds and chose to keep the existing **1000–2000 µs test envelope** and **1500 µs nominal center** because no datasheet is available. These are default command limits, not verification of either model's full rated travel or exact shaft angle. Firmware and browser must both use SPYDER 3; older releases are rejected.

## What the controls do

The **Unloaded servo setup** panel tests one selected servo independently of the assembled leg's limits. It requires its own unloaded confirmation and explicit Resume. The twelve signal assignments remain D2–D13; retain servo power off through USB connection/reset/upload and use a suitable external servo supply with common ground and verified shield rail isolation.

- **Center selected at 90°:** attaches only that selected output at the nominal center pulse. With a bench output already attached, it cancels a sweep and approaches center at 100 µs/s. First attachment can jump because the actual shaft position is unknown.
- **Sweep selected range:** after centering has settled, makes one pass **center → minimum → maximum → center**, at the chosen 10–100 µs/s. It then holds center; it does not repeat. The endpoints must straddle center and stay inside the approved test envelope. The panel shows phase, commanded pulse and target; these are not angle measurements.
- **STOP / heartbeat loss:** cancels the sweep and latches hold at the current command. Resume never restarts it. Resume and Center before deliberately starting another sweep.
- **Release outputs:** detaches the signal and exits test mode. Release before choosing another servo or returning to normal leg controls.

All other outputs must be released before entering test mode. Firmware rejects ordinary ARM/MOVE and pose commands during the test, so another joint cannot join the sweep. The selector follows device state after reconnection. The ordinary joint controls remain unavailable until Release.

Saved leg minimum/maximum/initial/direction values are not widened by the bench test. Entering it clears the selected joint's travel-check flag and standing record in RAM, because unloaded travel is not proof of safe assembled travel. EEPROM is never automatically rewritten; export any existing calibration beforehand and save only after inspection with outputs released.

## Off-leg procedure

1. Back up the current calibration JSON. With servo power off, remove the selected motor from the leg, secure its casing and leave the shaft/horn clear. Keep hands and leads out of its sweep.
2. Use the matching revision 3 console and firmware. Connect/upload with servo power off, inspect detached/held startup, then energize the correctly isolated external servo supply.
3. Select the actual signal pin/joint, confirm the motor is unloaded, and use Resume. Click Center and wait for the commanded center to settle.
4. The 90° label means the nominal midpoint command. Record/mark the horn reference; the controller cannot verify its true angle. If centering or refitting a horn, retain the matching centre screw and do not assume a numerical midpoint guarantees a 90° assembled joint.
5. Use the selected 1000–2000 µs default window, or choose a smaller window straddling 1500 µs. The interface does not allow wider unverified endpoints. Start conservatively and watch the motor. Stop for buzzing, binding, heating or reset; do not push against internal mechanical stops to discover a supposed maximum. The test cannot detect a stall.
6. Run one sweep. It returns to center and holds. Release before selecting the next motor. Switch servo power off before connection/reset/upload or mechanical handling.
7. After refitting motors/horns, support the body and repeat the assembled leg calibration from [bring-up](bring-up.md). Set conservative joint limits, verify direction and clearance, and capture a fresh standing record only after all installed joints pass.

## Software contract and verification scope

This release uses **SPYDER 3** in both firmware and console, with EEPROM/JSON version 1 unchanged. New commands are `CENTER joint` and `SWEEP joint minimum maximum speed`. STATUS begins with `BEGIN id stopped standingValid benchJoint sweepPhase`, where `benchJoint=-1` means ordinary mode and phase 0/1/2/3 means hold-or-center / toward minimum / toward maximum / back to center. Existing JOINT rows continue to expose the original leg limits; only the isolated bench output may command outside those limits inside the approved test envelope.

Implemented checks include individual selection and exclusivity, full one-pass sequencing, stored-limit preservation, STOP/lease/reset cancellation, rejection of ordinary outputs during bench mode, protocol validation, actual native firmware/session integration and UI reconnect selection. The 1000–2000 µs actual compiled-AVR simulation reached both endpoints, returned to 1500 µs and released LOW on only the selected signal. It does not establish physical maximum angle or validate unprovided wider endpoints.

The [verification record](verification/2026-10-19-unloaded-servo.md) records commands and limits of the software evidence. To support wider endpoints later, first establish the actual manufacturer pulse specification, then change and re-verify the firmware, console and Servo attachment envelope together. Normal assembled-leg limits and the persistence schema must not change as a side effect.

## Servo identification

The owner reports SG90 (9 g) and MG90D motors. Exact manufacturers, per-pin model assignments and full-travel specifications are not recorded. No MG90S substitution is assumed. Both models use the same explicit individual-test controls; there are no invented model-specific full-range presets. Manufacturer-page access was denied by the environment proxy, and the owner subsequently chose the existing defaults instead of a wider range.
