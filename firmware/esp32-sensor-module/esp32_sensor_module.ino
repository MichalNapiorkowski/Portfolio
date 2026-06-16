#include <Wire.h>
#include <Adafruit_VL53L0X.h>

#include <SPI.h>
#include <Ethernet.h>
#include "ModbusServerEthernet.h"

// ================= W5500 PINS =================
#define W5500_MOSI 13
#define W5500_MISO 12
#define W5500_SCK  14
#define W5500_CS   27
#define W5500_RST  26

// ================= I2C PINS =================
#define I2C_SDA 21
#define I2C_SCL 22

// ================= ETHERNET CONFIG =================
byte mac[] = { 0x00, 0x4B, 0x12, 0x8E, 0x99, 0x58 };

IPAddress localIP;

IPAddress ip(192, 168, 76, 70);
IPAddress gateway(192, 168, 76, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

// ================= MODBUS =================
ModbusServerEthernet MBserver;

const uint8_t MODBUS_ID = 255;
const uint16_t HR_COUNT = 10;

uint16_t holdingRegs[HR_COUNT];

const uint16_t resultsRegister = 0;
const uint16_t errorRegister = 1;

// ================= VL53L0X =================
#define XSHUT1 18
#define XSHUT2 19

// 7-bit I2C addresses. Hex form is easier to compare with I2C scanners.
#define S1ADDR 0x1E
#define S2ADDR 0x1F

const uint16_t NO_DISTANCE = 0xFFFF;
const uint16_t DETECT_ON_MM = 85;
const uint16_t DETECT_OFF_MM = 95;
const uint16_t I2C_TIMEOUT_MS = 20;
const uint16_t RANGE_TIMEOUT_MS = 80;
const uint32_t SENSOR_RETRY_INTERVAL_MS = 1000;

Adafruit_VL53L0X lox1;
Adafruit_VL53L0X lox2;

uint16_t dist1 = NO_DISTANCE;
uint16_t dist2 = NO_DISTANCE;

bool errorS1 = false;
bool errorS2 = false;

bool S1Initialized = false;
bool S2Initialized = false;

bool detectedS1 = false;
bool detectedS2 = false;

uint32_t lastRetryS1 = 0;
uint32_t lastRetryS2 = 0;

// ================= W5500 RESET =================
void resetW5500() {
  pinMode(W5500_RST, OUTPUT);

  digitalWrite(W5500_RST, LOW);
  delay(100);

  digitalWrite(W5500_RST, HIGH);
  delay(500);
}

// ================= I2C HELPERS =================
void startI2C() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  // ESP32 Wire timeout protects the loop when a sensor/cable disappears.
  // If your core does not support this name, use Wire.setTimeout(...).
  Wire.setTimeOut(I2C_TIMEOUT_MS);
}

