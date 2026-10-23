// Spyder simple walk: Seun's standing pose plus a slow creep gait.
//
// No calibration step. The standing angles are the ones from Seun's
// working stand code, and walking moves each joint a few degrees around
// them. One leg lifts at a time.
//
// COMMANDS: open the Serial Monitor at 9600 baud, line ending "Newline",
// and type one of these (or its one-letter shortcut):
//   stand  (s)        stand in Seun's pose
//   forward (w) [n]   walk forwards n cycles (no number = until stop)
//   back   (b) [n]    walk backwards
//   left   (a) [n]    turn left on the spot
//   right  (d) [n]    turn right on the spot
//   stop   (x)        finish the current step and stand
//   off    (o)        servos go limp (the robot drops - hold it)
//   check  (c)        run the direction check below
//   hip <leg> <deg>, knee <leg> <deg>, foot <leg> <deg>
//                     change one joint's standing angle (legs 1-4) and
//                     move to it, to find good values. Not saved: copy
//                     the numbers into STAND_HIP/KNEE/FOOT below.
// The robot cannot reply: KNEE2 is on pin 1, the USB serial TX pin, so
// the sketch only listens. Unplug that servo, or switch servo power off,
// while uploading.
//
// FIRST RUN: leave CHECK_DIRECTIONS at 1. The robot stands, then for each
// leg in turn (leg 1, 2, 3, 4) it:
//   1. moves the knee by +KNEE_LIFT for a second (the foot should LIFT),
//   2. moves the hip by +HIP_SWING for a second (the foot should swing
//      towards the FRONT of the robot).
// For any leg that moves the wrong way, change its 1 to -1 in KNEE_UP or
// HIP_FORWARD below and upload again. Once they are right, set
// CHECK_DIRECTIONS to 0 (you can still run the check with "check").
#include <Servo.h>

#define CHECK_DIRECTIONS 1

// ---- Pins (same as Seun's code) ----
#define HIP1 2
#define KNEE1 3
#define FOOT1 4

#define HIP2 5
#define KNEE2 1
#define FOOT2 7

#define HIP3 8
#define KNEE3 9
#define FOOT3 10

#define HIP4 11
#define KNEE4 12
#define FOOT4 13

// ---- Standing pose, Servo.write() degrees (from Seun's Stand(90, 90, 50)) ----
//                         leg1 leg2 leg3 leg4
// Leg 2's knee is 90 here, not Seun's 140: on pin 1 with Serial transmit
// on, Seun's sketch never actually sent it a pulse, and 140 drives it past
// its stop. Find the right value with "knee 2 <deg>" and put it here.
int STAND_HIP[4]  = {90,  70,  110, 90};
int STAND_KNEE[4] = {90,  90,  90,  75};
int STAND_FOOT[4] = {50,  50,  0,   25}; // leg3 was 50-55 = -5, which the servo treats as 0

// ---- Directions (check with CHECK_DIRECTIONS) ----
// +1 or -1 so that a positive knee move LIFTS the foot and a positive hip
// move swings the foot towards the FRONT of the robot.
const int KNEE_UP[4]     = {1, 1, 1, 1};
const int HIP_FORWARD[4] = {1, 1, 1, 1};
// Which side each leg is on, used for turning (1 = left side, 0 = right).
const int LEFT_SIDE[4]   = {0, 0, 1, 1};

// ---- Gait settings ----
#define HIP_SWING 20     // degrees each hip swings per step (front to back)
#define KNEE_LIFT 25     // degrees the knee lifts the foot during a step
#define CYCLE_MS 2400    // time for all four legs to take one step each
#define SWING_SHARE 0.8  // share of each leg's turn spent with its foot up
// Order the legs step in (leg numbers 1-4). A crawl steps a rear leg, then
// the front leg on the same side, then the other side.
const int STEP_ORDER[4] = {2, 1, 3, 4};

// ---- Button (optional): between A0 and GND ----
// Press to walk forward WALK_CYCLES cycles; press again to stop early.
// Without a button or PC, set AUTO_START to 1 to walk once after power-up.
#define BUTTON_PIN A0
#define AUTO_START 0
#define WALK_CYCLES 4

