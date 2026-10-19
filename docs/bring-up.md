# Supported bring-up: Uno + Sensor Shield V5

## 1. Inspect the assembly and power path

Use the existing fixed screw mounts and SG90 servos. The handover records an earlier missing passive backing pivot, uncertain horn retention and nominal 0.30 mm ear/keeper interference in one v3 arrangement. Check the **current** assembly: retained horn, matching centre screw, horn-to-leg attachment, free passive pivot and secure servo casing. Code cannot correct loose or binding joints. Do not force an interfering keeper into place.

Put the body on a stable support that keeps its weight off the legs. Leave room for small movements and keep wires clear. Identify which four of the six stations are occupied and a body front; enter them in the interface's physical labels.

Signal map: L0/J0–J2 = D2–D4; L1 = D5–D7; L2 = D8–D10; L3 = D11–D13. Preserve proximal-to-distal wire order. These are Uno pins, not the upstream PCA9685 channel table. Keep D0/RX and D1/TX unused by servos. Verify plugs against the shield's **S/V/G** labels and the actual servo lead identification; do not infer orientation from colour alone.

Use an external regulated supply at a voltage supported by your actual SG90s, with adequate combined startup/load current and rated distribution, wiring and protection. Do **not** power twelve servos from Uno USB, its regulator, or a rectangular 9 V battery. Share supply and Uno grounds. Before energising, verify your particular shield's external terminal, power rail and selector/jumper using its documentation and continuity checks: clone revisions differ, and the external rail must not backfeed USB/Uno power. The V5 name alone does not establish isolation. Keep an accessible servo-power disconnect. Current capacity, voltage under load and rail adequacy need real measurements.

**Current mapping remains D2–D13. D13 is also the Uno LED pin:** some bootloaders toggle it during connection/reset, before this sketch can drive it LOW. Keep servo power off while connecting, resetting or uploading. Sketch-level release fixes cannot prevent bootloader pulses or power-on twitch. You chose to retain D13 and switch servo power off during connection/reset. No plug move or A0 remap is required.

## 2. Upload with servo power off

1. Open `firmware/spyder_uno/spyder_uno.ino` in Arduino IDE, keeping its `.h` and `.cpp` files in that directory.
2. Install Arduino AVR Boards and **Servo 1.2.2**. EEPROM is included in the AVR core.
3. Select Arduino Uno and your USB port. Upload the revision 3 sketch with servo power disconnected. Replace all browser-console files with the same release and reload the page; mixed revision 1/2 firmware with the revision 3 console is rejected. EEPROM/profile format remains version 1.
4. Close Serial Monitor/Plotter before the browser connects. For a manual serial check, use 115200 baud and newline termination: `1 HELLO` returns `SPYDER 3` and `ACK 1`. `2 STATUS` shows twelve detached joints after reset. Never send ARM as a connectivity check.

Optional PlatformIO upload from your own USB-connected computer after a successful build: `pio run -e uno -t upload`. No upload has been attempted from the cloud.

## 3. Open the console on the connected computer

From the downloaded repository root run `python -m http.server 8000 --bind 127.0.0.1 --directory web` (or `python3`). Visit `http://localhost:8000` in desktop Chrome or Edge. Click **Connect Uno** and select its serial device. Opening USB commonly resets the board; the interface waits before handshaking and never replays old moves. If the port is busy, close other serial applications. Mobile screenshots demonstrate layout only; mobile USB/Web Serial support is not promised.

After connecting with servo power off, confirm detached state and hold. Energise the correctly isolated external servo supply only with the body supported. If resetting/reconnecting is needed, turn servo power off first.

**Try demo** is only a simulation. It does not establish physical wiring, power, positions or load capacity.

For motors removed from their legs, use [Unloaded servo setup](unloaded-servo-calibration.md) to center and sweep one output. This preserves the leg limits but clears its travel check and standing record in RAM. Refit, support the body, then follow the assembled calibration below.

## 4. Calibrate one servo, then one leg

The first attachment can jump to its selected pulse: SG90s do not report their actual angle. A slow transition cannot fix this. Begin with one unloaded or mechanically checked joint and the supported body. Do not assume 1500 µs is a safe assembled neutral. If uncertain, establish initial alignment with the horn/linkage unloaded before reconnecting the mechanism.

