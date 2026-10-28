# Project plan

1. **Establish adequate power and reset inhibition.** Replace PP3/3A source, check combined servo starting/stall current and distribution ratings, verify common ground and OE HIGH during reset with motors unpowered. Record measured rail behavior under staged load.
2. **Validate the current ESP32 BLE controller physically.** Confirm app subscription/replies, OFF boot/reconnect, STOP/OFF/disconnect behavior and I2C fault handling with a supported assembly. Use the [bring-up guide](bring-up.md).
3. **Calibrate each joint.** Use four-leg matching-joint commands to reduce repetition, then individual corrections for alignment/direction/limits. Check only observed free motion, save settings and retain a full status log. Verify horns and measured link lengths.
4. **Validate supported poses and finite steps.** Stand, partial straightening and single cycles with external support. Record binding, slipping, supply sag and direction errors. Acceptance is observed repeatability within safe travel, not just Ready messages.
5. **Develop balanced four-leg walking.** Agree body geometry, actual corner map, support/contact assumptions, foot clearance, body shifts and finite creep sequence before changing behavior. Simultaneous step tests are not a gait.
6. **Expand toward six legs after four-leg evidence.** Six three-joint legs require18 channels; a single16-channel PCA9685 cannot serve all18. Select additional output hardware/addressing and power distribution before implementation.
7. **Close CAD gaps.** Recover missing backing-pivot editable source and later fit packages from the owner's local materials, compare with selected prints and record real fit/retention results. See [CAD guide](../cad/README.md).
