/*     
 *  VREF for A4988 = 1.7 * .4 = .68
 */
#include <AccelStepper.h>
#include <MultiStepper.h>

#define MOTOR_X_ENABLE_PIN 8
#define MOTOR_X_STEP_PIN 2
#define MOTOR_X_DIR_PIN 5
#define MOTOR_Y_ENABLE_PIN 8
#define MOTOR_Y_STEP_PIN 3
#define MOTOR_Y_DIR_PIN 6
#define MOTOR_Z_ENABLE_PIN 8
#define MOTOR_Z_STEP_PIN 4
#define MOTOR_Z_DIR_PIN 7
#define MOTOR_A_ENABLE_PIN 8
#define MOTOR_A_STEP_PIN 12
#define MOTOR_A_DIR_PIN 13
 
// Define a stepper and the pins it will use
AccelStepper stepperX(AccelStepper::DRIVER, MOTOR_X_STEP_PIN, MOTOR_X_DIR_PIN);
AccelStepper stepperY(AccelStepper::DRIVER, MOTOR_Y_STEP_PIN, MOTOR_Y_DIR_PIN);
AccelStepper stepperZ(AccelStepper::DRIVER, MOTOR_Z_STEP_PIN, MOTOR_Z_DIR_PIN);
AccelStepper stepperA(AccelStepper::DRIVER, MOTOR_A_STEP_PIN, MOTOR_A_DIR_PIN);
MultiStepper steppers;

const double stringStep = .02;    // length of string on each step
const double maxMovePerStep = .1;  // the most distance to move in single (should likely be determined by multiple of stringStep)
const double stepsPerUnit = 5;    // number of steps per unit (inch or mm) on the stepper motors

double anchor[][3] = {{0,0,0}, {0,10,0}, {10,10,0}, {10,0,0}};  // location in 3D of the anchor loops
//double lineBuffer[] = {3,3,3,3};    // buffer of extra string - might not be needed if positions initialize to zero

double line[] = {0,0,0,0};    // Array of line lengths
long lline[] = {0,0,0,0};

double current[] = {0,0,1};   // Current position of drone
double target[] = {0,0,0};    // Target position of drone
double next[] = {0,0,0};      // incremental step position
double temp[] = {0,0,0};      // temp array; eg. holding difference in position

/** Main setup */
void setup() {  
  
  double maxxSpeed = 100.0;
  double acceleration = 20.0;

  stepperX.setEnablePin(MOTOR_X_ENABLE_PIN);
  stepperX.setPinsInverted(false, false, true);
  stepperX.setMaxSpeed(maxxSpeed);
  stepperX.setAcceleration(acceleration);  
  stepperX.enableOutputs();
  steppers.addStepper(stepperX);
  
  stepperY.setEnablePin(MOTOR_X_ENABLE_PIN);
  stepperY.setPinsInverted(false, false, true);
  stepperY.setMaxSpeed(maxxSpeed);
  stepperY.setAcceleration(acceleration);  
  stepperY.enableOutputs();
  steppers.addStepper(stepperY);

  stepperZ.setEnablePin(MOTOR_Z_ENABLE_PIN);
  stepperZ.setPinsInverted(false, false, true);
  stepperZ.setMaxSpeed(maxxSpeed);
  stepperZ.setAcceleration(acceleration);  
  stepperZ.enableOutputs();
  steppers.addStepper(stepperZ);

  stepperA.setEnablePin(MOTOR_A_ENABLE_PIN);
  stepperA.setPinsInverted(false, false, true);
  stepperA.setMaxSpeed(maxxSpeed);
  stepperA.setAcceleration(acceleration);  
  stepperA.enableOutputs();
  steppers.addStepper(stepperA);
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);    
}

/** Main loop */
void loop() {
  doRead();
//  stepperX.run();
//  stepperY.run();
//  stepperZ.run();
//  stepperA.run();
  steppers.run();
}

/** Read the serial port & run a command */
void doRead() {
  if(Serial.available() > 0) {
    int command = Serial.read();
    doCommand(command);
  }
}

