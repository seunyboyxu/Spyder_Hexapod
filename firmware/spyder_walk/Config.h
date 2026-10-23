#pragma once
#include <stdint.h>
// Every number you may need to tune for standing and walking lives here.
// Lengths are millimetres, angles are degrees.
//
// The leg lengths below are ESTIMATES from the CAD, not measurements:
//  - COXA and FEMUR follow the hpsaturn/quadpod design these parts adapt
//    (27.5 / 55 mm).
//  - TIBIA is the straight-line distance from the knee pivot hole to the
//    foot tip in cad/print/stl/06_Foot_x6.stl (about 74 mm).
// Measure servo shaft centre to servo shaft centre and replace them when
// you can. Everything else is computed from these.

// ---- Leg geometry (pivot to pivot) ----
#define COXA_MM 27.5f  // hip (yaw) shaft -> thigh (pitch) shaft
#define FEMUR_MM 55.0f // thigh shaft -> foot/knee shaft
#define TIBIA_MM 74.0f // knee shaft -> foot tip

// ---- Body geometry ----
// Distance from the body centre to each hip (yaw) shaft. The hexagonal
// chassis is 136 mm across flats, so about 68 mm to a flat plus the mount.
#define HIP_RADIUS_MM 80.0f

// Direction each leg points, measured counter-clockwise from straight
// ahead when looking down on the robot (0 = front, 90 = left side).
// Defaults assume the front and rear stations of the hexagon are used
// and the two middle stations are empty:
//   L0 = front right, L1 = rear right, L2 = rear left, L3 = front left.
// Use the "leg <n>" serial command to see which leg is which and fix this.
static const float LEG_MOUNT_DEG[4] = {-30.0f, -150.0f, 150.0f, 30.0f};

// ---- Servo signal pins: {hip, thigh, foot} for each leg (same wiring
// as the calibration firmware and servo_and_standing_calibration.ino) ----
static const uint8_t SERVO_PIN[4][3] = {
    {2, 3, 4}, {5, 6, 7}, {8, 9, 10}, {11, 12, 13}};

// ---- Servo trims (defaults; "trim" + "save" override these at runtime) ----
// The reference pose is: hip pointing straight out from the body, thigh
// horizontal, lower leg pointing straight down. TRIM is the servo angle
// offset from 90 degrees needed to reach that pose. DIR is +1 or -1 so that
// a positive joint angle means:
//   hip:   leg swings counter-clockwise seen from above
//   thigh: thigh lifts up
//   foot:  foot tip swings in towards the body
// Left and right legs are mirror images, so DIR usually differs per side.
static const float DEFAULT_TRIM[4][3] = {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
static const int8_t DEFAULT_DIR[4][3] = {
    {1, 1, 1}, {1, 1, 1}, {1, -1, -1}, {1, -1, -1}};

// Never command a servo outside this range (Servo.write degrees).
#define SERVO_MIN_DEG 10
#define SERVO_MAX_DEG 170

// ---- Poses ----
// Horizontal distance from the hip shaft to the foot tip, and how far the
// foot is below the thigh shaft.
#define STAND_REACH_MM 85.0f
#define STAND_HEIGHT_MM 60.0f
#define SIT_HEIGHT_MM 25.0f
#define POSE_CHANGE_MS 1500 // time to sit<->stand

// ---- Creep gait (one foot in the air at a time) ----
#define STRIDE_MM 30.0f     // how far each foot pushes back per step
#define TURN_DEG 12.0f      // body rotation per full cycle when turning
#define LIFT_MM 25.0f       // foot lift height during a step
#define CYCLE_MS 2400       // time for all four legs to take one step
#define BODY_SHIFT_MM 20.0f // lean away from the lifted leg (0 = off)
// Share of each leg's step slot with all four feet down, used to move the
// body lean across before the next leg lifts (0..0.8).
#define FOUR_FEET_DOWN 0.4f
// Order the legs lift in. Default: rear right, front right, rear left,
// front left (a standard crawl). Uses the L0..L3 numbering above.
static const uint8_t STEP_ORDER[4] = {1, 0, 2, 3};

// ---- Untethered button ----
// Push button between this pin and GND (uses the internal pull-up).
// Short press: off -> stand -> walk -> stand. Hold 1.5 s: sit and switch off.
#ifndef A0
#define A0 14 // only used when building the tests on a PC
#endif
#define BUTTON_PIN A0
#define LONG_PRESS_MS 1500

#define UPDATE_MS 20     // servo update period
#define SERIAL_BAUD 115200
#define EEPROM_ADDR 512 // trims are stored here; 0..125 is the calibration
                        // firmware's storage and is left untouched
