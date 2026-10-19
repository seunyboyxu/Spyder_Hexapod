import { Session, SerialTransport } from "./session.js?v=3.2";
import { DemoTransport } from "./demo.js?v=3.2";
import {
  validateProfile,
  BenchMinimumPulse,
  BenchMaximumPulse,
  BenchCenterPulse,
} from "./protocol.js?v=3.2";
const $ = (id) => document.getElementById(id),
  session = new Session();
let busy = false,
  noticeRevision = 0,
  previousStopped,
  imported = null,
  metadata = {
    labels: [
      "Leg 1 / position…",
      "Leg 2 / position…",
      "Leg 3 / position…",
      "Leg 4 / position…",
    ],
    front: "",
  };
function notice(text, error = false) {
  $("notice").textContent = text;
  $("notice").classList.toggle("error", error);
}
function speed() {
  const n = Number($("speed").value);
  if (!Number.isInteger(n) || n < 10 || n > 100)
    throw new Error("Speed must be 10–100 µs/s");
  return n;
}
function supported() {
  if (!$("supported").checked)
    throw new Error("Support the body and confirm joint retention first.");
}
async function action(fn, message) {
  if (busy || session.safetyPending > 0) return;
  const revision = noticeRevision;
  busy = true;
  render();
  try {
    await fn();
    if (message && revision === noticeRevision) notice(message);
  } catch (error) {
    if (revision === noticeRevision) notice(error.message, true);
  } finally {
    busy = false;
    render();
  }
}
function command(verb, args = [], message) {
  return action(() => session.command(verb, args), message);
}
for (let leg = 0; leg < 4; leg++) {
  const card = document.createElement("article");
  card.className = "leg";
  card.innerHTML = `<div class="leg-heading"><span class="leg-number">0${leg + 1}</span><label>PHYSICAL LEG POSITION<input class="leg-label" aria-label="Leg ${leg + 1} position" maxlength="40"></label></div>`;
  card.querySelector("input").value = metadata.labels[leg];
  card
    .querySelector("input")
    .addEventListener("change", (e) => (metadata.labels[leg] = e.target.value));
  for (let joint = 0; joint < 3; joint++) {
    const i = leg * 3 + joint,
      row = document.createElement("section");
    row.className = "joint";
    row.dataset.joint = i;
    row.innerHTML = `<div class="joint-title"><strong class="joint-id">L${leg}/J${joint} <span class="fine">· —</span></strong><span class="joint-state">Detached</span></div><div class="joint-readout"><span>Commanded <b class="commanded">—</b> µs</span><span>Target <b class="target">—</b></span></div><div class="joint-controls"><button class="minus" aria-label="Nudge negative">−5</button><label><span class="fine">Target / starting pulse (µs)</span><input class="pulse-input" type="number" value="1500" min="1400" max="1600" step="1" aria-label="Target pulse"></label><button class="plus" aria-label="Nudge positive">+5</button></div><div class="arm-row"><button class="arm">Arm joint</button><button class="move">Apply target</button></div><details><summary>LIMITS & CALIBRATION</summary><form class="calibration"><div class="calibration-grid"><label>Minimum (µs)<input name="minimum" type="number" value="1400" min="1000" max="1999" required></label><label>Maximum (µs)<input name="maximum" type="number" value="1600" min="1001" max="2000" required></label><label>Initial pulse (µs)<input name="initial" type="number" value="1500" min="1000" max="2000" required></label><label>Nudge direction<select name="direction"><option value="1">+ normal</option><option value="-1">− reversed</option></select></label></div><label class="check"><input name="calibrated" type="checkbox">Travel checked</label><button type="submit">Apply calibration</button></form></details>`;
    const pulse = () => {
      const value = Number(row.querySelector(".pulse-input").value);
      if (!Number.isInteger(value))
        throw new Error("Pulse must be a whole number");
      return value;
    };
    row.querySelector(".arm").onclick = () =>
      action(async () => {
        supported();
        await session.command("ARM", [i, pulse()]);
      }, "Joint armed at the selected pulse. Check free travel before nudging.");
    row.querySelector(".move").onclick = () =>
      action(async () => {
        supported();
        await session.command("MOVE", [i, pulse(), speed()]);
      });
    for (const [name, sign] of [
      ["minus", -1],
      ["plus", 1],
    ])
      row.querySelector("." + name).onclick = () =>
        action(async () => {
          supported();
          const j = session.state.joints[i],
            p = j.target + sign * 5 * j.direction;
          await session.command("MOVE", [i, p, speed()]);
          row.querySelector(".pulse-input").value = p;
        });
    row.querySelector("form").onsubmit = (event) => {
      event.preventDefault();
      const form = new FormData(event.target);
      return action(async () => {
        await session.command("CONFIG", [
          i,
          Number(form.get("minimum")),
          Number(form.get("maximum")),
          Number(form.get("initial")),
          Number(form.get("direction")),
          form.has("calibrated") ? 1 : 0,
        ]);
        row.querySelector(".pulse-input").value =
          session.state.joints[i].initial;
      }, "Calibration applied in RAM. Save to Uno while released to retain it after reset.");
    };
    card.append(row);
  }
  $("legs").append(card);
}
for (let i = 0; i < 12; i++) {
  const option = document.createElement("option");
  option.value = i;
  option.textContent = `L${Math.floor(i / 3)}/J${i % 3} · pin unknown`;
  $("bench-joint").append(option);
}
$("bench-minimum").value = BenchMinimumPulse;
$("bench-minimum").min = BenchMinimumPulse;
$("bench-minimum").max = BenchCenterPulse - 1;
$("bench-maximum").value = BenchMaximumPulse;
$("bench-maximum").min = BenchCenterPulse + 1;
$("bench-maximum").max = BenchMaximumPulse;
$("bench-envelope").textContent =
  `Default test envelope: ${BenchMinimumPulse}–${BenchMaximumPulse} µs. These pulses do not establish maximum angle or full mechanical travel. Speed: 10–100 µs/s.`;
