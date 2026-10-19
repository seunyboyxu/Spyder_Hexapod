# Connection diagnostic patch — 19 October 2026

The owner reported **Connection cancelled** while connecting and confirmed successful upload of revision 3. The physical Uno/Windows serial stream is not available in the cloud, so the original device-side failure is not established by the screenshot.

A deterministic reproduction showed that a failure during the 1800 ms reset wait could be hidden: receive() or the serial error callback first invalidates the session with its useful error; connect() then detects the changed generation, throws a generic cancellation and faults again, overwriting the displayed reason. An old BOOT revision and a serial read error both demonstrate this path. These are tested examples, not a claim that the owner's uploaded firmware is old.

The console now retains the original error per connection attempt and avoids faulting a later connection from an older catch. Version, lease, disabled-startup and cancellation checks are unchanged. There is no firmware or motor-control change; SPYDER 3 remains required and no re-upload is needed solely for this browser patch.

Fresh checks: `npm test` **39/39 passed**; `npm run test:browser` **17/17 passed**. The new regression failed with Connection cancelled instead of the expected diagnostic before production edits, then passed. Browser cases use the real serial/session adapter with a fake Web Serial port over the normal reset wait: incompatible boot remains a version error, read failure retains its message, and valid revision 3 boots connect with all outputs detached and hold latched. A separate test verifies a stale failed startup cannot close a newer healthy session. No successful physical Windows/Uno connection is claimed.

Recovery: replace the complete `web/` folder with the patched files, close Serial Monitor/Plotter and other port users, hard-refresh the page with Ctrl+Shift+R and retry with servo power disconnected. If it fails, record the full preserved error. See [bring-up](../bring-up.md#if-connection-fails).
