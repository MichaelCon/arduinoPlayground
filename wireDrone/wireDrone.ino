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

const double WIDTH = 150;    // Measure in real word units (inches) of movement area
const double LENGTH = 188;   // Measure in real word units (inches) of movement area
const double stepsPerUnit = 38;    // number of steps per unit (inch) on the stepper motors (eg 4 inches per 200 steps = 50)
const double maxMovePerStep = 4;  // the most distance to move in single (should likely be determined by multiple of stringStep)

double anchor[][3] = {{0,0,0}, {0,LENGTH,0}, {WIDTH, LENGTH,0}, {WIDTH,0,0}};  // location in 3D of the anchor loops
double lineOffset[] = {0,0,0,0};    // offset between stepper motor and line out

double line[] = {0,0,0,0};    // Array of line lengths
long lline[] = {0,0,0,0};

double current[] = {75, 94, 18};   // Current position of drone - note the positive z shouldn't matter as long it's consistent
double target[] = {0,0,0};    // Target position of drone
double next[] = {0,0,0};      // incremental step position
double temp[] = {0,0,0};      // temp array; eg. holding difference in position

boolean midStep = false;
boolean midMotion = false;

int stepCount = 0;

/** Main setup */
void setup() {  
  
  double maxxSpeed = 400.0;
  double acceleration = 100.0;

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

  calibarateOffsetsFromPosition();   
}

/** Main loop */
void loop() {
  doRead();
  // If steppers need to run, then run
  if(steppers.run()) {
    if(!midStep) {
      Serial.println("Taking a step");
      midStep = true; 
    }
  } else {    // We are not in the middle of a step
    if(midMotion) {
      midMotion = advance();  // This advances the stepper locations, so the next loop will trigger midStep back to true
      printCurrentPosition();
    }
  }
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
  if(command == 'G') {    // Go to a position
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
    
  if(command == 'S') {    // Set the starting/current position in XYZ space
    // read 3 more values from serial port & assume that is the current position
    for(int i = 0; i < 3; i++) {
      current[i] = Serial.parseInt();
    }
    // Compute the offsets for the stepper motors awareness
    calibarateOffsetsFromPosition();
  }
  if(command == 'L') {    // Set the current line lenghts (4 int to follow)
    // read 4 more values from serial port & assume that is the current position
    for(int i = 0; i < 3; i++) {
      line[i] = Serial.parseInt();
    }
    // Compute the offsets for the stepper motors awareness
    calibarateOffsetsFromLines();
  }
  if(command == 'M') {    // Move to a new position - 3 numbers to follow
    for(int i = 0; i < 3; i++) {
      target[i] = Serial.parseInt();
    }
    stepCount = 0;
    midMotion = true;
    advance();
  }
  if(command == 'D') {    // Print info to debug
    printCurrentPosition();
    printLineSteps();
    printLineLengths();
  }
}

/** Move to a new position */
void moveTo(double position[]) {
  for(int i = 0; i < 3; i++)
    target[i] = position[i];
}

/** Print current position to Serial */
void printCurrentPosition() {
    Serial.println("Current");
    for(int i = 0; i < 3; i++) {
      Serial.println(current[i]);
    }
}
/** Print current string lenghts to Serial */
void printLineLengths() {
    Serial.println("Line lengths");
    for(int i = 0; i < 4; i++) {
      Serial.println((double) (lline[i] + lineOffset[i]) / stepsPerUnit);
    }
}
/** Print current string step counts to Serial */
void printLineSteps() {
    Serial.println("lline");
    for(int i = 0; i < 4; i++) {
      Serial.println(lline[i]);  
    }
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
  computeLineLength(next);  // Determine desired line lengths and update the array

  stepCount++;
  Serial.println("Step #");
  Serial.println(stepCount);
  Serial.println("Next position");
  for(int i = 0; i < 4; i++) {
    Serial.println(lline[i]);  
  }
  
  steppers.moveTo(lline);

  // Update the current position
  for(int i = 0; i < 3; i++) {
    current[i] = next[i];
  }
  return true;
}

/** Compute the string lengths needed to be at a given location - results go into the main array */
void computeLineLength(double location[]) {
  for(int i = 0; i < 4; i++) {
    line[i] = distance(location, anchor[i]);
    lline[i] = (long) (stepsPerUnit * line[i]) - lineOffset[i];
  }
}

/** Calibrate the offsets of the stepper motors based on the length array and current position */
void calibarateOffsetsFromPosition() {
  for(int i = 0; i < 4; i++) {
    line[i] = distance(current, anchor[i]);
    lineOffset[i] = (long) (stepsPerUnit * line[i]) - lline[i];
    Serial.println(lineOffset[i]);
  }  
}
/** Calibrate the offsets of the stepper motors based on the length array and current position */
void calibarateOffsetsFromLines() {
  for(int i = 0; i < 4; i++) {
    // TODO
  }  
}

/** Compute a distance between 2 points in 3d */
double distance(double from[], double to[]) {
  double square = 0.0;
  for(int i = 0; i < 3; i++)
    square += (from[i] - to[i]) * (from[i] - to[i]);
  return sqrt(square);
}