$("bench-center-note").textContent =
  `90° is nominal at ${BenchCenterPulse} µs, not a measured angle. First attachment can jump. Explicit Resume is required before Center; powered servos can retain holding torque after Release.`;
function benchRange() {
  const values = ["bench-minimum", "bench-maximum", "bench-speed"].map((id) =>
    Number($(id).value),
  );
  const [minimum, maximum, rate] = values;
  if (
    !values.every(Number.isInteger) ||
    minimum < BenchMinimumPulse ||
    minimum >= BenchCenterPulse ||
    maximum <= BenchCenterPulse ||
    maximum > BenchMaximumPulse ||
    rate < 10 ||
    rate > 100
  )
    throw new Error(
      `Choose ${BenchMinimumPulse} ≤ minimum < ${BenchCenterPulse} < maximum ≤ ${BenchMaximumPulse} µs, and speed 10–100 µs/s.`,
    );
  return values;
}
function unloaded() {
  if (!$("unloaded").checked)
    throw new Error(
      "Remove the selected servo from the leg and confirm it is unloaded first.",
    );
}
$("bench-center").onclick = () =>
  action(async () => {
    unloaded();
    await session.command("CENTER", [Number($("bench-joint").value)]);
    populateCalibration();
  }, "Center requested. Nominal 90° is a command, not a measured position; keep the servo unloaded.");
$("bench-sweep").onclick = () =>
  action(async () => {
    unloaded();
    await session.command("SWEEP", [
      Number($("bench-joint").value),
      ...benchRange(),
    ]);
  }, "One unloaded sweep started. Watch for binding and keep STOP accessible.");