1. With all outputs released, open that joint's **Limits & calibration**. Defaults are 1400–1600 µs and an initial suggestion of 1500 µs. This is a small electrical range, not a mechanical clearance guarantee. Leave **Travel checked** unticked until tested.
2. Check the supported-body confirmation. Startup has hold latched: use **Resume controls**, then deliberately choose a starting pulse within limits and click **Arm joint** for that joint only. Verify the correct joint responds, with no binding, horn/casing slip or supply dips. If the wrong joint moves, stop and correct the signal map.
3. Use ±5 µs nudges or an explicit target at 50 µs/s or slower. Nudges change the outstanding target, not the present commanded pulse: a minus nudge may still leave that target above the current command. Other armed joints finish their own moves; STOP first if you need all motion held while switching joints. **Nudge direction** reverses the sign for mirrored mechanisms; pulse numbers always remain physical pulse widths.
4. For more travel, support the body and **Release outputs**, then expand that joint's limits slightly. Configuration requires all outputs released. The hard envelope is 1000–2000 µs; stay inside mechanical stops. Never widen limits merely to bypass a rejected target.
5. Re-arm at a known-safe pulse and test the new region. Stop for buzzing, stall, twisting, slip, heat or voltage dips; use the physical power disconnect for electrical/mechanical faults.
6. Release, set verified limits and initial pulse, choose direction, tick **Travel checked**, then **Apply calibration**. Its acknowledged initial pulse becomes the next arming suggestion; inspect it before arming. Changes invalidate any older standing pose. **Save to Uno** while released to retain calibration through resets.

Repeat for the other joints on one leg, then the remaining legs. There is no automatic Arm All or canned 90-degree standing pose.

## 5. Establish a standing pose

After all individual joint/leg checks pass, verify that the four occupied stations form a useful support polygon around the centre of mass, including battery/electronics. Four arbitrary hexagonal stations are not automatically a stable quadruped.

With support carrying the body, arm each joint individually at its verified initial pulse. Adjust toward a broad stance and low body height, avoiding sideways forces that push feet apart. Use a level grippy surface and suitable foot pads. There is no foot-contact, slip or overload sensing.

With all twelve joints calibrated, attached and stationary, use **Capture standing pose** and confirm that the supported stance is physically checked. This records commanded pulses only. Release with the body supported, then **Save to Uno** or export JSON. Re-arm individually before playback. **Move to standing** slowly approaches recorded targets; it does not plan placement or balance.

Transfer a small amount of weight first, checking grip, retention, current and voltage, then increase load only if stable. Keep support/catch in place during early tests. SG90 torque margin and the handover's unresolved power budget require physical evidence. Simulation and compilation do not establish that the robot stands without slipping.

## 6. STOP, release and recovery

- **STOP · hold** cancels targets and latches the last commanded signals. Physical servos may still be catching up; there is no position feedback. Holding torque and power remain.
- **Resume controls** clears the latch without restarting old targets. A reset requires reconnecting and deliberately rearming; an ordinary held STOP can retain attachment.
- **Release outputs** atomically detaches each signal and drives its pin LOW, including when release interrupts an active pulse. It may let the body fall. It does not switch the supply off or guarantee every servo model immediately releases torque.
- STOP and Release interrupt ordinary response waits and cancel queued actions. Controls remain disabled until acknowledgement and fresh readback. Complete serial lines already written remain ordered; blocked USB output or transmitted bytes cannot be recalled. If no confirmation arrives, treat state as unknown.
- Missing heartbeats for 1500 ms latches hold even if no joint is attached. Browser tab suspension can cause this. The browser waits 2500 ms for acknowledgements so a prolonged stalled output stream expires the firmware lease before the browser times out. If the interface reports unknown state, STOP has not been confirmed: use the physical disconnect if needed and support the robot before recovery. Bytes already transmitted cannot be recalled.
- A USB/Uno reset leaves outputs detached with hold latched. Reconnect, inspect state, use Resume controls, and explicitly re-arm; reconnection never restores movement automatically. Fault cleanup cancels rather than drains queued serial writes. If port cleanup itself times out, the UI reports it; unplug/reload before reconnecting.

## 7. Save and import settings

**Save to Uno** writes versioned, checksummed EEPROM with a marker committed last. An interrupted save may discard the previous record; invalid records are rejected on boot and detached defaults are used. This is not a two-bank transactional store.

**Export JSON** includes labels/front, calibration and pose. Labels remain computer/profile metadata, not Uno EEPROM fields. **Choose JSON** validates the complete file. **Apply imported profile** uploads only while released and confirms responses/readback. An imported pose needs explicit confirmation that it came from a physically calibrated supported stance on this robot. Import never arms or moves outputs.

A disconnect during import can leave some settings changed; the interface reports incomplete import rather than claiming rollback. Reconnect and inspect state before retrying. Save explicitly after import to persist it.

## Scope

This is a four-leg bench-testing milestone. No gait, six-leg conversion, automatic balance or measured feedback is included. The selected print meshes and two native Fusion archives are now in [cad/](../cad/README.md). Later fit packages and the backing-pivot editable STEP remain to be recovered. Actual mechanical fit, occupied stations, electrical checks, real USB flashing, servo response and loaded standing remain physical checks.
