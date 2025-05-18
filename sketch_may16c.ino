#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>



// --- Hardware Configuration ---
SoftwareSerial arduinoSerial(D7, D8); // RX = D7, TX = D8
LiquidCrystal_I2C lcd(0x27, 16, 2);



// --- WiFi Credentials ---
const char* ssid = "Washek";
const char* password = "wahid@2009";



// --- Web Server Instance ---
ESP8266WebServer server(80);




// --- Display Management ---
unsigned long displayStartTime = 0;
bool isDisplayingMessage = false;
const long DISPLAY_MESSAGE_DURATION = 3000; // Consistent naming
enum DisplayState { SHOW_RTC, SHOW_DHT, SHOW_MOISTURE };
DisplayState currentDisplayState = SHOW_RTC; // More descriptive name
unsigned long displayCycleStartTime = 0;    // Consistent naming
const long DISPLAY_CYCLE_INTERVAL = 2000;   // Consistent naming





// --- Sensor Data Flags and Storage ---
bool rtcDataAvailable = false; // More descriptive
bool dhtDataAvailable = false; // More descriptive
bool moistureDataAvailable = false; // More descriptive
String rtcDisplayData = "";    // More descriptive
String dhtDisplayData = "";    // More descriptive
String moistureDisplayData = ""; // More descriptive
float currentTemperature = -100.0;
float currentHumidity = -1.0;
int currentMoisture = -1;




// --- Alarm System ---
bool isAlarmSet = false;      // Consistent naming
unsigned long pumpStartTime = 0;
bool isPumpActive = false;     // Consistent naming
const unsigned long PUMP_DURATION_MS = 5000; // Explicit unit
String alarmTargetTime = "";  // More descriptive
String alarmTargetTime2 = "";
String lastErrorMessage = "";
int currentHour = -1;
int currentMinute = -1;
int currentSecond = -1;



// --- Automatic Pump Control ---
bool autoPumpEnabled = true; // Flag to enable/disable auto pump
const float AUTO_PUMP_TEMP_THRESHOLD = 32.0;
const float AUTO_PUMP_HUMIDITY_THRESHOLD = 50.0;
const int AUTO_PUMP_MOISTURE_THRESHOLD = 10;



// --- Function Prototypes ---
void handleRoot();
void handleLedOn();
void handleLedOff();
void handlePumpOn();
void handlePumpOff();
void handleSensors();
void handleSetAlarm();
void SerialCommands();
void handleLCDDisplayCycle();
void updateDisplayMessage();
void processSerialData();
void checkAlarmCondition();     // More descriptive
void controlPumpAction();      // More descriptive
void displayLCDMessage(const String& text); // Use const String& for efficiency
void automaticPumpControl1();    // New function for automatic pump control
void automaticPumpControl2();










void setup() {
  Serial.begin(115200);
  arduinoSerial.begin(9600);

  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(2, 0); lcd.print("A.D.I.S.C.A");
  lcd.setCursor(1, 1); lcd.print("-focus better");



  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting");// Indicate connection progress
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected");
  lcd.setCursor(0, 1);
  lcd.print("to the webserver");


  // --- Web Server Routes ---
  server.on("/", handleRoot);
  server.on("/ledOn", handleLedOn);
  server.on("/ledOff", handleLedOff);
  server.on("/pumpOn", handlePumpOn);
  server.on("/pumpOff", handlePumpOff);
  server.on("/setAlarm", handleSetAlarm);
  server.on("/sensors", handleSensors);
 
  
  server.begin();
  Serial.println("HTTP server started");
}






void loop() {
  server.handleClient();
  processSerialData();
  handleLCDDisplayCycle();
  updateDisplayMessage();
  checkAlarmCondition();
  checkAlarmCondition2();
  controlPumpAction();
  automaticPumpControl1();
  automaticPumpControl2();
  SerialCommands(); // Keep this in the loop to be responsive
}