for (const id of [
  "unloaded",
  "bench-joint",
  "bench-minimum",
  "bench-maximum",
  "bench-speed",
]) {
  $(id).addEventListener("change", render);
  $(id).addEventListener("input", render);
}
function render() {
  const ready = session.ready,
    state = session.state,
    detached = state?.joints.every((j) => !j.attached),
    count = state?.joints.filter((j) => j.attached).length || 0,
    can = ready && !busy && !(session.safetyPending > 0),
    checked = $("supported").checked,
    bench = ready && state.benchJoint >= 0,
    selected = bench ? state.benchJoint : Number($("bench-joint").value),
    selectedJoint = state?.joints[selected];
  let validRange = false;
  try {
    benchRange();
    validRange = true;
  } catch {}
  $("bench-joint").value = selected;
  $("bench-joint").disabled = !can || bench;
  Array.from($("bench-joint").options).forEach((option, i) => {
    const pin = state?.joints[i].pin;
    option.textContent = `L${Math.floor(i / 3)}/J${i % 3} · ${ready ? (pin >= 14 ? "A" + (pin - 14) : "D" + pin) : "pin unknown"}`;
  });
  const sameBench = bench && state.benchJoint === selected;
  $("bench-center").disabled =
    !can ||
    !$("unloaded").checked ||
    state?.stopped ||
    (!detached && !sameBench);
  $("bench-sweep").disabled =
    !can ||
    !$("unloaded").checked ||
    state?.stopped ||
    !sameBench ||
    state.sweepPhase !== 0 ||
    selectedJoint.commanded !== BenchCenterPulse ||
    selectedJoint.target !== BenchCenterPulse ||
    !validRange;
  $("bench-commanded").textContent = ready ? selectedJoint.commanded : "—";
  $("bench-target").textContent = ready ? selectedJoint.target : "—";
  $("bench-status").textContent = !ready
    ? "No device authority"
    : !bench
      ? "Released · select an unloaded servo, Resume, then Center"
      : state.stopped
        ? "Hold · sweep cancelled; Resume, then Center"
        : state.sweepPhase === 1
          ? "Sweeping to minimum · 1 / 3"
          : state.sweepPhase === 2
            ? "Sweeping to maximum · 2 / 3"
            : state.sweepPhase === 3
              ? "Returning to center · 3 / 3"
              : selectedJoint.commanded !== selectedJoint.target
                ? "Recentering · wait before sweeping"
                : selectedJoint.commanded === BenchCenterPulse
                  ? "Centered · nominal 90° · ready for one sweep"
                  : "Holding · Center before a fresh sweep";
  $("connection").textContent = ready
    ? session.transport.demo
      ? "DEMO · simulated robot"
      : "UNO · USB connected"
    : "Disconnected";
  $("connection-dot").classList.toggle("active", ready);
  $("motion-status").textContent = ready
    ? state.stopped
      ? "Hold latched · resume required"
      : count
        ? "Outputs active"
        : "All outputs detached"
    : "No device authority";
  $("output-count").textContent = ready
    ? `${count} / 12 attached`
    : "Device state unknown";
  $("pose-status").textContent = state?.standingValid
    ? "Captured · standing pose available."
    : "No standing pose captured.";
  $("connect").disabled = ready || busy || !!session.transport;
  $("demo").disabled = ready || busy || !!session.transport;
  $("disconnect").disabled =
    !session.transport || busy || session.safetyPending > 0;
  $("stop").disabled = !ready;
  $("release").disabled = !ready;
  $("resume").disabled = !can || !state?.stopped;
  const all = state?.joints.every((j) => j.attached && j.calibrated),
    still = state?.joints.every((j) => j.commanded === j.target);
  $("capture").disabled =
    !can || bench || !all || !still || state.stopped || !checked;
  $("stand").disabled =
    !can || bench || !all || !state.standingValid || state.stopped || !checked;
  for (const id of ["save", "load"]) $(id).disabled = !can || !detached;
  $("export").disabled = !can || bench;
  $("apply-profile").disabled = !can || !detached || !imported;
  document.querySelectorAll(".joint").forEach((row, i) => {
    const j = state?.joints[i];
    row.querySelector(".joint-state").textContent = ready
      ? j?.attached
        ? "Holding signal"
        : "Detached"
      : "Unknown / disconnected";
    row
      .querySelector(".joint-state")
      .classList.toggle("armed", ready && !!j?.attached);
    row.querySelector(".joint-id .fine").textContent = ready
      ? `· ${j.pin >= 14 ? "A" + (j.pin - 14) : "D" + j.pin}`
      : "· —";
    row.querySelector(".commanded").textContent = ready ? j.commanded : "—";
    row.querySelector(".target").textContent = ready ? j.target : "—";
    row.querySelector(".arm").disabled =
      !can || bench || !checked || !!j?.attached || !!state?.stopped;
    for (const name of ["minus", "plus", "move"])
      row.querySelector("." + name).disabled =
        !can || bench || !checked || !j?.attached || !!state?.stopped;
    row.querySelector("form button").disabled = !can || !detached;
    row.querySelectorAll("input, select").forEach((input) => {
      input.disabled = !!bench;
    });
    if (bench)
      row.querySelector("form").elements.calibrated.checked = j.calibrated;
    if (j) {
      const input = row.querySelector(".pulse-input");
      input.min = j.minimum;
      input.max = j.maximum;
    }
  });
}
function populateCalibration() {
  session.state?.joints.forEach((j, i) => {
    const row = document.querySelector(`[data-joint="${i}"]`),
      form = row.querySelector("form");
    for (const field of ["minimum", "maximum", "initial", "direction"])
      form.elements[field].value = j[field];
    form.elements.calibrated.checked = j.calibrated;
    row.querySelector(".pulse-input").value = j.initial;
  });
}
session.addEventListener("status", () => {
  if (
    previousStopped === false &&
    session.state?.stopped &&
    !(session.safetyPending > 0)
  ) {
    noticeRevision++;
    notice(
      "Device hold latched. Communication timeout or STOP can trigger a hold; outstanding movement targets were cancelled. Check support, then Resume explicitly.",
      true,
    );
  }
  previousStopped = session.state?.stopped;
  render();
});
session.addEventListener("activity", render);
session.addEventListener("fault", (e) => {
  previousStopped = undefined;
  if (e.detail !== "Disconnected") noticeRevision++;
  notice(
    e.detail + " · Disconnect, then reconnect before further commands.",
    true,
  );
  render();
});
$("supported").onchange = render;
$("front").onchange = (e) => (metadata.front = e.target.value);
$("connect").onclick = () =>
  action(async () => {
    if (!navigator.serial)
      throw new Error(
        "Web Serial requires desktop Chrome or Edge on localhost. Use Try demo to explore.",
      );
    const port = await navigator.serial.requestPort();
    await session.connect(new SerialTransport(port));
    populateCalibration();
  }, "Uno synchronised; outputs are never automatically armed. Check device state before proceeding.");
