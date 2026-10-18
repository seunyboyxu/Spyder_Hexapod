# Contributor handover

Start with [current state](project-state.md), then [project plan](project-plan.md). These describe the current four-leg Uno milestone; the [original CAD handover](archive/2026-10-16-SPYDER-CODEX-HANDOVER.md) is historical and contains superseded ESP32-first instructions.

## First session

1. Inspect the current branch and working tree; preserve changes. Read [hardware/bring-up](bring-up.md) before attaching power.
2. Explore the browser demo with no hardware: from the repository root run `python3 -m http.server 8000 --bind 127.0.0.1 --directory web`, open `http://localhost:8000` in desktop Chrome/Edge, then select **Try demo**. Node/npm is needed for development tests, not to serve the console.
3. Run [development checks](development.md). Firmware and UI must both use `SPYDER 2`; JSON/EEPROM stay version 1. Do not automatically upload firmware or move motors as a test step.
4. Find the selected [STLs and native Fusion sources](../cad/README.md). Use the part map; six-leg CAD is not evidence of fitted six-leg electronics or functioning firmware.
5. Take the next incomplete item in the [project plan](project-plan.md). The immediate hardware prerequisites are external servo power and one retained, freely moving joint.

## Where to work

| Area | Entry point |
|---|---|
| Arduino sketch and output adapter | [spyder_uno.ino](../firmware/spyder_uno/spyder_uno.ino) |
| Pulse limits, movement, hold/watchdog and pose | [Control.cpp](../firmware/spyder_uno/Control.cpp) |
| Commands/readback | [Protocol.cpp](../firmware/spyder_uno/Protocol.cpp), [protocol reference](protocol.md) |
| EEPROM validity/persistence | [Storage.cpp](../firmware/spyder_uno/Storage.cpp) |
| Serial ownership, queue and urgent controls | [session.js](../web/session.js) |
| UI / demo | [app.js](../web/app.js), [demo.js](../web/demo.js) |
| Native / JS / browser / MCU regressions | [tests](../tests/), [development guide](development.md) |
| Physical wiring, arming, calibration and standing sequence | [bring-up.md](bring-up.md) |
| Measured calibration record to fill in | [template](templates/calibration-log.md); export actual JSON from the console |
| Print/native correspondence and recovery gaps | [CAD guide](../cad/README.md), [parts.csv](../cad/parts.csv) |
| Original GitHub and licensing provenance | [upstream reference](reference/upstream.md) |

## Rules that prevent repeating earlier mistakes

Retain D2–D13 and the owner's power-off procedure for connection/reset/upload. Do not move signals to D0/D1 or silently substitute A0. Do not power twelve servos through Uno USB. A Sensor Shield V5 distributes connectors; it is not a high-current supply or feedback sensor.

Startup/reset must remain detached and held. PING cannot resume a STOP. STOP holds command signals; Release drives detached outputs LOW and can let the robot drop. First ARM can jump because actual shaft angle is unknown. ±5 changes the outstanding target, and other armed joints can finish their own transitions.

Keep firmware/console revisions together. Export actual calibration before changes, document its physical leg labels, and save explicitly while released. Do not invent standing angles or mark simulated pulses as measured joint positions.

## Sources and unresolved access

This repository consolidates the current conversation, the original uploaded handover and the owner's GitHub CAD upload. The additional [shared conversation](https://chatgpt.com/s/cx_6ad506e1089881918fe6e9d6769453ea) returned a proxy access denial on 18 October 2026 and was not read. Its contents have not been invented or claimed as incorporated. Add any extra decisions/files when its text or exports are available.

The original handover references Windows-local STEP/Fusion/fit/BOM packages not present in this checkout. Their names and recovery locations remain in that archived source; the [CAD guide](../cad/README.md) explains the important missing editable parts. Archive claims about inspection/fit refer to their original date and are not fresh physical validation.

## Recovery folder supplied by the owner

The owner identified this additional project folder on 18 October:

```text
C:\Users\seun_\OneDrive - The University of Manchester\Robotics Scoiety\FInalnd Hackathon
```

This is a Windows-local/OneDrive-synced path, not a mounted path in the cloud checkout. Its contents have not been read. Transfer a ZIP of the relevant project CAD, STEP/Fusion, fit notes and BOM files into the workspace to reconcile it with the manifest; do not assume it contains a particular missing file until inspected. The spelling/capitalization above is preserved exactly as supplied.
