# Continuing Spyder

Start with [current state](docs/project-state.md), [handover](docs/handover.md) and the [firmware guide](docs/firmware-guide.md). All active firmware targets the original ESP32 and PCA9685. Preserve each sketch's independent Arduino IDE folder and use a feature branch and pull request.

Run the affected suite, then `bash scripts/test-esp32.sh` and relevant [ESP32 builds](docs/development.md). Keep output-off startup, explicit arming, per-joint limits, atomic group validation, bounded inputs and global STOP/OFF/disconnect handling. Do not upload firmware or actuate hardware from build/test commands. Physical claims need recorded measurements or owner observations.

Keep selected CAD exports and native sources together. Follow the [CAD guide](cad/README.md), retain provenance, and update manifests only when the assets actually change. Record physical calibration using the [log template](docs/templates/calibration-log.md). Do not commit toolchains, credentials, machine-specific configuration, generated builds or delivery ZIPs.
