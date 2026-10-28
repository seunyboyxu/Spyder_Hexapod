# CAD recovery notes

These notes preserve mechanical recovery context from the original16 October2026 handover without retaining its superseded electronics/software instructions. The original complete document and older firmware remain in repository history.

The owner's selected print set consists of seven STLs and two Fusion archives from commit dc7ff4e. Exact files, component correspondence and checksums are in the [CAD guide](../../cad/README.md), [manifest](../../cad/asset-manifest.json) and [part table](../../cad/parts.csv). No new geometry/editability or powered-fit validation was performed during repository organization.

The backing-pivot family traces to upstream `models/Insert_body.stl`, distinct from `models/Body_insert.stl` (the old chassis). The provided grounded print has different bytes from the pinned upstream mesh, so exact equivalence is not claimed. The prior package `SG90-missing-backing-pivot/` or `SG90-missing-backing-pivot.zip` reportedly contains an editable STEP; it was not supplied here. Recover and compare it before treating it as the source for the selected print. A reconstructed mesh conversion would be a new source, not recovery of the original.

The complete Fusion assembly predates a later v3 fit trial. The historical report describes a0.50mm backing plate consuming0.20mm nominal ear clearance, leaving0.30mm interference in that v3 stack. This is not a measured defect in every selected v2 part. Fit packages and missing-pivot source still require recovery, inspection and physical checks. Fixed screw mounts are the active design; historical dovetail features do not change that decision.

The owner's [Windows recovery folder](../handover.md#recovery-folder-supplied-by-the-owner) is not accessible in this cloud workspace. Do not claim its contents or inaccessible shared-chat attachments are incorporated. Preserve [original source attribution](upstream.md) when recovering/editing models.
