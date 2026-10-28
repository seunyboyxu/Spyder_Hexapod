# Four-leg group calibration implementation plan

Goal: Extend the existing four-leg BLE sketch so the owner can arm, calibrate and explicitly confirm all four legs with fewer commands.

Scope: four-leg sketch/controller only, related tests, README and a new delivery ZIP. Preserve previous one-leg/Uno candidates, wiring, BLE compatibility and saved settings format. User explicitly requests this extension; implement locally for physical test before publication.

Design: `leg all` broadcasts cal/move/test/zero/flip/check/limits to corresponding joints on all four legs. Validate every recipient before changing any state or output. Keep each leg's own zero, direction and travel limits; reject the whole command if one recipient cannot accept it. Movement starts from a common tick origin and blocks mutations until all legs finish. `cal all` arms all twelve at stored references without requiring previous checks, one every100ms, all outputs initially off. Existing checked `armall` retains its requirements. `check all` explicitly confirms all twelve only if each is attached, stationary and at its own reference. Checks remain human confirmations, never sensor readings. STOP/OFF/disconnect cancel all stages and movements. No automatic motion sequence or inferred mirroring.

- [x] Add failing public-controller tests for batch calibration, per-leg settings, atomic rejection, unchecked staged arming, explicit check-all and cancellation.
- [x] Implement side-effect-free command validation and coordinator broadcast/staging.
- [x] Exercise the workflow through the BLE sketch, including notification loss during unchecked arming.
- [x] Update help/README with a short bulk-calibration walkthrough and retained power constraints.
- [x] Run sanitizer suite, BLE compatibility suite, project tests and ESP32 builds; review the change.
- [x] Package complete updated sketch separately with fresh verification and checksums; do not push/upload.
