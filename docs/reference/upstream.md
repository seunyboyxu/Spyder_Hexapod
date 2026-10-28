# Original GitHub and source history

Original example project: **[hpsaturn/quadpod](https://github.com/hpsaturn/quadpod)**. Reviewed baseline: [e4a7f0efd0447abaa603074676545080f0f3187b](https://github.com/hpsaturn/quadpod/tree/e4a7f0efd0447abaa603074676545080f0f3187b), including its [README](https://github.com/hpsaturn/quadpod/blob/e4a7f0efd0447abaa603074676545080f0f3187b/README.md), [servo firmware](https://github.com/hpsaturn/quadpod/blob/e4a7f0efd0447abaa603074676545080f0f3187b/src/servos.cpp) and [models](https://github.com/hpsaturn/quadpod/tree/e4a7f0efd0447abaa603074676545080f0f3187b/models).

Quadpod uses an ESP32, PCA9685, twelve servos and Bluetooth Classic. Its channel ordering, startup movement, leg identities and quadruped gait assumptions differ from Spyder's current wiring and explicit calibration. It is reference material, not a drop-in sketch or validated six-leg gait. The current self-contained test controllers are in this repository's [firmware index](../../firmware/README.md).

Current project: [seunyboyxu/Spyder_Hexapod](https://github.com/seunyboyxu/Spyder_Hexapod). Legacy firmware and browser tools were removed from the active tree at the owner's request on28 October2026; their earlier versions remain in Git history. CAD, attribution and current ESP32 tests are retained.

## Licenses and provenance

Upstream contains a GPL version3 license; the exact pinned text is retained in [quadpod-GPL-3.0.txt](licenses/quadpod-GPL-3.0.txt). This does not silently assign a new global license to owner-authored files or assert independently checked model terms. The owner has not selected a separate repository-wide license for newly authored code/docs. Preserve upstream notices and establish applicable terms before separately licensed redistribution.

Upstream attributes model work to [Santiago Vanegas](https://www.thingiverse.com/alfazoom/designs) and references [the original12DOF design](https://www.thingiverse.com/thing:3122758) and [improved models](https://www.thingiverse.com/thing:4644707). Retain these origins for derived models.

The owner's seven STLs and two Fusion files came from [dc7ff4e, All Spyder Parts](https://github.com/seunyboyxu/Spyder_Hexapod/commit/dc7ff4e). Files were moved unchanged under cad; original paths, hashes and component correspondence are recorded in the [CAD guide](../../cad/README.md) and [asset manifest](../../cad/asset-manifest.json). [CAD recovery notes](cad-history.md) preserve missing-source and fit-package context.

The additional [shared conversation](https://chatgpt.com/s/cx_6ad506e1089881918fe6e9d6769453ea) could not be fetched on18 October2026. Its contents/attachments have not been inspected. Add recovered decisions/files explicitly with provenance rather than treating the inaccessible link as evidence.
