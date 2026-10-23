// Spyder standing and walking sketch for the four-leg Uno build.
//
// This is a separate sketch from the calibration firmware in
// firmware/spyder_uno: upload one or the other. Tune numbers in Config.h.
// It runs without a PC: use the A0 button, or send commands from the
// Arduino Serial Monitor at 115200 baud ("help" lists them).
//
// Power the servos from their own 5-6 V supply, not the Uno. The robot
// drops when switched off, so support it the first few times.
#include "Config.h"
#include "Walk.h"
#include <EEPROM.h>
#include <Servo.h>
using namespace walk;

// Holding: servos on, feet still (standing, sitting or mid-calibration).
enum Mode : uint8_t { Off, Moving, Holding, Walking, Calibrating };
enum Pending : uint8_t { None, Sit, SitThenOff };

Servo servos[LegCount][3];
float trim[LegCount][3];
int8_t dir[LegCount][3];
float calAngle[LegCount][3]; // joint angles used in calibration mode

Mode mode = Off;
Pending pending = None;
Gait gait;
float standHeight = STAND_HEIGHT_MM;
float poseHeight = SIT_HEIGHT_MM; // height of the pose being held
float stride = STRIDE_MM;
Vec3 feet[LegCount], moveFrom[LegCount], moveTo[LegCount];
uint32_t moveStart, lastUpdate;
uint16_t moveMs;
bool offAfterMove;

// ---------------- servo output ----------------

uint16_t servoMicros(uint8_t leg, uint8_t joint, float angle) {
  float deg = 90.0f + trim[leg][joint] + dir[leg][joint] * angle;
  if (deg < SERVO_MIN_DEG)
    deg = SERVO_MIN_DEG;
  if (deg > SERVO_MAX_DEG)
    deg = SERVO_MAX_DEG;
  // Same mapping as Servo.write(degrees), with sub-degree resolution.
  return uint16_t(MIN_PULSE_WIDTH +
                  deg * (MAX_PULSE_WIDTH - MIN_PULSE_WIDTH) / 180.0f + 0.5f);
}

void writeAngles(uint8_t leg, const Angles &a) {
  const float angle[3] = {a.hip, a.thigh, a.foot};
  for (uint8_t j = 0; j < 3; ++j) {
    servos[leg][j].writeMicroseconds(servoMicros(leg, j, angle[j]));
    if (!servos[leg][j].attached()) {
      // The pulse is set before attaching so the first pulse is already
      // right. Staggered attaches soften the startup current.
      servos[leg][j].attach(SERVO_PIN[leg][j]);
      delay(40);
    }
  }
}

void writeFeet() {
  for (uint8_t i = 0; i < LegCount; ++i) {
    Angles a;
    solveLeg(bodyToLeg(i, feet[i]), a);
    writeAngles(i, a);
  }
}

void writeCalibration() {
  for (uint8_t i = 0; i < LegCount; ++i)
    writeAngles(i, Angles{calAngle[i][0], calAngle[i][1], calAngle[i][2]});
}

void switchOff() {
  for (uint8_t i = 0; i < LegCount; ++i)
    for (uint8_t j = 0; j < 3; ++j)
      servos[i][j].detach();
  mode = Off;
  pending = None;
  Serial.println(F("off (servos limp)"));
}

// ---------------- poses ----------------

void setPose(float h) {
  for (uint8_t i = 0; i < LegCount; ++i)
    feet[i] = homeFoot(i, STAND_REACH_MM, h);
}

// Glide every foot from where it is now to the pose at height h.
void moveToPose(float h, uint16_t ms, bool thenOff) {
  for (uint8_t i = 0; i < LegCount; ++i) {
    moveFrom[i] = feet[i];
    moveTo[i] = homeFoot(i, STAND_REACH_MM, h);
  }
  moveStart = millis();
  moveMs = ms;
  poseHeight = h;
  offAfterMove = thenOff;
  mode = Moving;
}

void stand() {
  if (mode == Walking) {
    gait.stop(); // ends standing once the current step is finished
    pending = None;
    Serial.println(F("stopping"));
    return;
  }
  if (mode == Off || mode == Calibrating) {
    if (mode == Calibrating)
      switchOff();
    setPose(SIT_HEIGHT_MM); // start low so the rise is gentle
    writeFeet();
  }
  moveToPose(standHeight, POSE_CHANGE_MS, false);
  Serial.println(F("standing up"));
}

void sit(bool thenOff) {
  if (mode == Off)
    return;
  if (mode == Calibrating) {
    switchOff();
    return;
  }
  if (mode == Walking) {
    gait.stop();
    pending = thenOff ? SitThenOff : Sit;
  } else {
    moveToPose(SIT_HEIGHT_MM, POSE_CHANGE_MS, thenOff);
  }
  Serial.println(F("sitting down"));
}

