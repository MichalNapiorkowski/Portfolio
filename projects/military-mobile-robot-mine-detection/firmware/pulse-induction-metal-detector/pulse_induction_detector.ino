const int ilosc_probek = 30;
int probki[ilosc_probek];
const int okno_pomiarowe = 20;
long int srednia[okno_pomiarowe] = {0};
long int srednia_pomiarow = 0;
long int suma = 0;
long int suma_pomiarow = 0;
int j = 0;
int war = 0;
int srednia_pomiarow_poczatkowa = 0;

void setup() {
  pinMode(A0, INPUT);
  pinMode(A4, INPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(11, OUTPUT);

  ADCSRA = (ADCSRA & 0xF8) | 0x02;
  Serial.begin(2000000);
}

void loop() {
  digitalWrite(8, LOW);
  delayMicroseconds(50);
  digitalWrite(8, HIGH);
  delayMicroseconds(40);

  for (int i = 0; i < ilosc_probek; i++) {
    probki[i] = analogRead(A0);
  }

  for (int i = 0; i < ilosc_probek; i++) {
    Serial.println(probki[i]);
  }

  suma = 0;
  for (int i = 0; i < ilosc_probek; i++) {
    suma += probki[i];
  }
  srednia[j] = suma / ilosc_probek;
  // Serial.println(srednia[j]);

  j++;
  if (j == okno_pomiarowe) {
    j = 0;
    suma_pomiarow = 0;

    for (int k = 0; k < okno_pomiarowe; k++) {
      suma_pomiarow += srednia[k];
    }
    srednia_pomiarow = suma_pomiarow / okno_pomiarowe;

    if (war < 220 && war > 200){
    if (srednia_pomiarow_poczatkowa < srednia_pomiarow){
      srednia_pomiarow_poczatkowa = srednia_pomiarow;
    }
    }
      war++;
    // Serial.println(srednia_pomiarow-srednia_pomiarow_poczatkowa);

    if (srednia_pomiarow > srednia_pomiarow_poczatkowa) {
      float ton = map(srednia_pomiarow, 0, 1024, 200, 2000);
      tone(11, ton); 
    }
    delayMicroseconds(100);
    noTone(11);

    for (int k = 0; k < okno_pomiarowe; k++) {
      srednia[k] = 0;
    }
  }

    // delayMicroseconds(1700);
}