void handleRoot() {
  String alarmStatus = isAlarmSet ? "Alarm set for: " + alarmTargetTime2 : "No alarm set";
  String errorDisplay = lastErrorMessage != "" ? "<h3 style='color: red;'>Error: " + lastErrorMessage + "</h3>" : "";
  String pattern = "([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]"; // Pattern for HH:MM:SS
  String html = "<!DOCTYPE html><html><head>"
                "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                "<title>ADISCA Control Panel</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; }"
                "h2 { font-size: 2em; margin-bottom: 20px; }"
                "button { font-size: 1.5em; padding: 15px 30px; margin: 10px; cursor: pointer; }"
                "h3 { font-size: 1.2em; margin-top: 20px; }"
                "pre { font-size: 1em; background-color: #f0f0f0; padding: 10px; }"
                "form { display: inline-block; margin-top: 20px; }"
                "input { font-size: 1.2em; padding: 10px; margin-right: 10px; }"
                "</style>"
                "</head><body>"
                "<h2>ADISCA Control Panel</h2>"
                "<button onclick=\"location.href='/ledOn'\">LED ON</button> "
                "<button onclick=\"location.href='/ledOff'\">LED OFF</button><br><br>"
                "<button onclick=\"location.href='/pumpOn'\">PUMP ON</button> "
                "<button onclick=\"location.href='/pumpOff'\">PUMP OFF</button><br><br>"
                "<h3>Sensor Data:</h3><pre>" + rtcDisplayData + "  " + dhtDisplayData + "  " + moistureDisplayData + "</pre>"
                "<h3>Alarm Status:</h3><pre>" + alarmStatus + "</pre>"
                + errorDisplay +
                "<form method='get' action='/setAlarm'>"
                "<label for='alarmTime'>Set Alarm (HH:MM:SS): </label>"
                "<input type='text' id='alarmTime' name='time' pattern='" + pattern + "' placeholder='HH:MM:SS' required>"
                "<input type='submit' value='Set Alarm'>"
                "</form><br>"
                "</body></html>";
  server.send(200, "text/html", html);
}






void handleSensors() {
  String jsonResponse = "{";
  jsonResponse += "\"rtc\": \"" + rtcDisplayData + "\",";
  jsonResponse += "\"dht\": \"" + dhtDisplayData + "\",";
  jsonResponse += "\"moisture\": \"" + moistureDisplayData + "\"";
  jsonResponse += "}";
  server.send(200, "application/json", jsonResponse);
}







void handleSetAlarm() {
  String timeString = server.arg("time");
  Serial.print("Received time: ");
  Serial.println(timeString);
  if (timeString.length() == 8 && timeString.indexOf(':') == 2 && timeString.lastIndexOf(':') == 5) {
    // Additional validation for hours
    int hours = timeString.substring(0, 2).toInt();
    if (hours >= 0 && hours <= 23) {
      Serial.print("Alarm set for: ");
      Serial.println(timeString);
      server.send(200, "text/plain", "Alarm set for " + timeString);
      alarmTargetTime2 = timeString;
      isAlarmSet = true;
    } else {
      Serial.println("Invalid hour value (must be 00-23)");
      server.send(200, "text/plain", "Invalid hour value (must be 00-23)");
    }
  } else {
    Serial.println("Invalid time format received");
    server.send(200, "text/plain", "Invalid time format (HH:MM:SS)");
  }
}







void handleLedOn()  {
  Serial.println("Web request: LED ON"); // Debugging output
  arduinoSerial.println("1");
  displayLCDMessage("LED is on");
  displayStartTime = millis();
  isDisplayingMessage = true;
  
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", ""); 
}






void handleLedOff() {
  Serial.println("Web request: LED OFF"); // Debugging output
  arduinoSerial.println("0");
  displayLCDMessage("LED is off");
  displayStartTime = millis();
  isDisplayingMessage = true;
  
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", ""); 
}





void handlePumpOn() {
  Serial.println("Web request: PUMP ON"); // Debugging output
  arduinoSerial.println("2");
  displayLCDMessage("Pump is on");
  displayStartTime = millis();
  isDisplayingMessage = true;
  
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}