bool pingI2C(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void recoverI2CBus() {
  Wire.end();

  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(2);

  // Clock out a stuck slave if SDA was held low during an interrupted transfer.
  for (uint8_t i = 0; i < 9; i++) {
    pinMode(I2C_SCL, INPUT_PULLUP);
    delayMicroseconds(5);
    pinMode(I2C_SCL, OUTPUT);
    digitalWrite(I2C_SCL, LOW);
    delayMicroseconds(5);
  }

  // Generate a STOP condition: SDA low while SCL released, then SDA released.
  pinMode(I2C_SDA, OUTPUT);
  digitalWrite(I2C_SDA, LOW);
  delayMicroseconds(5);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delayMicroseconds(5);
  pinMode(I2C_SDA, INPUT_PULLUP);
  delayMicroseconds(5);

  startI2C();
}

bool initSensor(Adafruit_VL53L0X &lox, uint8_t xshutPin, uint8_t addr) {
  digitalWrite(xshutPin, LOW);
  delay(20);
  digitalWrite(xshutPin, HIGH);
  delay(60);

  if (!lox.begin(addr, false, &Wire)) {
    digitalWrite(xshutPin, LOW);
    return false;
  }

  if (!pingI2C(addr)) {
    digitalWrite(xshutPin, LOW);
    return false;
  }

  return true;
}

bool updateDetection(uint16_t distance, bool previousState) {
  if (distance == NO_DISTANCE) {
    return false;
  }

  // Hysteresis avoids flicker around the 85 mm threshold.
  if (previousState) {
    return distance < DETECT_OFF_MM;
  }

  return distance < DETECT_ON_MM;
}

bool readDistanceWithTimeout(Adafruit_VL53L0X &lox, uint8_t addr, uint16_t &distance) {
  distance = NO_DISTANCE;

  if (!lox.startRange()) {
    return false;
  }

  uint32_t startedAt = millis();

  while (!lox.isRangeComplete()) {
    if (!pingI2C(addr)) {
      return false;
    }

    if (millis() - startedAt > RANGE_TIMEOUT_MS) {
      return false;
    }

    delay(1);
  }

  uint16_t measuredDistance = lox.readRangeResult();
  uint8_t rangeStatus = lox.readRangeStatus();

  if (!pingI2C(addr)) {
    return false;
  }

  if (rangeStatus == 0 && measuredDistance != NO_DISTANCE) {
    distance = measuredDistance;
  }

  return true;
}

void updateSensor(
  Adafruit_VL53L0X &lox,
  uint8_t xshutPin,
  uint8_t addr,
  bool &initialized,
  bool &runtimeError,
  uint16_t &distance,
  bool &detected,
  uint32_t &lastRetry
) {
  uint32_t now = millis();

  if (!initialized) {
    distance = NO_DISTANCE;
    detected = false;

    if (now - lastRetry >= SENSOR_RETRY_INTERVAL_MS) {
      lastRetry = now;
      initialized = initSensor(lox, xshutPin, addr);
      runtimeError = !initialized && runtimeError;

      if (initialized) {
        runtimeError = false;
      }
    }

    return;
  }

  // Do not call rangingTest() if the sensor does not ACK its assigned address.
  if (!pingI2C(addr)) {
    initialized = false;
    runtimeError = true;
    distance = NO_DISTANCE;
    detected = false;
    lastRetry = now;
    recoverI2CBus();
    return;
  }

  // Non-blocking-style measurement with our own timeout. This avoids getting
  // trapped inside rangingTest() after a sensor/cable failure.
  if (!readDistanceWithTimeout(lox, addr, distance)) {
    initialized = false;
    runtimeError = true;
    distance = NO_DISTANCE;
    detected = false;
    lastRetry = now;
    recoverI2CBus();
    return;
  }

  runtimeError = false;

  detected = updateDetection(distance, detected);
}

// ================= MODBUS FC03 =================
ModbusMessage FC03(ModbusMessage request) {
  uint16_t addr = 0;
  uint16_t words = 0;

  ModbusMessage response;

  request.get(2, addr);
  request.get(4, words);

  uint16_t index = 0;

  if (addr == 0) {
    index = 0;
  } else {
    index = addr - 1;
  }

  if (words == 0 || index >= HR_COUNT || (index + words) > HR_COUNT) {
    response.setError(
      request.getServerID(),
      request.getFunctionCode(),
      ILLEGAL_DATA_ADDRESS
    );

    return response;
  }

  response.add(
    request.getServerID(),
    request.getFunctionCode(),
    (uint8_t)(words * 2)
  );

  for (uint16_t i = 0; i < words; i++) {
    response.add(holdingRegs[index + i]);
  }

  return response;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // ================= I2C =================
  startI2C();

  pinMode(XSHUT1, OUTPUT);
  pinMode(XSHUT2, OUTPUT);

  digitalWrite(XSHUT1, LOW);
  digitalWrite(XSHUT2, LOW);
  delay(50);

  S1Initialized = initSensor(lox1, XSHUT1, S1ADDR);
  Serial.println(S1Initialized ? "S1 init OK" : "S1 init FAIL");

  S2Initialized = initSensor(lox2, XSHUT2, S2ADDR);
  Serial.println(S2Initialized ? "S2 init OK" : "S2 init FAIL");

  // ================= W5500 INIT =================
  resetW5500();

  SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);
  Ethernet.init(W5500_CS);

  Serial.println("Starting Ethernet DHCP...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("DHCP FAIL");
    Ethernet.begin(mac, ip, dns, gateway, subnet);
  }

  delay(1000);

  Serial.println("Ethernet OK");
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("W5500 not found!");
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable not connected!");
  }

  // ================= MODBUS INIT =================
  for (int i = 0; i < HR_COUNT; i++) {
    holdingRegs[i] = 0;
  }

  MBserver.registerWorker(MODBUS_ID, READ_HOLD_REGISTER, &FC03);
  MBserver.start(503, 4, 10000);

  Serial.println("Modbus TCP server started on port 503");
}

void loop() {
  updateSensor(
    lox1,
    XSHUT1,
    S1ADDR,
    S1Initialized,
    errorS1,
    dist1,
    detectedS1,
    lastRetryS1
  );

  updateSensor(
    lox2,
    XSHUT2,
    S2ADDR,
    S2Initialized,
    errorS2,
    dist2,
    detectedS2,
    lastRetryS2
  );

  uint16_t errorCode = 0x0000;
  errorCode |= ((!S1Initialized) << 0);
  errorCode |= ((!S2Initialized) << 1);
  errorCode |= (errorS1 << 2);
  errorCode |= (errorS2 << 3);

  uint16_t detection = (detectedS1 << 0) | (detectedS2 << 1);

  holdingRegs[resultsRegister] = detection;
  holdingRegs[errorRegister] = errorCode;

  // Serial.print("s1: ");
  // Serial.print(detectedS1);
  // Serial.print(" d1: ");
  // Serial.print(dist1);
  // Serial.print(" e1: ");
  // Serial.print(errorS1);
  // Serial.print(" | s2: ");
  // Serial.print(detectedS2);
  // Serial.print(" d2: ");
  // Serial.print(dist2);
  // Serial.print(" e2: ");
  // Serial.print(errorS2);
  // Serial.print(" | err: 0x");
  // Serial.println(errorCode, HEX);

  delay(1);
}