#define UPDATE_MS 20

Servo hip[4], knee[4], foot[4];
const int HIP_PIN[4]  = {HIP1, HIP2, HIP3, HIP4};
const int KNEE_PIN[4] = {KNEE1, KNEE2, KNEE3, KNEE4};
const int FOOT_PIN[4] = {FOOT1, FOOT2, FOOT3, FOOT4};

// Write a servo angle with sub-degree resolution, using the same
// 0-180 -> 544-2400 us mapping as Servo.write().
void writeAngle(Servo &s, float deg) {
  if (deg < 0)
    deg = 0;
  if (deg > 180)
    deg = 180;
  s.writeMicroseconds(544 + deg * (2400 - 544) / 180.0);
}

// Put every leg at its standing angle plus the given offsets (degrees).
void setLeg(int leg, float hipOffset, float kneeOffset) {
  writeAngle(hip[leg], STAND_HIP[leg] + HIP_FORWARD[leg] * hipOffset);
  writeAngle(knee[leg], STAND_KNEE[leg] + KNEE_UP[leg] * kneeOffset);
  writeAngle(foot[leg], STAND_FOOT[leg]);
}

void stand() {
  for (int leg = 0; leg < 4; leg++)
    setLeg(leg, 0, 0);
}

// One leg's hip and knee offsets at time t (0..1) through the gait cycle.
// Each leg owns a quarter of the cycle: it lifts, swings forward and puts
// its foot down, then pushes back slowly while the others take their turn.
void legAt(int leg, float t, float &hipOffset, float &kneeOffset) {
  int slot = 0;
  for (int k = 0; k < 4; k++)
    if (STEP_ORDER[k] == leg + 1)
      slot = k;
  float u = t - slot * 0.25;
  if (u < 0)
    u += 1;
  const float swing = 0.25 * SWING_SHARE;
  if (u < swing) {
    float s = u / swing;
    hipOffset = -0.5 + s * s * (3 - 2 * s); // smooth -0.5 -> +0.5
    kneeOffset = sin(PI * s);               // up and back down
  } else {
    hipOffset = 0.5 - (u - swing) / (1 - swing); // +0.5 -> -0.5
    kneeOffset = 0;
  }
}

// Serial commands. Only receiving is used, so pin 1 stays free for KNEE2.
char line[16];
int lineLength = 0;

// Returns true when a full command line has arrived in `line`.
bool readLine() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (lineLength == 0)
        continue;
      line[lineLength] = 0;
      lineLength = 0;
      return true;
    }
    if (lineLength < (int)sizeof(line) - 1)
      line[lineLength++] = tolower(c);
  }
  return false;
}
bool haveCommand = false;

// Walk for a number of cycles. forward: 1 = forwards, -1 = backwards.
// turn: 0 = straight, 1 = turn left, -1 = turn right.
// The stride grows during the first cycle and shrinks during the last, so
// the robot starts and finishes in the standing pose. Any new command, or
// a button press, ends the walk smoothly within one cycle.
void walk(long cycles, int forward, int turn) {
  if (cycles < 1)
    return;
  unsigned long total = cycles * CYCLE_MS;
  const unsigned long start = millis();
  for (;;) {
    unsigned long now = millis() - start;
    const bool gotLine = !haveCommand && readLine();
    if (gotLine || digitalRead(BUTTON_PIN) == LOW) {
      if (gotLine)
        haveCommand = true; // run it once this walk has finished
      // Finish at the end of the next full cycle, shrinking the stride.
      unsigned long end = (now / CYCLE_MS + 1) * CYCLE_MS;
      if (end - now < CYCLE_MS / 2)
        end += CYCLE_MS;
      if (end < total)
        total = end;
    }
    if (now > total)
      now = total;
    float t = (now % CYCLE_MS) / float(CYCLE_MS);
    if (now == total)
      t = 0;
    float size = 1;
    if (now < CYCLE_MS)
      size = now / float(CYCLE_MS);
    // Shrink over the last stretch so the hips come back to standing.
    const unsigned long left = total - now;
    if (left < CYCLE_MS)
      size = min(size, left / float(CYCLE_MS));
    for (int leg = 0; leg < 4; leg++) {
      int dir = forward;
      if (turn != 0) // tank turn: one side forwards, the other backwards
        dir = (LEFT_SIDE[leg] ? -turn : turn);
      float h, k;
      legAt(leg, t, h, k);
      setLeg(leg, dir * h * HIP_SWING * size, k * KNEE_LIFT);
    }
    if (now == total)
      break;
    delay(UPDATE_MS);
  }
  stand();
  while (digitalRead(BUTTON_PIN) == LOW) // don't restart on the same press
    ;
}

