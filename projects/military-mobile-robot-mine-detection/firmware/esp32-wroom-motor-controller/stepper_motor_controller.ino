#include <AccelStepper.h>
#include <TMCStepper.h>
#include <HardwareSerial.h>

#define EN_PIN           13
#define DIR_PIN          33
#define DIR_PIN_2        32 
#define STEP_PIN         26
#define INTERUPT_PIN     35
#define DRIVER_ADDRESS   0b00
#define R_SENSE 0.11f 

HardwareSerial MOTOR_SERIAL(1);

TMC2209Stepper driver(&MOTOR_SERIAL, R_SENSE, DRIVER_ADDRESS);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, -1);

char lastChar = '0';
unsigned long currentTime = 0;
unsigned long lastActionTime = -1000;
unsigned long lastSgTime = 0;
bool isRunning = false;
char lastSettings = 'c';
int notDetected = false;
bool enable = false;
char receivedChar = '0';
int stepsPerRotation = 200;
char tempSettings = 'a';
bool rotateFlag = false;
bool active = false;

void setup() {
  Serial.begin(9600);
  MOTOR_SERIAL.begin(115200, SERIAL_8N1, -1, 27); 

  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(DIR_PIN_2, OUTPUT);
  pinMode(INTERUPT_PIN, INPUT);

  stepper.setEnablePin(EN_PIN);
  stepper.setPinsInverted(false, false, false);
  stepper.disableOutputs();

  driver.begin();
  driver.toff(3);
  driver.pwm_autoscale(true);
  driver.pwm_autograd(true);
  driver.en_spreadCycle(false);
  driver.rms_current(50);
}

void stepperSettings(int current, int step, float rps, bool en_spreadCycle) {
  float speed = stepsPerRotation * rps * step;
  stepper.disableOutputs();
  driver.microsteps(step);
  driver.rms_current(current);
  driver.en_spreadCycle(en_spreadCycle);
  stepper.setMaxSpeed(speed); 
  stepper.setAcceleration(1 * speed);
  stepper.setPinsInverted(false, false, true);
  stepper.enableOutputs();
}

void setDirection(int dir1, int dir2){
  digitalWrite(DIR_PIN, dir1 > 0 ? HIGH : LOW);
  digitalWrite(DIR_PIN_2, dir2 > 0 ? HIGH : LOW);
}

void setSpeed(char receivedChar){
  switch (receivedChar) {
    case 'a':
      stepperSettings(700, 64, 0.5f, false);
      break;
    case 'b':
      stepperSettings(800, 64, 1.0f, false);
      break;
    case 'c':
      stepperSettings(900, 32, 1.5f, false);
      break;
    case 'd':
      stepperSettings(1000, 32, 2.0f, false);
      break;
    case 'e':
      stepperSettings(1400, 8, 4.0f, true);
      break;
    default:
      break;
  }
  lastSettings = receivedChar;
}

void runMotors(char receivedChar) {
  if(!isRunning){
    setSpeed(lastSettings);
    stepper.move(10000000);
    rotateFlag = false;
  }
  if (receivedChar != lastChar || ((receivedChar == '3' || receivedChar == '4') && !rotateFlag)) {
    switch (receivedChar) {
      case '1':
        setDirection(0, 1);
        break;
      case '2':
        setDirection(1, 0);
        break;
      case '3':
        stepperSettings(1500, 64, 0.5f, false);
        setDirection(1, 1);
        rotateFlag = true;
        break;
      case '4':
        stepperSettings(1500, 64, 0.5f, false);
        setDirection(0, 0);
        rotateFlag = true;
        break;
      default:
        break;
    }
    lastChar = receivedChar;
  }

  lastActionTime = millis();
  isRunning = true;
}

void loop() {
  if (Serial.available() > 0) {
    while (Serial.available() > 0) {
      receivedChar = Serial.read();
    }
    if (receivedChar == 'T' || receivedChar == 'F' ){
      enable = (receivedChar == 'T') ? true : (receivedChar == 'F') ? false : enable;
    } else if(receivedChar >= 'a' && receivedChar <= 'e'){
      setSpeed(receivedChar);
    } else if (receivedChar >= '1' && receivedChar <= '4'){
      runMotors(receivedChar);
      active = true;
    }
  }

  currentTime = millis();
  notDetected = digitalRead(INTERUPT_PIN);

  if ((currentTime - lastActionTime < 210)) {
    stepper.run();
  } else {
    if((currentTime - lastActionTime >= 220) && (currentTime - lastActionTime < 700) ){
      isRunning = false;
      driver.en_spreadCycle(false);
      stepper.setCurrentPosition(0);
      stepper.setPinsInverted(false, false, false);
      driver.rms_current(1000);
    } else {
      driver.rms_current(80);
      driver.en_spreadCycle(false);
      stepper.setCurrentPosition(0);
      stepper.setPinsInverted(false, false, false);
    }
  }
}
