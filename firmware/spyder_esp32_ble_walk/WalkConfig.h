#pragma once
#include <stdint.h>
// Walking settings: everything you may want to tune for the gait is here.
// Lengths are mm, angles degrees. Leg geometry (27.5/55/74 mm) is in LegTest.h.

// Legs 1..4 are front-left, front-right, rear-left and rear-right (the
// corners recorded in docs/project-state.md). Each value is the direction
// that leg points, counter-clockwise from straight ahead seen from above.
// Assumes the front and rear stations of the hexagonal body are used.
static const float LegMountDeg[4] = {30.0f, -30.0f, 150.0f, -150.0f};
// Body centre to each hip (yaw) shaft. The chassis is 136 mm across flats.
const float HipRadiusMm = 80.0f;

// Standing foot position in each leg's frame: the same (85, 0, -60) mm as
// the four-leg test's "stand", so walking starts from that pose.
const float StandReachMm = 85.0f, StandHeightMm = 60.0f;

// Crawl gait: one foot in the air at a time.
const float StrideMm = 20.0f;       // how far each foot pushes back per step
const float TurnDeg = 10.0f;        // body rotation per cycle when turning
const float LiftMm = 15.0f;         // foot lift during a step
const uint16_t CycleMs = 6000;      // all four legs step once
const float BodyShiftMm = 18.0f;    // lean away from the leg about to lift
const float FourFeetDown = 0.5f;    // share of each step slot with all feet down
// Leg numbers 0..3 (leg 1..4) in lifting order: rear-right, front-right,
// rear-left, front-left.
static const uint8_t StepOrder[4] = {3, 1, 2, 0};

// Fastest any joint may move while walking, in tenths of a degree per 20 ms
// update (24 = 120 deg/s). The gait is slow enough to stay under this.
const int16_t WalkMaxStepTenths = 24;
