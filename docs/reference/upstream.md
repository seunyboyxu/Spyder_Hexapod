# Original GitHub and source history

## Original project

- Repository: [hpsaturn/quadpod](https://github.com/hpsaturn/quadpod).
- Reviewed baseline: [`e4a7f0efd0447abaa603074676545080f0f3187b`](https://github.com/hpsaturn/quadpod/tree/e4a7f0efd0447abaa603074676545080f0f3187b), pinned by the original handover and fetched read-only again during organization.
- [README at that revision](https://github.com/hpsaturn/quadpod/blob/e4a7f0efd0447abaa603074676545080f0f3187b/README.md), [firmware](https://github.com/hpsaturn/quadpod/blob/e4a7f0efd0447abaa603074676545080f0f3187b/src/servos.cpp), [models](https://github.com/hpsaturn/quadpod/tree/e4a7f0efd0447abaa603074676545080f0f3187b/models).

Quadpod is a four-leg, twelve-servo ESP32 project using a PCA9685 and Bluetooth Classic. Its firmware has different channel order, leg identities, startup movement and quadruped gait assumptions. It is a historical/reference baseline, not the firmware to flash onto this Uno build.

The Spyder Uno calibration sources were implemented separately in this thread; the old walking/IK implementation is not copied into this calibration controller. The present repository is [seunyboyxu/Spyder_Hexapod](https://github.com/seunyboyxu/Spyder_Hexapod).

The upstream README attributes model work to [Santiago Vanegas](https://www.thingiverse.com/alfazoom/designs) and references [the original 12DOF design](https://www.thingiverse.com/thing:3122758) and [its improved models](https://www.thingiverse.com/thing:4644707). Its `models/README.md` also identifies the original design. Preserve these origins when editing/redistributing derived models.

## Licenses and file provenance

Upstream contains a GPL version 3 license document. Its exact pinned text is preserved as [quadpod-GPL-3.0.txt](licenses/quadpod-GPL-3.0.txt). This reference does not silently assign a new global license to the owner's files or assert independently checked Thingiverse model terms. The owner has not selected a separate repository-wide license for newly authored code/docs; retain upstream notices and establish the applicable terms before a separately licensed redistribution.

The owner's seven selected STL files and two Fusion files came from commit [`dc7ff4e`](https://github.com/seunyboyxu/Spyder_Hexapod/commit/dc7ff4e), “All Spyder Parts.” They were moved unchanged into `cad/`; original paths, hashes and component correspondence are recorded in the [CAD guide](../../cad/README.md) and [manifest](../../cad/asset-manifest.json).

`models/Insert_body.stl` is the upstream backing-insert family. It is distinct from `models/Body_insert.stl`, the old chassis. The provided grounded print's hash differs from the upstream file, so no exact-byte equivalence is claimed. The archived handover documents its export and fit caveats.

## Historical sources and missing material

The [16 October original handover](../archive/2026-10-16-SPYDER-CODEX-HANDOVER.md) is retained verbatim, including its original Windows paths and unimplemented six-leg proposal. Current decisions in [project-state.md](../project-state.md) take precedence. Its old files/BOM/fit packages are recovery references, not files claimed to exist here.

The additional [shared conversation](https://chatgpt.com/s/cx_6ad506e1089881918fe6e9d6769453ea) could not be fetched: the environment proxy denied access on 18 October 2026. Its contents and attachments have not been inspected. Record recovered decisions and CAD files explicitly rather than treating an inaccessible link as evidence.
