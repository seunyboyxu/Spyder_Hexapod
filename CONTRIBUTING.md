# Continuing Spyder

Read [the contributor handover](docs/handover.md), [current state](docs/project-state.md) and [project plan](docs/project-plan.md). Use the existing checkout/branch and preserve local changes. Keep the four-leg Uno milestone separate from future six-leg firmware.

For software changes, run the affected regression first, then the [development checks](docs/development.md). Firmware and browser protocol changes must be coordinated; retain detached/held startup, limits, heartbeat hold, explicit arming, urgent STOP/Release and stale-response protection. Build/test commands must not automatically upload or actuate hardware.

For CAD changes, preserve editable sources and print exports together. Follow the [CAD editing guide](cad/README.md), update the part map/manifests and state which physical fit checks were actually performed. Do not overwrite the selected print files with a historical revision merely because it has a similar filename.

For physical work, use [bring-up](docs/bring-up.md), fill a [calibration log](docs/templates/calibration-log.md) and export the real profile. Distinguish measured outcomes, simulation results, historical claims and planned work. Redact credentials or unrelated personal information from new logs before committing them.