bool standingStill() {
  return mode == Holding && poseHeight == standHeight;
}

void walkCommand(float s, float turn) {
  if (!standingStill() && mode != Walking) {
    Serial.println(F("stand first"));
    return;
  }
  pending = None;
  gait.walk(s, turn);
  mode = Walking;
  Serial.println(F("walking"));
}

void update() {
  const uint32_t now = millis();
  const uint16_t dt = now - lastUpdate;
  lastUpdate = now;
  if (mode == Walking) {
    gait.update(dt, standHeight, feet);
    if (!gait.active()) {
      // Settle the body lean back to the plain pose.
      if (pending == None)
        moveToPose(standHeight, 400, false);
      else
        moveToPose(SIT_HEIGHT_MM, POSE_CHANGE_MS, pending == SitThenOff);
      pending = None;
    }
    writeFeet();
  } else if (mode == Moving) {
    float t = float(now - moveStart) / moveMs;
    if (t >= 1)
      t = 1;
    const float s = t * t * (3 - 2 * t); // ease in and out
    for (uint8_t i = 0; i < LegCount; ++i) {
      feet[i].x = moveFrom[i].x + (moveTo[i].x - moveFrom[i].x) * s;
      feet[i].y = moveFrom[i].y + (moveTo[i].y - moveFrom[i].y) * s;
      feet[i].z = moveFrom[i].z + (moveTo[i].z - moveFrom[i].z) * s;
    }
    writeFeet();
    if (t >= 1) {
      mode = Holding;
      if (offAfterMove)
        switchOff();
      else
        Serial.println(F("ready"));
    }
  } else if (mode == Calibrating) {
    writeCalibration();
  }
}

// ---------------- trims in EEPROM ----------------

struct Saved {
  uint8_t magic, version;
  float trim[LegCount][3];
  int8_t dir[LegCount][3];
  uint8_t sum;
};
const uint8_t SavedMagic = 0x5c;

uint8_t checksum(const Saved &s) {
  const uint8_t *b = reinterpret_cast<const uint8_t *>(&s);
  uint8_t sum = 0;
  for (uint8_t i = 0; i < sizeof(Saved) - 1; ++i)
    sum = sum * 31 + b[i];
  return sum;
}

void loadDefaults() {
  for (uint8_t i = 0; i < LegCount; ++i)
    for (uint8_t j = 0; j < 3; ++j) {
      trim[i][j] = DEFAULT_TRIM[i][j];
      dir[i][j] = DEFAULT_DIR[i][j];
    }
}

void loadTrims() {
  Saved s;
  EEPROM.get(EEPROM_ADDR, s);
  if (s.magic != SavedMagic || s.version != 1 || s.sum != checksum(s)) {
    loadDefaults();
    Serial.println(F("using trims from Config.h"));
    return;
  }
  memcpy(trim, s.trim, sizeof(trim));
  memcpy(dir, s.dir, sizeof(dir));
  Serial.println(F("loaded saved trims"));
}

void saveTrims() {
  Saved s;
  s.magic = SavedMagic;
  s.version = 1;
  memcpy(s.trim, trim, sizeof(trim));
  memcpy(s.dir, dir, sizeof(dir));
  s.sum = checksum(s);
  EEPROM.put(EEPROM_ADDR, s);
  Serial.println(F("saved"));
}

void showTrims() {
  Serial.println(F("// paste into Config.h to make these the defaults"));
  Serial.print(F("static const float DEFAULT_TRIM[4][3] = {"));
  for (uint8_t i = 0; i < LegCount; ++i) {
    Serial.print(i ? F(", {") : F("{"));
    for (uint8_t j = 0; j < 3; ++j) {
      Serial.print(trim[i][j], 1);
      Serial.print(j < 2 ? F(", ") : F("}"));
    }
  }
  Serial.println(F("};"));
  Serial.print(F("static const int8_t DEFAULT_DIR[4][3] = {"));
  for (uint8_t i = 0; i < LegCount; ++i) {
    Serial.print(i ? F(", {") : F("{"));
    for (uint8_t j = 0; j < 3; ++j) {
      Serial.print(dir[i][j]);
      Serial.print(j < 2 ? F(", ") : F("}"));
    }
  }
  Serial.println(F("};"));
}

// ---------------- commands ----------------

void help() {
  Serial.println(F(
      "stand | sit | off | walk [mm] | back | left | right | stop\n"
      "height <mm> | stride <mm>\n"
      "cal              all joints to the reference pose\n"
      "trim <j> <deg>   set joint j's trim (j = leg*3 + 0 hip/1 thigh/2 foot)\n"
      "nudge <j> <deg>  add to joint j's trim\n"
      "test <j> <deg>   in cal: move joint j to an angle to check direction\n"
      "flip <j>         reverse joint j's direction\n"
      "leg <n>          in cal: raise leg n's thigh to find which leg it is\n"
      "save | show | defaults"));
}

