/*
 * Smart Energy Meter Interface with GSM  (Arduino Uno)
 *  - Energy metering IC (e.g. ADE7757 / any IC with a CF pulse output) -> pin D2
 *  - SIM800L GSM module: Arduino D7 <- SIM800 TX, Arduino D8 -> SIM800 RX
 *  - Every REPORT_MS the usage, power and bill are sent by SMS.
 *
 * SIMULATE = 1 : no hardware needed. Fake meter pulses are generated in software
 *                and the SMS text is printed to the Serial Monitor (works in Wokwi).
 * SIMULATE = 0 : real hardware (meter IC + SIM800L).
 */
#define SIMULATE 1

#include <SoftwareSerial.h>

const byte  CF_PIN         = 2;                 // interrupt pin (meter CF output)
const float PULSES_PER_KWH = 3200.0;            // from the meter IC datasheet / meter constant
const char  PHONE[]        = "+911234567890";   // <-- put your phone number here

#if SIMULATE
  const unsigned long REPORT_MS = 10000UL;      // report every 10 s for the demo
  const float SIM_LOAD_W   = 1500.0;            // pretend load
  const int   SIM_WEIGHT   = 100;               // each fake pulse counts as 100 (time-lapse demo)
#else
  const unsigned long REPORT_MS = 60000UL;      // report every 60 s
#endif

SoftwareSerial gsm(7, 8);                        // RX, TX

volatile unsigned long pulseCount = 0;
unsigned long lastPulseUs = 0;

void onPulse() {                                 // interrupt: one pulse = fixed amount of energy
  unsigned long now = micros();
  if (now - lastPulseUs > 2000UL) {              // ignore glitches shorter than 2 ms
    pulseCount++;
    lastPulseUs = now;
  }
}

// Slab tariff (sample values in Rs/kWh, change as needed)
float computeBill(float kwh) {
  float bill = 0;
  if (kwh > 200) { bill += (kwh - 200) * 7.0; kwh = 200; }
  if (kwh > 100) { bill += (kwh - 100) * 5.0; kwh = 100; }
  bill += kwh * 3.0;
  return bill;
}

void gsmInit() {
  gsm.begin(9600);
  delay(1000);
  gsm.println("AT");        delay(500);
  gsm.println("ATE0");      delay(500);
  gsm.println("AT+CMGF=1"); delay(500);         // SMS text mode
}

void sendSMS(const char *msg) {
#if SIMULATE
  Serial.println(F("--- SMS (simulated) ---"));
  Serial.println(msg);
  Serial.println(F("-----------------------"));
#else
  gsm.print("AT+CMGS=\""); gsm.print(PHONE); gsm.println("\"");
  delay(1000);
  gsm.print(msg);
  delay(200);
  gsm.write(26);                                 // Ctrl+Z ends the message
  delay(5000);
#endif
}

unsigned long lastReport = 0, lastReportPulses = 0;
unsigned long lastFake = 0;

void setup() {
  Serial.begin(9600);
  pinMode(CF_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CF_PIN), onPulse, FALLING);
#if !SIMULATE
  gsmInit();
#endif
  Serial.println(F("Smart energy meter started"));
}

void loop() {
#if SIMULATE
  // fake pulses: interval (ms) = 3.6e9 / (PULSES_PER_KWH * load_W) / weight
  unsigned long fakeInterval = (unsigned long)(3.6e9 / (PULSES_PER_KWH * SIM_LOAD_W) / SIM_WEIGHT);
  if (millis() - lastFake >= fakeInterval) {
    lastFake = millis();
    noInterrupts(); pulseCount += SIM_WEIGHT; interrupts();
  }
#endif

  if (millis() - lastReport >= REPORT_MS) {
    unsigned long dtMs = millis() - lastReport;
    lastReport = millis();

    noInterrupts(); unsigned long p = pulseCount; interrupts();
    unsigned long dp = p - lastReportPulses;
    lastReportPulses = p;

    float totalKwh = p / PULSES_PER_KWH;
    float powerW   = dp * (3.6e9 / PULSES_PER_KWH) / (float)dtMs;   // average power in the interval
#if SIMULATE
    powerW /= SIM_WEIGHT;                                           // undo the time-lapse weight -> shows the true load
#endif
    float bill = computeBill(totalKwh);

    char e[12], w[12], b[12], msg[100];
    dtostrf(totalKwh, 1, 3, e);
    dtostrf(powerW,   1, 0, w);
    dtostrf(bill,     1, 2, b);
    snprintf(msg, sizeof(msg), "Energy: %s kWh\nPower: %s W\nBill: Rs %s", e, w, b);
    sendSMS(msg);
  }
}
