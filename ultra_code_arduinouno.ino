#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>       // Include the DHT library
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

Servo myServo1; // create servo object for servo 1 (originally myServo)
Servo myServo2; // create servo object for servo 2
Servo myServo3; // create servo object for servo 3

// --- Display Management ---
unsigned long displayStartTime = 0;
bool isDisplayingMessage = false;
const long DISPLAY_MESSAGE_DURATION = 3000; // Consistent naming
enum DisplayState { SHOW_RTC, SHOW_DHT, SHOW_MOISTURE, SHOW_ULTRASONIC };
DisplayState currentDisplayState = SHOW_RTC; // More descriptive name
unsigned long displayCycleStartTime = 0;    // Consistent naming
const long DISPLAY_CYCLE_INTERVAL = 2000;   // Consistent naming

// These global variables will store the data to be displayed on the LCD
String rtcData = "";
String dhtData = "";
String moistureData = "";
String ultrasonicData = "";


const int servo1Pin = 11; // Original servo pin
const int servo2Pin = 12; // New servo pin
const int servo3Pin = 10; // New servo pin

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
#include <SoftwareSerial.h>
SoftwareSerial espSerial(2, 3);  // RX = 2, TX = 3

unsigned long lastTime = 0;
const long interval = 1000;

// Add a global variable to store custom LCD text
String customLCDText = "";
bool displayCustomText = false; // Flag to indicate if custom text should be displayed


#define PIN1 8      // First control pin
#define PIN2 7      // Second control pin
#define DHTPIN 9    // DHT11 sensor pin
#define DHTTYPE DHT11  // DHT11 or DHT22 (AM2302)
#define MoisturePin A0 // Soil Moisture Sensor Pin
#define trigPin 4        // Ultrasonic sensor trigger pin
#define echoPin 5

DHT dht(DHTPIN, DHTTYPE); // Create DHT sensor object

void setup() {
  espSerial.begin(9600);
  pinMode(PIN1, OUTPUT);
  pinMode(PIN2, OUTPUT);
  digitalWrite(PIN1, HIGH);
  digitalWrite(PIN2, HIGH);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(2, 0); lcd.print("A.D.I.S.C.A");
  lcd.setCursor(1, 1); lcd.print("-focus better");
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    Serial.flush();
    abort();
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //comment to avoid reset everytime code is uploaded
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //uncomment after first upload with battery
  dht.begin();  // Initialize DHT sensor
  delay(2000); // Give some time for the initial LCD message to be seen
  lcd.clear(); // Clear the setup message before starting the display cycle
}

void sendRTCData() {
  DateTime now = rtc.now();
  // Ensure HH:MM:SS format with leading zeros
  String hour = (now.hour() < 10 ? "0" : "") + String(now.hour());
  String minute = (now.minute() < 10 ? "0" : "") + String(now.minute());
  String second = (now.second() < 10 ? "0" : "") + String(now.second());

  // Assign to the GLOBAL rtcData variable
  rtcData = "RTC," + String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + "," +
            hour + ":" + minute + ":" + second;
  espSerial.println(rtcData); // Send RTC data with "RTC" prefix
}

void sendDHTData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    // Assign error message to the GLOBAL dhtData variable
    dhtData = "DHT,Failed to read";
    espSerial.println(dhtData);
    return;
  }
  // Assign to the GLOBAL dhtData variable
  dhtData = "DHT," + String(t) + "," + String(h);
  espSerial.println(dhtData); // Send DHT data with "DHT" prefix
}

void sendMoistureData() {
  int sensorValue = analogRead(MoisturePin);
  int moisture = map(sensorValue, 0, 1023, 100, 0); // Map to 1-100 range
  if (moisture < 0) {
    moisture = 0;
  }
  if (moisture > 100) {
    moisture = 100;
  }
  // Assign to the GLOBAL moistureData variable
  moistureData = "MOISTURE," + String(moisture) + "%"; // Added '%' for clarity
  espSerial.println(moistureData);
}

void sendUltrasonicData() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distanceCm = duration * 0.034 / 2; // Speed of sound in cm/µs is approx 0.034

  // Assign to the GLOBAL ultrasonicData variable
  ultrasonicData = "ULTRASONIC," + String(distanceCm) + "cm"; // Added 'cm' for clarity
  espSerial.println(ultrasonicData);
}

