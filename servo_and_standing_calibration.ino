#include <Servo.h>

#define HIP1 2
#define KNEE1 3
#define FOOT1 4

#define HIP2 5
#define KNEE2 6
#define FOOT2 7

#define HIP3 8
#define KNEE3 9
#define FOOT3 10

#define HIP4 11
#define KNEE4 12
#define FOOT4 13

Servo hip1;
Servo knee1;
Servo foot1;

Servo hip2;
Servo knee2;
Servo foot2;

Servo hip3;
Servo knee3;
Servo foot3;

Servo hip4;
Servo knee4;
Servo foot4;



void setup() {
  //configure servos
  Serial.begin(9600);
  attachMotorPins();
  //SetMotors(90); //set all motors to 90degrees for base calibration
  Stand(90, 90, 50);



}

void attachMotorPins()
{
  hip1.attach(HIP1);
  knee1.attach(KNEE1);
  foot1.attach(FOOT1);

  hip2.attach(HIP2);
  knee2.attach(KNEE2);
  foot2.attach(FOOT2);

  hip3.attach(HIP3);
  knee3.attach(KNEE3);
  foot3.attach(FOOT3);

  hip4.attach(HIP4);
  knee4.attach(KNEE4);
  foot4.attach(FOOT4);

}

void SetMotors(int deg)
{
  hip1.write(deg);
  knee1.write(deg);
  foot1.write(deg);

  hip2.write(deg);
  knee2.write(deg);
  foot2.write(deg);

  hip3.write(deg);
  knee3.write(deg);
  foot3.write(deg);

  hip4.write(deg);
  knee4.write(deg);
  foot4.write(deg);



}

void Stand(int h, int k, int f)
{
  hip1.write(h);
  knee1.write(k);
  foot1.write(f);

  hip2.write(h);
  knee2.write(k);
  foot2.write(f);

  hip3.write(h);
  knee3.write(k);
  foot3.write(f);

  hip4.write(h);
  knee4.write(k);
  foot4.write(f);

}

void loop() {
  

}