void handlePumpOff(){
  Serial.println("Web request: PUMP OFF"); // Debugging output
  arduinoSerial.println("9");
  displayLCDMessage("Pump is off");
  displayStartTime = millis();
  isDisplayingMessage = true;
   
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}







// --- SERIAL COMMUNICATION ---
void SerialCommands() {
  if (Serial.available()) {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim();
    if (inputString.startsWith("3") && inputString.length() > 1) {
      String textToDisplay = inputString.substring(1);
      displayLCDMessage(textToDisplay);
      displayStartTime = millis();
      isDisplayingMessage = true;
    } else if (inputString == "1") {
      arduinoSerial.println("1");
      displayLCDMessage("LED is on");
    } else if (inputString == "0") {
      arduinoSerial.println("0");
      displayLCDMessage("LED is off");
    } else if (inputString == "2") {
      arduinoSerial.println("2");
      displayLCDMessage("PUMP is on");
    } else if (inputString == "9") {
      arduinoSerial.println("9");
      displayLCDMessage("PUMP is off");
    } else if (inputString.startsWith("4") && inputString.length() > 7) {
      int hour = inputString.substring(1, 3).toInt();
      int minute = inputString.substring(4, 6).toInt();
      int second = inputString.substring(7, 9).toInt();
      if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
        alarmTargetTime = (hour < 10 ? "0" : "") + String(hour) + ":" +
                          (minute < 10 ? "0" : "") + String(minute) + ":" +
                          (second < 10 ? "0" : "") + String(second);
        isAlarmSet = true;
        Serial.println("Alarm set to: " + alarmTargetTime);
        displayLCDMessage("Alarm set to:\n" + alarmTargetTime);
      } else {
        displayLCDMessage("Invalid time");
      }
    } displayStartTime = millis();
      isDisplayingMessage = true;
  }
}







void processSerialData() {
  if (arduinoSerial.available()) {
    String data = arduinoSerial.readStringUntil('\n');
    data.trim();
    Serial.println("Received from Arduino: " + data);

    if (data.startsWith("RTC,")) {
      int c1 = data.indexOf(','), c2 = data.indexOf(',', c1 + 1);
      if (c1 > 0 && c2 > c1) {
        String datePart = data.substring(c1 + 1, c2);
        String timePart = data.substring(c2 + 1);
        rtcDisplayData = "Date: " + datePart + "\nTime: " + timePart;
        rtcDataAvailable = true;
        int colon1 = timePart.indexOf(':'), colon2 = timePart.indexOf(':', colon1 + 1);
        if (colon1 > 0 && colon2 > colon1) {
          currentHour = timePart.substring(0, colon1).toInt();
          currentMinute = timePart.substring(colon1 + 1, colon2).toInt();
          currentSecond = timePart.substring(colon2 + 1).toInt();
        }
      }
    } else if (data.startsWith("DHT,")) {
      int c1 = data.indexOf(','), c2 = data.indexOf(',', c1 + 1);
      if (c1 > 0 && c2 > c1) {
        String temp = data.substring(c1 + 1, c2);
        String humidity = data.substring(c2 + 1);
        currentTemperature = temp.toFloat();
        currentHumidity = humidity.toFloat();
        dhtDisplayData = "Temp: " + temp + " C\nHum: " + humidity + " %";
        dhtDataAvailable = true;
      }
    } else if (data.startsWith("MOISTURE,")) {
      int c1 = data.indexOf(',');
      if (c1 > 0) {
        String moisture = data.substring(c1 + 1);
        currentMoisture = moisture.toInt();
        moistureDisplayData = "Moisture: " + moisture + " %";
        moistureDataAvailable = true;
      }
    }
  }
}








// --- LCD DISPLAY ---
void handleLCDDisplayCycle() {
  if (!isDisplayingMessage) {
    if (millis() - displayCycleStartTime >= DISPLAY_CYCLE_INTERVAL) {
      displayCycleStartTime = millis();
      switch (currentDisplayState) {
        case SHOW_RTC:
          if (rtcDataAvailable) displayLCDMessage(rtcDisplayData);
          currentDisplayState = SHOW_DHT;
          break;
        case SHOW_DHT:
          if (dhtDataAvailable) displayLCDMessage(dhtDisplayData);
          currentDisplayState = SHOW_MOISTURE;
          break;
        case SHOW_MOISTURE:
          if (moistureDataAvailable) displayLCDMessage(moistureDisplayData);
          currentDisplayState = SHOW_RTC;
          break;
      }
    }
  }
}







