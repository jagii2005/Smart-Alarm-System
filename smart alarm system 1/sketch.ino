#include <WiFi.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ── WiFi ─────────────────────────────────────────────
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

// ── ThingSpeak ───────────────────────────────────────
WiFiClient    tsClient;
unsigned long channelID   = 3356347;
const char*   writeAPIKey = "7K9SEVOW0LM9W3B3";

// ── LCD ──────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── Pins ─────────────────────────────────────────────
const int PIN_BUZZER = 25;
const int PIN_LED    = 26;
const int PIN_STOP   = 32;
const int PIN_SNOOZE = 33;
const int PIN_MOOD   = 34;

// ── State ─────────────────────────────────────────────
bool  alarmActive  = true;
int   snoozeCount  = 0;
int   mood         = 0;
bool  puzzleSolved = false;

// ── Puzzle ────────────────────────────────────────────
int puzzleA       = 6;
int puzzleB       = 7;
int correctAnswer = 42;

// ── Timing ────────────────────────────────────────────
unsigned long lastUpload = 0;
const unsigned long UPLOAD_INTERVAL = 15000;

// ── Weather Simulation (FIXED) ────────────────────────
String weatherOptions[] = {
  "32C  Sunny    ",
  "28C  Cloudy   ",
  "25C  Rainy    ",
  "30C  Windy    ",
  "22C  Stormy   "
};
int weatherIndex = 0;

// ── Get rotating weather ──────────────────────────────
String getWeather() {
  weatherIndex = (weatherIndex + 1) % 5;
  return weatherOptions[weatherIndex];
}

// ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED,    OUTPUT);
  pinMode(PIN_STOP,   INPUT_PULLUP);
  pinMode(PIN_SNOOZE, INPUT_PULLUP);
  pinMode(PIN_MOOD,   INPUT);

  // ── LED Boot Test (FIXED) ─────────────────────────
  digitalWrite(PIN_LED, HIGH);
  delay(500);
  digitalWrite(PIN_LED, LOW);
  delay(500);
  digitalWrite(PIN_LED, HIGH);
  delay(500);
  digitalWrite(PIN_LED, LOW);

  lcd.init();
  lcd.backlight();

  // Splash screen
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("  Smart  Alarm  ");
  lcd.setCursor(0, 1); lcd.print(" Initializing...");
  delay(2000);

  // WiFi connect
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Connecting WiFi ");
  lcd.setCursor(0, 1); lcd.print("Please wait.....");
  WiFi.begin(ssid, password);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    ThingSpeak.begin(tsClient);
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WiFi Connected! ");
    lcd.setCursor(0, 1); lcd.print(WiFi.localIP().toString());
    Serial.println("\nWiFi OK");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WiFi Failed!    ");
    lcd.setCursor(0, 1); lcd.print("Offline Mode    ");
    Serial.println("WiFi failed");
  }
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("ALARM IS ON!    ");
  lcd.setCursor(0, 1); lcd.print("Solve puzzle... ");
  delay(1500);
}

// ─────────────────────────────────────────────────────
void loop() {
  if (alarmActive) {
    runAlarm();
  } else {
    routineMode();
  }
}

// ── ALARM MODE (FIXED LED blinking) ──────────────────
void runAlarm() {
  int freq = (snoozeCount >= 2) ? 2000 : 1000;
  tone(PIN_BUZZER, freq);

  // LED blinks during alarm (FIXED)
  digitalWrite(PIN_LED, HIGH);
  delay(200);
  digitalWrite(PIN_LED, LOW);
  delay(200);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WAKE UP! Solve: ");
  lcd.setCursor(0, 1);
  lcd.print(String(puzzleA) + " x " +
            String(puzzleB) + " = ?    ");
  delay(400);

  // SNOOZE pressed
  if (digitalRead(PIN_SNOOZE) == LOW) {
    noTone(PIN_BUZZER);
    digitalWrite(PIN_LED, LOW);
    snoozeCount++;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Snoozed! #" + String(snoozeCount) + "     ");
    lcd.setCursor(0, 1); lcd.print("Back in 5 sec...");
    Serial.println("Snooze #" + String(snoozeCount));
    delay(5000);
    return;
  }

  // STOP pressed = puzzle solved
  if (digitalRead(PIN_STOP) == LOW) {
    noTone(PIN_BUZZER);
    digitalWrite(PIN_LED, LOW);
    puzzleSolved = true;
    alarmActive  = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ans=" + String(correctAnswer) + " Correct!");
    lcd.setCursor(0, 1); lcd.print("Alarm Stopped!  ");
    Serial.println("Alarm stopped. Puzzle solved.");
    delay(2500);
  }
}

// ── ROUTINE MODE (FIXED weather) ─────────────────────
void routineMode() {
  digitalWrite(PIN_LED, HIGH);

  // Mood button
  if (digitalRead(PIN_MOOD) == HIGH) {
    mood = (mood + 1) % 3;
    Serial.println("Mood: " + String(mood));
    delay(400);
  }

  // Show greeting + mood
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Good Morning!   ");
  lcd.setCursor(0, 1);
  switch (mood) {
    case 0: lcd.print("Mood: Happy  :) "); break;
    case 1: lcd.print("Mood: Sleepy -_-"); break;
    case 2: lcd.print("Mood: Stress >:("); break;
  }
  delay(2500);

  // Show weather (FIXED - now rotates)
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Weather Today:  ");
  lcd.setCursor(0, 1); lcd.print(getWeather());
  delay(2500);

  // Show snooze count
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Snooze Count:   ");
  lcd.setCursor(0, 1); lcd.print(String(snoozeCount) + " time(s)        ");
  delay(2000);

  // ThingSpeak upload
  if (millis() - lastUpload > UPLOAD_INTERVAL) {
    sendToThingSpeak();
    lastUpload = millis();
  }
}

// ── THINGSPEAK ────────────────────────────────────────
void sendToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No WiFi - skip upload");
    return;
  }

  ThingSpeak.setField(1, snoozeCount);
  ThingSpeak.setField(2, mood);
  ThingSpeak.setField(3, alarmActive  ? 1 : 0);
  ThingSpeak.setField(4, puzzleSolved ? 1 : 0);

  int result = ThingSpeak.writeFields(channelID, writeAPIKey);

  lcd.clear();
  if (result == 200) {
    lcd.setCursor(0, 0); lcd.print("ThingSpeak: OK! ");
    lcd.setCursor(0, 1); lcd.print("Data Uploaded!  ");
    Serial.println("ThingSpeak: SUCCESS");
  } else {
    lcd.setCursor(0, 0); lcd.print("Upload Failed!  ");
    lcd.setCursor(0, 1); lcd.print("Code:" + String(result));
    Serial.println("ThingSpeak Error: " + String(result));
  }
  delay(2000);
}
