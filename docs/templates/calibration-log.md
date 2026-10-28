# ESP32 physical calibration log — copy before filling

This blank template contains no validated angles. Keep a full `show all` (four-leg) or `show` (one-leg) transcript with the completed log. Defaults and checked flags are not measurements.

- Date/operator:
- Sketch folder, displayed version and Git commit:
- ESP32 board model / ESP32 core version / PCA9685 board revision:
- Phone/app/version, TX notifications and reply checks:
- Servo model per joint:
- External source/converter voltage/current ratings, distribution/fusing:
- Measured rail voltage/current at rest, arming and supported motion:
- Verified SDA21/SCL22/OE23, common ground, OE reset voltage:
- Installed CAD/horn/retention revision and measured link lengths:
- Body front and actual leg corners:
- Servo power off during wiring/upload/reset:

| Leg | Joint | PCA channel | Corner / servo model | Reference deg | Min deg | Max deg | Direction ±1 | Alignment/free travel/direction observed |
|---|---|---|---|---|---|---|---|---|
| 1 | Hip0 | 0 | | | | | | |
| 1 | Knee1 | 1 | | | | | | |
| 1 | Foot2 | 2 | | | | | | |
| 2 | Hip0 | 3 | | | | | | |
| 2 | Knee1 | 4 | | | | | | |
| 2 | Foot2 | 5 | | | | | | |
| 3 | Hip0 | 6 | | | | | | |
| 3 | Knee1 | 7 | | | | | | |
| 3 | Foot2 | 8 | | | | | | |
| 4 | Hip0 | 9 | | | | | | |
| 4 | Knee1 | 10 | | | | | | |
| 4 | Foot2 | 11 | | | | | | |

For the one-leg sketches fill only leg1/channels0–2. Leg angle scale is90°→1472µs; the separate unloaded motor sketch uses90°→1500µs. Record measured pulse/shaft data separately; do not equate nominal commands with physical angles.

Record supported cal all/armall, individual corrections, check/save/load, stand, straight, one finite step, STOP hold, OFF release, BLE disconnect/unsubscribe and reconnect-off behavior. State support used, test duration, supply sag, binding/buzzing/slipping and untested items. Four-leg simultaneous step is not balanced walking. Attach status transcript and relevant photos/video; ESP32 Preferences records are not automatically exported as browser JSON.