void updateDisplayMessage() {
  if (isDisplayingMessage && (millis() - displayStartTime >= DISPLAY_MESSAGE_DURATION)) {
    lcd.clear();
    isDisplayingMessage = false;
  }
}






void displayLCDMessage(const String& text) {
  lcd.clear();
  if (text.length() <= 16) {
    lcd.setCursor(0, 0);
    lcd.print(text);
  } else {
    lcd.setCursor(0, 0);
    lcd.print(text.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(text.substring(16));
  }
}







void checkAlarmCondition() {
  if (isAlarmSet && currentHour >= 0 && currentMinute >= 0 && currentSecond >= 0) {
    if (alarmTargetTime.length() == 8) {
      String currentTime = (currentHour < 10 ? "0" : "") + String(currentHour) + ":" +
                           (currentMinute < 10 ? "0" : "") + String(currentMinute) + ":" +
                           (currentSecond < 10 ? "0" : "") + String(currentSecond);
      if (currentTime == alarmTargetTime) {
        Serial.println("Alarm 1 triggered - Pump ON");
        arduinoSerial.println("2"); // Pump ON
        isPumpActive = true;
        pumpStartTime = millis();
        isAlarmSet = false; // Optional: disable alarm after triggering
      }
    }
  }
}






void checkAlarmCondition2() {
  if (isAlarmSet && currentHour >= 0 && currentMinute >= 0 && currentSecond >= 0) {
    if (alarmTargetTime2.length() == 8) {
      String currentTime = (currentHour < 10 ? "0" : "") + String(currentHour) + ":" +
                           (currentMinute < 10 ? "0" : "") + String(currentMinute) + ":" +
                           (currentSecond < 10 ? "0" : "") + String(currentSecond);
      if (currentTime == alarmTargetTime2) {
        Serial.println("Alarm 2 triggered - Pump ON");
        arduinoSerial.println("2"); // Pump ON
        isPumpActive = true;
        pumpStartTime = millis();
        isAlarmSet = false; // Optional
      }
    }
  }
}







void controlPumpAction() {
  if (isPumpActive && millis() - pumpStartTime >= PUMP_DURATION_MS) {
    arduinoSerial.println("9"); // Pump OFF
    Serial.println("Pump OFF after duration");
    isPumpActive = false;
  }
}






void automaticPumpControl1() {
  if (autoPumpEnabled && currentTemperature > -100 && currentHumidity > -1 && currentMoisture > -1) { //check if the sensor values are valid
    if (currentTemperature > AUTO_PUMP_TEMP_THRESHOLD &&
        currentHumidity < AUTO_PUMP_HUMIDITY_THRESHOLD &&
        currentMoisture < AUTO_PUMP_MOISTURE_THRESHOLD) {
      if (!isPumpActive) {
        Serial.println("Automatic Pump Control: Conditions met - Pump ON");
        arduinoSerial.println("2"); // Turn pump ON
        isPumpActive = true;
        pumpStartTime = millis(); // Start the timer
      }
    }
  }
}


void automaticPumpControl2() {
  if (autoPumpEnabled && currentTemperature > -100 && currentHumidity > -1 && currentMoisture > -1) { //check if the sensor values are valid
    if (currentTemperature > AUTO_PUMP_TEMP_THRESHOLD ||
        currentHumidity < AUTO_PUMP_HUMIDITY_THRESHOLD ||
        currentMoisture < AUTO_PUMP_MOISTURE_THRESHOLD) {
      if (!isPumpActive) {
        Serial.println("Automatic Pump Control: Conditions met - Pump ON");
        arduinoSerial.println("2"); // Turn pump ON
        isPumpActive = true;
        pumpStartTime = millis(); // Start the timer
      }
    }
  }
}