bool calJoint(int j) {
  if (j < 0 || j >= LegCount * 3) {
    Serial.println(F("joint must be 0-11"));
    return false;
  }
  if (mode != Calibrating) {
    Serial.println(F("send cal first"));
    return false;
  }
  return true;
}

void startCalibration() {
  if (mode != Off && mode != Calibrating) {
    Serial.println(F("send off first (support the robot)"));
    return;
  }
  memset(calAngle, 0, sizeof(calAngle));
  mode = Calibrating;
  Serial.println(F("calibrating: adjust trims until hips point straight out,"
                   " thighs are level and feet point straight down"));
}

void command(char *line) {
  char *word = strtok(line, " ");
  if (!word)
    return;
  char *arg1 = strtok(nullptr, " "), *arg2 = strtok(nullptr, " ");
  const int j = arg1 ? atoi(arg1) : -1;
  const float value = arg2 ? atof(arg2) : (arg1 ? atof(arg1) : 0);
  if (!strcmp(word, "help"))
    help();
  else if (!strcmp(word, "stand") || !strcmp(word, "stop"))
    stand();
  else if (!strcmp(word, "sit"))
    sit(false);
  else if (!strcmp(word, "off"))
    switchOff();
  else if (!strcmp(word, "walk"))
    walkCommand(arg1 ? value : stride, 0);
  else if (!strcmp(word, "back"))
    walkCommand(-stride, 0);
  else if (!strcmp(word, "left"))
    walkCommand(0, TURN_DEG);
  else if (!strcmp(word, "right"))
    walkCommand(0, -TURN_DEG);
  else if (!strcmp(word, "stride") && arg1) {
    stride = constrain(value, 0, 60);
    Serial.println(F("ok; applies to the next walk/back command"));
  } else if (!strcmp(word, "height") && arg1) {
    if (mode == Walking || mode == Moving) {
      Serial.println(F("wait until still"));
      return;
    }
    standHeight = constrain(value, SIT_HEIGHT_MM, FEMUR_MM + TIBIA_MM - 10);
    if (mode == Holding)
      moveToPose(standHeight, 800, false);
    Serial.println(F("ok"));
  } else if (!strcmp(word, "cal"))
    startCalibration();
  else if (!strcmp(word, "trim") && arg2 && calJoint(j))
    trim[j / 3][j % 3] = value;
  else if (!strcmp(word, "nudge") && arg2 && calJoint(j))
    trim[j / 3][j % 3] += value;
  else if (!strcmp(word, "test") && arg2 && calJoint(j))
    calAngle[j / 3][j % 3] = value;
  else if (!strcmp(word, "flip") && arg1 && calJoint(j))
    dir[j / 3][j % 3] = -dir[j / 3][j % 3];
  else if (!strcmp(word, "leg") && arg1 && calJoint(j * 3)) {
    memset(calAngle, 0, sizeof(calAngle));
    calAngle[j][1] = 30;
  } else if (!strcmp(word, "save"))
    saveTrims();
  else if (!strcmp(word, "show"))
    showTrims();
  else if (!strcmp(word, "defaults")) {
    loadDefaults();
    Serial.println(F("Config.h trims loaded (not saved)"));
  } else
    Serial.println(F("unknown command; try help"));
}

void readSerial() {
  static char line[32];
  static uint8_t length = 0;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\n' || c == '\r') {
      line[length] = 0;
      if (length)
        command(line);
      length = 0;
    } else if (length < sizeof(line) - 1) {
      line[length++] = tolower(c);
    }
  }
}

// ---------------- button ----------------

void readButton() {
  static bool down = false, longDone = false;
  static uint32_t since = 0;
  const bool pressed = digitalRead(BUTTON_PIN) == LOW;
  const uint32_t now = millis();
  if (pressed != down) {
    if (now - since < 30) // debounce
      return;
    down = pressed;
    if (!pressed && !longDone) { // short press
      if (mode == Off || (mode == Holding && poseHeight != standHeight))
        stand();
      else if (standingStill())
        walkCommand(stride, 0);
      else if (mode == Walking)
        stand();
    }
    since = now;
    longDone = false;
  } else if (pressed && !longDone && now - since >= LONG_PRESS_MS) {
    longDone = true;
    sit(true);
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  loadTrims();
  Serial.println(F("SPYDER WALK ready, servos off. Press the button or send"
                   " 'stand'. 'help' lists commands."));
  lastUpdate = millis();
}

void loop() {
  readSerial();
  readButton();
  if (millis() - lastUpdate >= UPDATE_MS)
    update();
}