$("demo").onclick = () =>
  action(async () => {
    await session.connect(new DemoTransport());
    populateCalibration();
  }, "DEMO: simulated signals only. No robot is connected or physically tested.");
$("disconnect").onclick = () =>
  action(async () => {
    if (session.ready) await session.stop().catch(() => {});
    await session.disconnect();
  }, "Disconnected. Powered servos may retain holding torque; use the physical power switch when needed.");
$("stop").onclick = async () => {
  const revision = ++noticeRevision;
  try {
    await session.stop();
    if (revision === noticeRevision)
      notice(
        "STOP acknowledged: last commanded outputs held. Resume controls explicitly.",
      );
  } catch (e) {
    if (revision === noticeRevision) notice(e.message, true);
  }
  render();
};
$("release").onclick = async () => {
  if (
    !confirm(
      "Release all servo signals? The robot may drop. Support the body first.",
    )
  )
    return;
  const revision = ++noticeRevision;
  try {
    await session.release();
    if (revision === noticeRevision)
      notice("Outputs released. Servo power is still present.");
  } catch (error) {
    if (revision === noticeRevision) notice(error.message, true);
  }
  render();
};
$("resume").onclick = () =>
  command(
    "RESUME",
    [],
    "Controls resumed. Cancelled movement targets will not restart.",
  );
