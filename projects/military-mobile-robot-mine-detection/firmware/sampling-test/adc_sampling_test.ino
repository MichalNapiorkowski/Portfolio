// const float czestotliwosc = 7000;
// const float czestotliwosc_probkowania = 300000;
const float czestotliwosc = 200;
const float czestotliwosc_probkowania = 2000000;
float impuls;
float probkowanie;
int opoznienie = 0;

unsigned long startTime = 0;
unsigned long elapsedTime = 0;
unsigned long loopCounter = 0;               
unsigned long serialPrintCounter = 0; 

void setup() {
  pinMode(A0, INPUT);
  pinMode(8, OUTPUT);

  ADCSRA = (ADCSRA & 0xF8) | 0x04; // 0x05 - 38.5kHz, 0x04 - 79kHz
  Serial.begin(2000000);

  impuls = (1000000 / czestotliwosc);
  probkowanie = (1000000 / czestotliwosc_probkowania);
  opoznienie = impuls * 0.998;
  startTime = millis();
}

void loop() {
  elapsedTime = millis() - startTime;
  // if (elapsedTime >= 5000) {
    if (elapsedTime > -2) {
    Serial.print("Switching frequency: ");
    Serial.println(loopCounter/5);
    
    Serial.print("Sampling frequency: ");
    Serial.println(serialPrintCounter/5);

    Serial.print("Samples per period: ");
    Serial.println(serialPrintCounter/loopCounter);
    
    while (true) {
    }
  }
  digitalWrite(8, HIGH);
    for (int i = 0; i < (impuls - opoznienie) / probkowanie; i++) {
    Serial.println(analogRead(A0));
    serialPrintCounter++; 
    delayMicroseconds(probkowanie);
  }
  digitalWrite(8, LOW);

  for (int i = 0; i < 0.02*opoznienie / probkowanie; i++) {
    Serial.println(analogRead(A0));
    serialPrintCounter++; 
    delayMicroseconds(probkowanie);
  }
      delayMicroseconds(0.8*opoznienie);

  loopCounter++;
}
