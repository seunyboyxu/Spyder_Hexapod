# Project plan

Updated **18 October 2026**. This is the continuation roadmap; the [16 October software implementation plan](superpowers/plans/2026-10-16-uno-calibration.md) is a historical design record. All remaining task owners are unassigned.

## Milestone 1 — retain a reproducible calibration baseline

- [x] Implement revision 2 Uno firmware, Web Serial console, demo and persistence.
- [x] Reproduce/fix release waveform, urgent-control, timing, heartbeat and initial-pulse issues.
- [x] Retain native, integration, browser and actual-AVR waveform regressions.
- [x] Organize the owner's STL/Fusion files with provenance and a part map.
- [ ] Reconcile the files from the owner's [Windows recovery folder](handover.md#recovery-folder-supplied-by-the-owner) once transferred. The owner clarified the additional conversation mainly contained files; both that link and the local folder are inaccessible from this cloud environment.

Acceptance: a new contributor can clone the repository, run the demo/checks and identify the exact hardware/software revision without the chat. Refer to the [handover verification](verification/2026-10-18-handover.md) for executed checks; publication is recorded by Git history.

## Milestone 2 — one reliable, retained joint

- [ ] Record the actual SG90 version, shield revision and external supply specification. Confirm the shield rail isolation and common ground before applying servo power.
- [ ] Identify which hip/mount revision is physically fitted. Inspect horn-to-servo and horn-to-part attachment, centre screw, backing pivot and axial retention. Resolve binding/play with actual dimensions; do not force the known v3 stack interference.
- [ ] With the body supported, follow [bring-up](bring-up.md): one unloaded servo, then one retained joint. Keep servo power off through connection/reset/upload and begin within a mechanically checked range.
- [ ] Log signal pin, safe initial pulse, endpoints, direction and supply measurements. Confirm STOP/Release/reset behavior on the actual board, including the retained D13 procedure.

Acceptance: repeatable deliberate movement of that joint without unwanted resets, binding, horn slip or supply dips, with its physical limits and retention recorded. Electrical limits alone are insufficient.

## Milestone 3 — four-leg calibration and supported standing

- [ ] Mark front and the four occupied chassis stations; label each L0–L3 wiring group and joint axis.
- [ ] Repeat the joint trial for all twelve motors, then one supported leg at a time. Check wire/adjacent-part clearance and load response.
- [ ] Export the measured calibration JSON, Save to Uno while released, and verify deliberate re-arm after reset. Do not publish a generic profile as if it were mechanically validated.
- [ ] Adjust a supported stance with a broad support polygon around the actual centre of mass. Add suitable foot grip and avoid sideways foot forces.
- [ ] Capture standing only when all twelve joints are checked, attached and stationary. Transfer load gradually and record a defined no-slip standing trial with voltage/current and surface details.

Acceptance: a reproducible physical four-leg standing demonstration and retained calibration/test log. No claim of automatic balance, walking or universally safe angles.

## Milestone 4 — complete editable mechanical sources

- [ ] Recover the `SG90-missing-backing-pivot` STEP package and the later v3 fit CAD/issue notes from the original machine or conversation attachments.
- [ ] Open both provided Fusion files in Fusion 360, confirm part correspondence by geometry/re-export, and record what is directly editable versus mesh-derived.
- [ ] Revise the actual retained joint stack from measured servos/horns/pivots. Update the complete assembly with the successful parts and fitted electronics/fastener envelopes.
- [ ] Commit edited native/STEP and print exports together; update the manifest and one-part fit-trial record before multiplying prints.

Acceptance: every active print has an identified editable source or an explicitly documented reconstruction, with revision and fit evidence. Keep historical alternatives labelled.

## Milestone 5 — six-leg conversion and gait

This is a separate design and implementation project. Do not just change `JointCount` from 12 to 18: the current Uno/Servo setup is a twelve-output calibration implementation.

- [ ] Confirm the six-leg controller/output architecture. The earlier ESP32 + two PCA9685 boards at 0x40/0x41 remains a candidate, with explicit logic levels, output mapping and whole-robot power budget.
- [ ] Measure link axes/lengths and hip origins; define six local frames, station order and joint signs. Do not treat STL bounding boxes as link lengths.
- [ ] Add eighteen-joint calibration, bounded numeric/command validation, persistent settings and nonblocking STOP/watchdog behavior with tests before output enable.
- [ ] Test all individual joints/legs and a supported six-leg stance. Verify reachability, full sweep, cable clearance, torque and support geometry.
- [ ] Implement a slow one-leg-at-a-time wave gait; consider tripod gait only after stable support/load evidence. Keep toolchain upgrades separate so failures are attributable.

Acceptance: recorded successful build, staged hardware tests and a slow stable gait on the actual six-leg robot. Cosmetic lid/society artwork follows reliable operation; exact artwork has not been supplied.
