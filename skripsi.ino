#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>

// WiFi credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// Firebase credentials
#define FIREBASE_HOST "your_firebase_project.firebaseio.com"
#define FIREBASE_AUTH "your_firebase_database_secret"

// Initialize Firebase
FirebaseData firebaseData;

// Initialize Servo
Servo servo;

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize RTC
DS3231 rtc;

// Pin definitions
#define SERVO_PIN 18
#define TRIGGER_PIN 19
#define ECHO_PIN 21
#define PH_SENSOR_PIN 34
#define MOTOR_PIN 23

// Time variables
int feedHour, feedMinute;
bool feedNow = false;

// Function to connect to WiFi
void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

// Function to initialize Firebase
void initializeFirebase() {
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}

// Function to read ultrasonic sensor
long readUltrasonic() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2;
  return distance;
}

// Function to read pH sensor
float readPH() {
  int sensorValue = analogRead(PH_SENSOR_PIN);
  float voltage = sensorValue * (3.3 / 4095.0);
  float phValue = 7 + ((2.5 - voltage) / 0.18);
  return phValue;
}

// Function to feed
void feed() {
  // Open servo
  servo.write(90);
  delay(1000);
  servo.write(0);
  
  // Activate motor to throw feed
  digitalWrite(MOTOR_PIN, HIGH);
  delay(2000);
  digitalWrite(MOTOR_PIN, LOW);
  
  // Reset feedNow
  feedNow = false;
}

// Function to update schedule from Firebase
void updateSchedule() {
  if (Firebase.getInt(firebaseData, "/feedHour")) {
    feedHour = firebaseData.intData();
  }
  if (Firebase.getInt(firebaseData, "/feedMinute")) {
    feedMinute = firebaseData.intData();
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  connectToWiFi();

  // Initialize Firebase
  initializeFirebase();

  // Initialize Servo
  servo.attach(SERVO_PIN);
  servo.write(0);

  // Initialize Motor
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  // Initialize Ultrasonic Sensor
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Initialize RTC
  rtc.begin();
}

void loop() {
  // Update schedule from Firebase
  updateSchedule();

  // Get current time
  DateTime now = rtc.now();
  int currentHour = now.hour();
  int currentMinute = now.minute();
  
  // Check if it's time to feed
  if (currentHour == feedHour && currentMinute == feedMinute) {
    feedNow = true;
  }
  
  // Feed if needed
  if (feedNow) {
    feed();
  }
  
  // Read sensors
  long distance = readUltrasonic();
  float phValue = readPH();
  
  // Update Firebase with sensor data
  Firebase.setFloat(firebaseData, "/phValue", phValue);
  Firebase.setInt(firebaseData, "/feedRemaining", distance);
  
  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("pH: ");
  lcd.print(phValue);
  lcd.setCursor(0, 1);
  lcd.print("Feed: ");
  lcd.print(distance);
  lcd.print(" cm");
  
  delay(60000); // Delay 1 minute
}