$("capture").onclick = () => {
  if (
    confirm(
      "Is the body supported with a physically checked standing stance? This records commanded pulses, not measured positions.",
    )
  )
    command(
      "CAPTURE",
      [],
      "Standing pose captured in RAM. Release and Save to Uno to retain it.",
    );
};
$("stand").onclick = () =>
  action(async () => {
    supported();
    await session.command("STAND", [speed()]);
  }, "Moving toward your calibrated standing targets. Watch the feet and keep STOP accessible.");
$("save").onclick = () =>
  command("SAVE", [], "Saved calibration and standing pose to Uno EEPROM.");
$("load").onclick = () =>
  action(async () => {
    await session.command("LOAD");
    populateCalibration();
  }, "Loaded device settings; outputs remain detached.");
$("export").onclick = () => {
  try {
    const s = session.state,
      p = validateProfile({
        version: 1,
        joints: s.joints,
        standing: s.standingValid ? s.joints.map((j) => j.standing) : null,
        ...metadata,
      });
    const url = URL.createObjectURL(
        new Blob([JSON.stringify(p, null, 2)], { type: "application/json" }),
      ),
      a = document.createElement("a");
    a.href = url;
    a.download = "spyder-calibration.json";
    a.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  } catch (e) {
    notice(e.message, true);
  }
};
$("import-file").onchange = async (e) => {
  imported = null;
  try {
    const file = e.target.files[0];
    if (!file) return;
    if (file.size > 20000) throw new Error("Profile too large (20 KB maximum)");
    imported = validateProfile(JSON.parse(await file.text()));
    notice(
      "Profile validated locally. Apply while released to upload it; no movement will start.",
    );
  } catch (e) {
    notice(e.message, true);
  }
  render();
};
$("apply-profile").onclick = () =>
  action(async () => {
    const p = validateProfile(imported);
    if (
      p.standing &&
      !confirm(
        "Confirm this imported pose came from a physically calibrated, supported stance on this robot. Importing does not move the servos.",
      )
    )
      return;
    try {
      for (let i = 0; i < 12; i++) {
        const j = p.joints[i];
        await session.command("CONFIG", [
          i,
          j.minimum,
          j.maximum,
          j.initial,
          j.direction,
          +j.calibrated,
        ]);
      }
      if (p.standing) await session.command("POSE", p.standing);
      metadata = { labels: p.labels, front: p.front };
      document
        .querySelectorAll(".leg-label")
        .forEach((el, i) => (el.value = p.labels[i]));
      $("front").value = p.front;
      populateCalibration();
      notice(
        "Profile uploaded and read back. Save to Uno to persist it; outputs remain detached.",
      );
    } catch (e) {
      throw new Error(
        "Import incomplete; some settings may have changed. Read back before retrying. " +
          e.message,
      );
    }
  });
const powerWarning = document.createElement("strong");
powerWarning.textContent = "USB-only servo power is unsuitable.";
document
  .querySelector("aside .panel p:not(.eyebrow)")
  .prepend(powerWarning, " ");
const jointWorkspace = document.querySelector(".joint-workspace");
jointWorkspace.querySelector(".section-heading span").textContent =
  "12 SIGNALS / DEVICE PIN MAP";
jointWorkspace.querySelector(":scope > p").textContent =
  "Configure travel with all outputs released. Arm at the explicit starting pulse. ±5 adjusts the outstanding target in your chosen direction: minus may still move upward until you set a target below the commanded pulse. Other armed joints continue toward their own targets. STOP holds all joints.";
render();
