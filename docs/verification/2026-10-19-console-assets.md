# Console 3.2 browser asset delivery — 19 October 2026

The owner confirmed revision 3 was uploaded, then supplied a screenshot with the unloaded-servo panel and an error asking for **revision 2**. That wording comes from the older browser protocol parser. Both the repository and the preceding connection-fix download contain a parser requiring SPYDER 3. The screenshot therefore establishes that older browser code is running; it cannot distinguish a cached module from files served from an older/mixed folder.

Console 3.2 gives the entry module, internal imports and stylesheet matching `?v=3.2` URLs, and displays **Console 3.2 · Firmware 3**. `Start-Console.bat` serves its adjacent `web/` folder by absolute path on port 8033. A complete console-only package extracted into a fresh folder avoids merging old files and using the previous port-8000 server. Keep the already uploaded revision 3 sketch: this release changes no firmware or movement behavior.

## Fresh checks

| Check | Result |
|---|---|
| Obsolete unversioned-module regression | Failed before the asset change; passed afterwards. The browser refuses deliberately broken unversioned modules, connects to the demo and centers the selected unloaded servo at 1500 µs through the versioned modules. |
| `npm test` | 39 passed, none skipped, including actual C++ firmware/Web Serial integration and connection startup diagnostics. |
| `npm run test:browser` | 18 Chromium tests passed, including incompatible boot, serial read failure, healthy revision 3 startup, unloaded controls and the obsolete-module case. |

Windows Command Prompt and physical USB/servo behavior are not exercised by Linux browser tests. The revision label identifies the downloaded console's expected protocol; it does not independently identify connected firmware. A real connection still requires its handshake. Keep servo power off during connection/reset and close other applications holding the serial port.