/** Parse a string into a number */
int parseString(String s, int from, int to) {
  char carray[6];
  s.substring(from, to).toCharArray(carray, to - from + 1);
  int i = atoi(carray);
  return i;
}

/** Perform an action */
void doCommand(int command) {
  // Simple commands to move a motor to a position
  if(command == 'X') {
    int pos = Serial.parseInt();
    Serial.println("Moving x : ");
    Serial.println(pos);
    stepperX.moveTo(pos);
  }
  if(command == 'Y') {
    int pos = Serial.parseInt();
    Serial.println("Moving y to ");
    Serial.println(pos);
    stepperY.moveTo((long) pos);
  }
  if(command == 'Z') {
    int pos = Serial.parseInt();
    Serial.println("Moving z to ");
    Serial.println(pos);
    stepperZ.moveTo((long) pos);
  }
  if(command == 'A') {
    int pos = Serial.parseInt();
    Serial.println("Moving a to ");
    Serial.println(pos);
    stepperA.moveTo((long) pos);
  }
  if(command == 'M') {
    int pos = Serial.parseInt();
    Serial.println("Moving all to ");
    Serial.println(pos);
    for(int i = 0; i < 4; i++) {
      lline[i] = pos;
    }
    steppers.moveTo(lline);
  }
  if(command == 'G') {
    for(int i = 0; i < 4; i++) {
      lline[i] = Serial.parseInt();
    }
    steppers.moveTo(lline);
  }
  if(command == 'V')  { // set the velocity
    int maxxSpeed = Serial.parseInt();
    stepperX.setMaxSpeed(maxxSpeed);
    stepperY.setMaxSpeed(maxxSpeed);
    stepperZ.setMaxSpeed(maxxSpeed);
    stepperA.setMaxSpeed(maxxSpeed);
  }
  
  
  if(command == 'S') {    // Set the starting/current position
    // read 3 more values from serial port & assume that is the current position
    for(int i = 0; i < 3; i++)
      current[i] = Serial.parseInt();
  }
  if(command == 'C') {    // Set the starting/current position
    // read 4 more values from serial port & assume that is the line lengths
    for(int i = 0; i < 3; i++)
      current[i] = Serial.parseInt();
  }
  if(command == 'G') {    // Go to a position
    // read 3 more values from serial port & kick off the move
  }
}

/** Move to a new position */
void moveTo(double position[]) {
  for(int i = 0; i < 3; i++)
    target[i] = position[i];
}

/** Move the drone an incremental step toward the target location */
boolean advance() {
  // First determine the scaling factor to scale down to an incremental move
  double maxDiff = 0;
  double factor = 0;
  for(int i = 0; i < 3; i++) {
    temp[i] = target[i] - current[i];
    if(abs(temp[i]) > maxDiff) {
      maxDiff = abs(temp[i]);
      factor = maxDiff / maxMovePerStep;
    }
  }
  // If we are close, just exit
  if(maxDiff < maxMovePerStep) {
    return false;
  }
  // Determine the next position along this linear path
  for(int i = 0; i < 3; i++) {
    next[i] = current[i] + temp[i] / factor;
  }
  computeLineLength(next);  // Desired line lengths

  steppers.moveTo(lline);
  steppers.runSpeedToPosition(); // Blocks until all are in position
  // Update the current position
  for(int i = 0; i < 3; i++) {
    current[i] = next[i];
  }
}

/** Compute the string lengths needed to be at a given location - results go into slen array */
void computeLineLength(double location[]) {
  for(int i = 0; i < 4; i++) {
    line[i] = distance(location, anchor[i]);
    lline[i] = (long) (stepsPerUnit * line[i]);
  }
}

/** Compute a distance between 2 points in 3d */
double distance(double from[], double to[]) {
  double square = 0.0;
  for(int i = 0; i < 3; i++)
    square += (from[i] - to[i]) * (from[i] - to[i]);
  return sqrt(square);
}