void loop() {
  // Always call data sending functions to keep global variables updated
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= interval) {
    lastTime = currentTime;
    sendRTCData(); // Send RTC data and update global rtcData
    sendDHTData(); // Send DHT data and update global dhtData
    sendMoistureData(); // Send moisture sensor data and update global moistureData
    sendUltrasonicData(); // Send ultrasonic data and update global ultrasonicData
  }

  // Handle LCD display. Prioritize custom text if set.
  if (displayCustomText) {
    if (millis() - displayStartTime >= DISPLAY_MESSAGE_DURATION) {
      lcd.clear();
      displayCustomText = false; // Custom text display time is over
      // Reset the sensor display cycle so it starts fresh after custom text
      currentDisplayState = SHOW_RTC;
      displayCycleStartTime = millis(); 
    }
  } else {
    handleLCDDisplayCycle(); // Handle cycling through display states for sensor data
    updateDisplayMessage();  // Handle clearing the display after sensor message duration
  }


  if (espSerial.available()) {
    char cmdChar = espSerial.peek(); // Peek to see if it's '3'
    if (cmdChar == '3') {
      espSerial.read(); // Consume the '3'
      customLCDText = espSerial.readStringUntil('\n');
      customLCDText.trim(); // Remove any whitespace
      
      Serial.print("Received custom LCD text: ");
      Serial.println(customLCDText);

      // Display the custom text immediately
      displayLCDMessage(customLCDText);
      displayCustomText = true; // Set flag to keep displaying it for a duration
      displayStartTime = millis(); // Start timer for custom text display
      
    } else {
      // Process other single-character commands as before
      char cmd = espSerial.read();
      while (espSerial.available()) espSerial.read(); // Clear serial buffer
      switch (cmd) {
        case '1':
          digitalWrite(PIN1, LOW);
          break;
        case '0':
          digitalWrite(PIN1, HIGH);
          break;
        case '2':
          digitalWrite(PIN2, LOW);
          break;
        case '9':
          digitalWrite(PIN2, HIGH);
          break;
        case '5':
          delay(200);
          myServo1.attach(servo1Pin);
          myServo1.write(0);
          delay(200);
          myServo1.detach();
          break;
        case 't':
          delay(200);
          myServo1.attach(servo1Pin);
          myServo1.write(180);
          delay(200);
          myServo1.detach();
          break;
        case '6':
          delay(200);
          myServo2.attach(servo2Pin);
          myServo2.write(0);
          delay(200);
          myServo2.detach();
          break;
        case 'y':
          delay(200);
          myServo2.attach(servo2Pin);
          myServo2.write(180);
          delay(200);
          myServo2.detach();
          break;
        case '7':
          delay(200);
          myServo3.attach(servo3Pin);
          myServo3.write(0);
          delay(200);
          myServo3.detach();
          break;
        case 'u':
          delay(200);
          myServo3.attach(servo3Pin);
          myServo3.write(180);
          delay(200);
          myServo3.detach();
          break;
        default:
          break;
      }
    }
  }
}

// --- LCD DISPLAY FUNCTIONS ---
void handleLCDDisplayCycle() {
  // Only proceed if a message isn't currently being displayed (i.e., not in the middle of DISPLAY_MESSAGE_DURATION)
  if (!isDisplayingMessage) {
    if (millis() - displayCycleStartTime >= DISPLAY_CYCLE_INTERVAL) {
      displayCycleStartTime = millis(); // Reset timer for next cycle
      switch (currentDisplayState) {
        case SHOW_RTC:
          displayLCDMessage("Time: " + rtcData.substring(rtcData.indexOf(',') + 1)); // Extract just time/date part
          currentDisplayState = SHOW_DHT;
          break;
        case SHOW_DHT:
          displayLCDMessage("Temp/Hum: " + dhtData.substring(dhtData.indexOf(',') + 1)); // Extract just temp/hum part
          currentDisplayState = SHOW_MOISTURE;
          break;
        case SHOW_MOISTURE:
          displayLCDMessage("Moisture: " + moistureData.substring(moistureData.indexOf(',') + 1)); // Extract just moisture part
          currentDisplayState = SHOW_ULTRASONIC;
          break;
        case SHOW_ULTRASONIC:
          displayLCDMessage("Distance: " + ultrasonicData.substring(ultrasonicData.indexOf(',') + 1)); // Extract just distance part
          currentDisplayState = SHOW_RTC; // Cycle back to RTC
          break;
      }
      isDisplayingMessage = true; // A message is now being displayed
      displayStartTime = millis(); // Set the start time for this message
    }
  }
}

void updateDisplayMessage() {
  // If a message is being displayed and its duration has passed, clear the LCD
  if (isDisplayingMessage && (millis() - displayStartTime >= DISPLAY_MESSAGE_DURATION)) {
    lcd.clear();
    isDisplayingMessage = false; // No message is currently being displayed
  }
}

void displayLCDMessage(const String& text) {
  lcd.clear(); // Clear before printing new message
  lcd.setCursor(0, 0); // Start at top-left
  // Print up to 16 characters on the first line
  lcd.print(text.substring(0, min((int)text.length(), 16)));

  // If the text is longer than 16 characters, print the rest on the second line
  if (text.length() > 16) {
    lcd.setCursor(0, 1); // Move to second line
    lcd.print(text.substring(16));
  }
}
