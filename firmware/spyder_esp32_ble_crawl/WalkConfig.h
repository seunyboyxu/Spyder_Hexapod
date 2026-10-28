#pragma once
#include <stdint.h>
// Crawl gait settings: everything you may want to tune is here.
// Lengths mm, angles degrees, speeds mm/s. Leg link lengths are in LegTest.h.
//
// The step sequences are the SunFounder crawling-quadruped gait as used by
// hpsaturn/quadpod (src/servos.cpp: step_forward, step_back, turn_left,
// turn_right), re-expressed in Spyder's body frame. Each foot sits at -1, 0
// or +1 "steps" from its stance point. A cycle is: lift one leg from -1 to
// +1, move the body (every foot -1), lift two more legs, move the body, lift
// the last leg. quadpod's servo mapping and magic offsets are NOT used:
// Spyder's own calibration converts foot positions to servo pulses.

// Legs 0..3 = front-left, front-right, rear-left, rear-right (as
// docs/project-state.md). Direction each leg points, counter-clockwise from
// straight ahead seen from above; front and rear hexagon stations.
static const float LegMountDeg[4] = {30.0f, -30.0f, 150.0f, -150.0f};
const float HipRadiusMm = 80.0f;   // body centre to each hip (yaw) shaft

// Standing pose in each leg's frame: the four-leg test "stand" (85, 0, -60).
const float StandReachMm = 85.0f, StandHeightMm = 60.0f;
// Extra hip yaw for the walking stance, turning feet towards the sides
// (quadpod's stance is square). 0 walks from the v1.1 stand pose. Raising it
// improves balance but needs hip limits wider than the default +-30 deg.
const float StanceYawDeg = 0.0f;

const float StrideMm = 20.0f;  // quadpod y_step (40 there); body moves 2x per cycle
const float TurnDeg = 6.0f;    // the same, as rotation about the body centre
const float LiftMm = 10.0f;    // foot clearance (quadpod: 20; Spyder's default
                               // +-30 deg knee/foot limits allow about 10)

const float LegSpeedMmS = 80.0f;   // lift, carry and lower (one foot)
const float BodySpeedMmS = 40.0f;  // all four feet together

// Before every lift, move the body (all feet down) until its centre is at
// least MinMarginMm inside the other three feet's triangle, moving no further
// than MaxLeanMm from the nominal pose. quadpod's square stance needed no
// lean; Spyder's long, narrow stance does.
const float MinMarginMm = 12.0f, MaxLeanMm = 25.0f;

// Lift order per direction: quadpod's sequences with its legs
// (0 FR, 1 RR, 2 FL, 3 RL) mapped to Spyder's.
static const uint8_t ForwardOrder[4] = {0, 3, 1, 2};  // FL, RR, FR, RL
static const uint8_t BackOrder[4] = {2, 1, 3, 0};     // RL, FR, RR, FL
static const uint8_t LeftOrder[4] = {2, 3, 1, 0};     // RL, RR, FR, FL
static const uint8_t RightOrder[4] = {0, 1, 3, 2};    // FL, FR, RR, RL

// Not the cycle length (that follows from the speeds, about 6-7 s): the
// controller's preflight simulates 5 x CycleMs, which must cover start-up,
// three cycles, a direction change and the stop. Host tests check this.
const uint16_t CycleMs = 9000;

// Fastest any joint may move while walking, in tenths of a degree per 20 ms
// update (24 = 120 deg/s). Host tests check the gait stays under it.
const int16_t WalkMaxStepTenths = 24;