void checkDirections() {
  for (int leg = 0; leg < 4; leg++) {
    delay(1500);
    setLeg(leg, 0, KNEE_LIFT); // foot should lift
    delay(1000);
    setLeg(leg, 0, 0);
    delay(700);
    setLeg(leg, HIP_SWING, 0); // foot should swing towards the front
    delay(1000);
    setLeg(leg, 0, 0);
  }
}

bool buttonPressed() {
  if (digitalRead(BUTTON_PIN) == HIGH)
    return false;
  delay(30); // debounce
  while (digitalRead(BUTTON_PIN) == LOW)
    ;
  return true;
}

void attachAll() {
  // Set the standing angles before attaching so the first pulse is right.
  stand();
  for (int leg = 0; leg < 4; leg++) {
    if (hip[leg].attached())
      continue;
    hip[leg].attach(HIP_PIN[leg]);
    knee[leg].attach(KNEE_PIN[leg]);
    foot[leg].attach(FOOT_PIN[leg]);
    delay(100); // one leg at a time to soften the startup current
  }
}

void detachAll() {
  for (int leg = 0; leg < 4; leg++) {
    hip[leg].detach();
    knee[leg].detach();
    foot[leg].detach();
  }
}

void runCommand(char *text) {
  char *word = strtok(text, " ");
  if (!word)
    return;
  char *number = strtok(NULL, " ");
  // No number: keep walking until "stop" (or any other command).
  long cycles = number ? atol(number) : 100000L;
  const char *w = word;
  if (!strcmp(w, "off") || !strcmp(w, "o")) {
    detachAll();
    return;
  }
  attachAll();
  int *joint = NULL;
  if (!strcmp(w, "hip"))
    joint = STAND_HIP;
  else if (!strcmp(w, "knee"))
    joint = STAND_KNEE;
  else if (!strcmp(w, "foot"))
    joint = STAND_FOOT;
  if (joint) {
    char *degrees = strtok(NULL, " ");
    const int leg = number ? atoi(number) : 0;
    if (leg >= 1 && leg <= 4 && degrees)
      joint[leg - 1] = constrain(atoi(degrees), 0, 180);
    stand();
    return;
  }
  if (!strcmp(w, "forward") || !strcmp(w, "w"))
    walk(cycles, 1, 0);
  else if (!strcmp(w, "back") || !strcmp(w, "b"))
    walk(cycles, -1, 0);
  else if (!strcmp(w, "left") || !strcmp(w, "a"))
    walk(cycles, 0, 1);
  else if (!strcmp(w, "right") || !strcmp(w, "d"))
    walk(cycles, 0, -1);
  else if (!strcmp(w, "check") || !strcmp(w, "c"))
    checkDirections();
  else // stand, stop and anything unknown: just stand
    stand();
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(9600);
  UCSR0B &= ~_BV(TXEN0); // give pin 1 back to the KNEE2 servo
  attachAll();
  delay(2000);
  if (CHECK_DIRECTIONS)
    checkDirections();
}

void loop() {
  if (haveCommand) {
    haveCommand = false;
    runCommand(line);
    return;
  }
  if (readLine()) {
    runCommand(line);
    return;
  }
  if (AUTO_START || buttonPressed()) {
    // Button routine: edit this to change what a press does.
    attachAll();
    walk(WALK_CYCLES, 1, 0); // forwards
    if (AUTO_START)
      while (true) // run once, then stand still
        ;
  }
}
