#include <Wire.h>

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>



// --- Hardware Configuration ---
SoftwareSerial arduinoSerial(D7, D8); // RX = D7, TX = D8




// --- WiFi Credentials ---
const char* ssid = "Washek";
const char* password = "wahid@2009";

// Define a static IP address for your ESP8266 (optional)
IPAddress staticIP(192, 168, 0, 183); // Your desired static IP
IPAddress gateway(192, 168, 0, 1);    // Your router's gateway IP
IPAddress subnet(255, 255, 255, 0);  // Your subnet mask

// --- Web Server Instance ---
ESP8266WebServer server(80);






// --- Sensor Data Flags and Storage ---

String rtcDisplayData1 = "";    // More descriptive
String rtcDisplayData2 = "";
String dhtDisplayData = "";    // More descriptive
String moistureDisplayData = ""; // More descriptive
String ultrasonicDisplayData = "";

int currentMoisture = -1;
float currentTemperature = -100.0;
float currentUltrasonicDistance = 0.0;




// --- Alarm System ---
bool isAlarmSet = false;      // Consistent naming
bool isAlarmSet1 = false; 
bool isAlarmSet2 = false; 
bool isAlarmSet3 = false; 
bool isAlarmSet4 = false; 
bool isAlarmSet5 = false; 

unsigned long pumpStartTime = 0;
bool isPumpActive = false;     // Consistent naming
const unsigned long PUMP_DURATION_MS = 5000; // Explicit unit
String alarmTargetTime = "";  // More descriptive
String lastErrorMessage = "";
int currentHour = -1;
int currentMinute = -1;
int currentSecond = -1;



// --- Global variable to store watering times ---
String timer[5] = {"", "", "", "", ""}; // Stores times as "HH:MM:SS" strings


String receivedLCDText = ""; // This will hold the text you want to display


// --- Function Prototypes ---
void handleRoot();
void handleLedOn();
void handleLedOff();
void handlePumpOn();
void handlePumpOff();
void handleSensors();
void SerialCommands();
void processSerialData();
void checkAlarmCondition();     // More descriptive
void controlPumpAction();      // More descriptive
void handleSetLCDText();
void handleSetTimersWeb();
void handleSetTimersApp();












void setup() {
  Serial.begin(115200);
  arduinoSerial.begin(9600);


  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  // --- Web Server Routes ---
  server.on("/", handleRoot);
  server.on("/ledOn", handleLedOn);
  server.on("/ledOff", handleLedOff);
  server.on("/pumpOn", handlePumpOn);
  server.on("/pumpOff", handlePumpOff);
  server.on("/valve1On", handleValve1On);
  server.on("/valve1Off", handleValve1Off);
  server.on("/valve2On", handleValve2On);
  server.on("/valve2Off", handleValve2Off);
  server.on("/valve3On", handleValve3On);
  server.on("/valve3Off", handleValve3Off);
  server.on("/sensors", handleSensors);
  server.on("/setLCDText", HTTP_POST, handleSetLCDText);
  server.on("/setTimers1", HTTP_POST, handleSetTimersWeb);
  server.on("/setTimers2", HTTP_POST, handleSetTimersApp);
 
  
  server.begin();
  Serial.println("HTTP server started");
}






void loop() {
  server.handleClient();
  processSerialData();
  checkAlarmCondition();
  controlPumpAction();
  SerialCommands(); // Keep this in the loop to be responsive
}







void handleRoot() {
  String errorDisplay = lastErrorMessage != "" ? "<h3 style='color: red;'>Error: " + lastErrorMessage + "</h3>" : "";
  String html = "<!DOCTYPE html><html><head>"
              "<meta name='viewport' content='width=device-width, initial-scale=1'>"
              "<title>ADISCA Control Panel</title>"
              "<style>"
              "body { font-family: Arial, sans-serif; text-align: center; }"
              "h2 { font-size: 2em; margin-bottom: 20px; }"
              "button { font-size: 1.5em; padding: 15px 30px; margin: 10px; cursor: pointer; }"
              "h3 { font-size: 1.2em; margin-top: 20px; }"
              "pre { font-size: 1em; background-color: #f0f0f0; padding: 10px; }"
              ".timer-section { margin: 20px auto; max-width: 500px; border: 1px solid #ccc; padding: 15px; border-radius: 5px; }"
              ".timer-slot { display: flex; align-items: center; margin-bottom: 10px; }"
              ".timer-slot label { flex-grow: 1; text-align: left; margin-right: 10px; }"
              ".timer-slot input[type=number] { padding: 8px; font-size: 1em; border-radius: 3px; border: 1px solid #ddd; width: 50px; text-align: center; margin: 0 2px; }" // Adjusted width
              ".timer-slot span.separator { font-size: 1.2em; margin: 0 2px; }"
              ".timer-slot button { padding: 8px 15px; font-size: 1em; cursor: pointer; border-radius: 3px; margin-left: 10px; }"
              ".controls { margin-top: 20px; }"
              ".input-group { margin: 20px auto; max-width: 300px; text-align: center; }"
              ".input-group input[type=text] { padding: 10px; font-size: 1em; width: 100%; box-sizing: border-box; margin-bottom: 10px; border-radius: 3px; border: 1px solid #ddd; }"
              ".input-group button[type=submit] { font-size: 1.2em; padding: 10px 20px; cursor: pointer; border-radius: 3px; }"
              "</style>"
              "</head><body>"
              "<h2>ADISCA Control Panel</h2>"
              "<button onclick=\"location.href='/ledOn'\">LED ON</button> "
              "<button onclick=\"location.href='/ledOff'\">LED OFF</button><br><br>"
              "<button onclick=\"location.href='/pumpOn'\">PUMP ON</button> "
              "<button onclick=\"location.href='/pumpOff'\">PUMP OFF</button><br><br>"

     
              "<button onclick=\"location.href='/valve1On'\">Valve 1 ON</button> "
              "<button onclick=\"location.href='/valve1Off'\">Valve 1 OFF</button><br>"
              "<button onclick=\"location.href='/valve2On'\">Valve 2 ON</button> "
              "<button onclick=\"location.href='/valve2Off'\">Valve 2 OFF</button><br>"
              "<button onclick=\"location.href='/valve3On'\">Valve 3 ON</button> "
              "<button onclick=\"location.href='/valve3Off'\">Valve 3 OFF</button><br><br>"      


              "<h3>Sensor Data:</h3><pre>" + rtcDisplayData2 + "  " + dhtDisplayData + "  " + moistureDisplayData + " " + ultrasonicDisplayData + "</pre>"
              + errorDisplay +

              "<h3>Send Text to LCD:</h3>"
              "<div class='input-group'>"
              "<form action='/setLCDText' method='post'>"
              "<input type='text' name='text' placeholder='Enter text for LCD'>"
              "<button type='submit'>Send to LCD</button>"
              "</form>"
              "</div><br>"

              "<h3>Set Automatic Watering Timings (Daily):</h3>"
              "<div class='timer-section'>"
              "<p>Enter up to 5 daily watering times (HH:MM:SS):</p>" // Updated text
              "<form action='/setTimers1' method='post'>"
              // Timer 1
              "<div class='timer-slot'>"
              "<label>Time 1:</label>"
              "<input type='number' name='timer1_h' min='0' max='23' placeholder='HH' value='" + getHour(timer[0]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer1_m' min='0' max='59' placeholder='MM' value='" + getMinute(timer[0]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer1_s' min='0' max='59' placeholder='SS' value='" + getSecond(timer[0]) + "'>"
              "<div class='controls'>"
              "<button type='submit'>Set Timers</button> "
              "</div>"
              "<button type='button' onclick=\"document.querySelector('input[name=\\'timer1_h\\']').value = '0'; document.querySelector('input[name=\\'timer1_m\\']').value = '0'; document.querySelector('input[name=\\'timer1_s\\']').value = '0';\">Set 0</button>"
              "</div>"
              // Timer 2
              "<div class='timer-slot'>"
              "<label>Time 2:</label>"
              "<input type='number' name='timer2_h' min='0' max='23' placeholder='HH' value='" + getHour(timer[1]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer2_m' min='0' max='59' placeholder='MM' value='" + getMinute(timer[1]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer2_s' min='0' max='59' placeholder='SS' value='" + getSecond(timer[1]) + "'>"
              "<div class='controls'>"
              "<button type='submit'>Set Timers</button> "
              "</div>"
              "<button type='button' onclick=\"document.querySelector('input[name=\\'timer2_h\\']').value = '0'; document.querySelector('input[name=\\'timer2_m\\']').value = '0'; document.querySelector('input[name=\\'timer2_s\\']').value = '0';\">Set 0</button>"
              "</div>"
              // Timer 3
              "<div class='timer-slot'>"
              "<label>Time 3:</label>"
              "<input type='number' name='timer3_h' min='0' max='23' placeholder='HH' value='" + getHour(timer[2]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer3_m' min='0' max='59' placeholder='MM' value='" + getMinute(timer[2]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer3_s' min='0' max='59' placeholder='SS' value='" + getSecond(timer[2]) + "'>"
              "<div class='controls'>"
              "<button type='submit'>Set Timers</button> "
              "</div>"
              "<button type='button' onclick=\"document.querySelector('input[name=\\'timer3_h\\']').value = '0'; document.querySelector('input[name=\\'timer3_m\\']').value = '0'; document.querySelector('input[name=\\'timer3_s\\']').value = '0';\">Set 0</button>"
              "</div>"
              // Timer 4
              "<div class='timer-slot'>"
              "<label>Time 4:</label>"
              "<input type='number' name='timer4_h' min='0' max='23' placeholder='HH' value='" + getHour(timer[3]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer4_m' min='0' max='59' placeholder='MM' value='" + getMinute(timer[3]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer4_s' min='0' max='59' placeholder='SS' value='" + getSecond(timer[3]) + "'>"
              "<div class='controls'>"
              "<button type='submit'>Set Timers</button> "
              "</div>"
              "<button type='button' onclick=\"document.querySelector('input[name=\\'timer4_h\\']').value = '0'; document.querySelector('input[name=\\'timer4_m\\']').value = '0'; document.querySelector('input[name=\\'timer4_s\\']').value = '0';\">Set 0</button>"
              "</div>"
              // Timer 5
              "<div class='timer-slot'>"
              "<label>Time 5:</label>"
              "<input type='number' name='timer5_h' min='0' max='23' placeholder='HH' value='" + getHour(timer[4]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer5_m' min='0' max='59' placeholder='MM' value='" + getMinute(timer[4]) + "'>"
              "<span class='separator'>:</span>"
              "<input type='number' name='timer5_s' min='0' max='59' placeholder='SS' value='" + getSecond(timer[4]) + "'>"
              "<div class='controls'>"
              "<button type='submit'>Set Timers</button> "
              "</div>"
              "<button type='button' onclick=\"document.querySelector('input[name=\\'timer5_h\\']').value = '0'; document.querySelector('input[name=\\'timer5_m\\']').value = '0'; document.querySelector('input[name=\\'timer5_s\\']').value = '0';\">Set 0</button>"
              "</div>"
              "<div class='controls'>"
              "<button type='submit'>Set Timers</button> "
              "</div>"
              "</form>"
              "</div><br>"


              "</body></html>";
  server.send(200, "text/html", html);
}

// Helper functions to extract HH, MM, SS from the time string
String getHour(String timeStr) {
  if (timeStr.length() >= 2) {
    return timeStr.substring(0, timeStr.indexOf(':'));
  }
  return "";
}

String getMinute(String timeStr) {
  int firstColon = timeStr.indexOf(':');
  if (firstColon != -1) {
    int secondColon = timeStr.indexOf(':', firstColon + 1);
    if (secondColon != -1) {
      return timeStr.substring(firstColon + 1, secondColon);
    }
  }
  return "";
}

String getSecond(String timeStr) {
  int firstColon = timeStr.indexOf(':');
  if (firstColon != -1) {
    int secondColon = timeStr.indexOf(':', firstColon + 1);
    if (secondColon != -1) {
      return timeStr.substring(secondColon + 1);
    }
  }
  return "";
}






void handleSetTimersApp() { Serial.println("Received timer from app:");

if (server.hasArg("timer0")) {
    timer[0] = server.arg("timer0");
    if (timer[0]==0) {isAlarmSet1 = false;}
    else{
    Serial.print("Timer 1 set to: "); Serial.println(timer[0]);
    isAlarmSet = true;
    isAlarmSet1 = true;}
    
}
if (server.hasArg("timer1")) {
    timer[1] = server.arg("timer1");
    if (timer[1]==0) {isAlarmSet2 = false;}
    else{
    Serial.print("Timer 2 set to: "); Serial.println(timer[1]);
    isAlarmSet = true;
    isAlarmSet2 = true;}
    
}
if (server.hasArg("timer2")) {
    timer[2] = server.arg("timer2");
    if (timer[2]==0) {isAlarmSet3 = false;}
    else{
    Serial.print("Timer 3 set to: "); Serial.println(timer[2]);
    isAlarmSet = true;
    isAlarmSet3 = true;}
}
if (server.hasArg("timer3")) {
    timer[3] = server.arg("timer3");
    if (timer[3]==0) {isAlarmSet2 = false;}
    else{
    Serial.print("Timer 4 set to: "); Serial.println(timer[3]);
    isAlarmSet = true;
    isAlarmSet4 = true;}
}
if (server.hasArg("timer4")) {
    timer[4] = server.arg("timer4");
    if (timer[4]==0) {isAlarmSet2 = false;}
    else{
    Serial.print("Timer 5 set to: "); Serial.println(timer[4]);
    isAlarmSet = true;
    isAlarmSet5 = true;}
 }

 
      server.sendHeader("Location", "/");
      server.send(302, "text/plain", "");
      
}











void handleSetTimersWeb() {
  Serial.println("Received timer from web:");

  String hourStr;
  String minuteStr;
  String secondStr;
  String formattedHour;
  String formattedMinute;
  String formattedSecond;

  // Timer 1
  hourStr = server.arg("timer1_h");
  minuteStr = server.arg("timer1_m");
  secondStr = server.arg("timer1_s");
  if (hourStr.length() > 0 && minuteStr.length() > 0 && secondStr.length() > 0) {
    if (hourStr == "0" && minuteStr == "0" && secondStr == "0") {
      timer[0] = "";
      isAlarmSet1 = false;
      Serial.println("Timer 1 disabled.");
    } else {
      int hour = hourStr.toInt();
      formattedHour = (hour < 10 ? "0" : "") + String(hour);
      int minute = minuteStr.toInt();
      formattedMinute = (minute < 10 ? "0" : "") + String(minute);
      int second = secondStr.toInt();
      formattedSecond = (second < 10 ? "0" : "") + String(second);
      timer[0] = formattedHour + ":" + formattedMinute + ":" + formattedSecond;
      Serial.print("Timer 1 set to: "); Serial.println(timer[0]);
      isAlarmSet = true;
      isAlarmSet1 = true;
    }
  } else {
    timer[0] = "";
    isAlarmSet1 = false;
  }

  // Timer 2
  hourStr = server.arg("timer2_h");
  minuteStr = server.arg("timer2_m");
  secondStr = server.arg("timer2_s");
  if (hourStr.length() > 0 && minuteStr.length() > 0 && secondStr.length() > 0) {
    if (hourStr == "0" && minuteStr == "0" && secondStr == "0") {
      timer[1] = "";
      isAlarmSet2 = false;
      Serial.println("Timer 2 disabled.");
    } else {
      int hour = hourStr.toInt();
      formattedHour = (hour < 10 ? "0" : "") + String(hour);
      int minute = minuteStr.toInt();
      formattedMinute = (minute < 10 ? "0" : "") + String(minute);
      int second = secondStr.toInt();
      formattedSecond = (second < 10 ? "0" : "") + String(second);
      timer[1] = formattedHour + ":" + formattedMinute + ":" + formattedSecond;
      Serial.print("Timer 2 set to: "); Serial.println(timer[1]);
      isAlarmSet = true;
      isAlarmSet2 = true;
    }
  } else {
    timer[1] = "";
    isAlarmSet2 = false;
  }

  // Timer 3
  hourStr = server.arg("timer3_h");
  minuteStr = server.arg("timer3_m");
  secondStr = server.arg("timer3_s");
  if (hourStr.length() > 0 && minuteStr.length() > 0 && secondStr.length() > 0) {
    if (hourStr == "0" && minuteStr == "0" && secondStr == "0") {
      timer[2] = "";
      isAlarmSet3 = false;
      Serial.println("Timer 3 disabled.");
    } else {
      int hour = hourStr.toInt();
      formattedHour = (hour < 10 ? "0" : "") + String(hour);
      int minute = minuteStr.toInt();
      formattedMinute = (minute < 10 ? "0" : "") + String(minute);
      int second = secondStr.toInt();
      formattedSecond = (second < 10 ? "0" : "") + String(second);
      timer[2] = formattedHour + ":" + formattedMinute + ":" + formattedSecond;
      Serial.print("Timer 3 set to: "); Serial.println(timer[2]);
      isAlarmSet = true;
      isAlarmSet3 = true;
    }
  } else {
    timer[2] = "";
    isAlarmSet3 = false;
  }

  // Timer 4
  hourStr = server.arg("timer4_h");
  minuteStr = server.arg("timer4_m");
  secondStr = server.arg("timer4_s");
  if (hourStr.length() > 0 && minuteStr.length() > 0 && secondStr.length() > 0) {
    if (hourStr == "0" && minuteStr == "0" && secondStr == "0") {
      timer[3] = "";
      isAlarmSet4 = false;
      Serial.println("Timer 4 disabled.");
    } else {
      int hour = hourStr.toInt();
      formattedHour = (hour < 10 ? "0" : "") + String(hour);
      int minute = minuteStr.toInt();
      formattedMinute = (minute < 10 ? "0" : "") + String(minute);
      int second = secondStr.toInt();
      formattedSecond = (second < 10 ? "0" : "") + String(second);
      timer[3] = formattedHour + ":" + formattedMinute + ":" + formattedSecond;
      Serial.print("Timer 4 set to: "); Serial.println(timer[3]);
      isAlarmSet = true;
      isAlarmSet4 = true;
    }
  } else {
    timer[3] = "";
    isAlarmSet4 = false;
  }

  // Timer 5
  hourStr = server.arg("timer5_h");
  minuteStr = server.arg("timer5_m");
  secondStr = server.arg("timer5_s");
  if (hourStr.length() > 0 && minuteStr.length() > 0 && secondStr.length() > 0) {
    if (hourStr == "0" && minuteStr == "0" && secondStr == "0") {
      timer[4] = "";
      isAlarmSet5 = false;
      Serial.println("Timer 5 disabled.");
    } else {
      int hour = hourStr.toInt();
      formattedHour = (hour < 10 ? "0" : "") + String(hour);
      int minute = minuteStr.toInt();
      formattedMinute = (minute < 10 ? "0" : "") + String(minute);
      int second = secondStr.toInt();
      formattedSecond = (second < 10 ? "0" : "") + String(second);
      timer[4] = formattedHour + ":" + formattedMinute + ":" + formattedSecond;
      Serial.print("Timer 5 set to: "); Serial.println(timer[4]);
      isAlarmSet = true;
      isAlarmSet5 = true;
    }
  } else {
    timer[4] = "";
    isAlarmSet5 = false;
  }

  // Redirect back to the root
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}












void handleSensors() {
  String jsonResponse = "{";
  jsonResponse += "\"rtc\": \"" + rtcDisplayData2 + "\",";
  jsonResponse += "\"dht\": \"" + dhtDisplayData + "\",";
  jsonResponse += "\"moisture\": \"" + moistureDisplayData + "\",";
  jsonResponse += "\"ultrasonic\": \"" + ultrasonicDisplayData + "\"";
  jsonResponse += "}";
  server.send(200, "application/json", jsonResponse);
}






void handleSetLCDText() {
  if (server.hasArg("text")) {
    receivedLCDText = server.arg("text");
    Serial.print("Received LCD text from web: ");
    Serial.println(receivedLCDText);

    // Send '3' as a command, then the actual text to Arduino
    arduinoSerial.print("3");
    arduinoSerial.println(receivedLCDText);

    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");    
  } else {
    server.send(400, "text/plain", "Error: 'text' argument not found.");
  }
}




void handleLedOn()  {
  Serial.println("Web request: LED ON"); // Debugging output
  arduinoSerial.println("1");
  

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", ""); 
}






void handleLedOff() {
  Serial.println("Web request: LED OFF"); // Debugging output
  arduinoSerial.println("0");

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", ""); 
}





void handlePumpOn() {
  Serial.println("Web request: PUMP ON"); // Debugging output
  arduinoSerial.println("2");
  
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}







void handlePumpOff(){
  Serial.println("Web request: PUMP OFF"); // Debugging output
  arduinoSerial.println("9");
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}




void handleValve1On() {
  Serial.println("Web request: Valve 1 On"); // Debugging output
  arduinoSerial.println("5");
 
   
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}



void handleValve1Off() {
  Serial.println("Web request: Valve 1 Off"); // Debugging output
  arduinoSerial.println("t");
   
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}



void handleValve2On() {
  Serial.println("Web request: Valve 2 On"); // Debugging output
  arduinoSerial.println("6");
   
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}



void handleValve2Off() {
  Serial.println("Web request: Valve 2 Off"); // Debugging output
  arduinoSerial.println("y");

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}



void handleValve3On() {
  Serial.println("Web request: Valve 3 On"); // Debugging output
  arduinoSerial.println("7");
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}



void handleValve3Off() {
  Serial.println("Web request: Valve 3 Off"); // Debugging output
  arduinoSerial.println("u");
   
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}







// --- SERIAL COMMUNICATION ---
void SerialCommands() {
  if (Serial.available()) {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim();
    if (inputString.startsWith("3") && inputString.length() > 1) {
      String textToDisplay = inputString.substring(1); // Extract the text after '3'
      Serial.print("Sending LCD text to Arduino via serial: ");
      Serial.println(textToDisplay);
      arduinoSerial.print("3"); // Send a '3' command to Arduino
      arduinoSerial.println(textToDisplay); // Then send the actual text
    } else if (inputString == "1") {
      arduinoSerial.println("1");
      arduinoSerial.print("3");
      arduinoSerial.println("Relay 1 on");
    } else if (inputString == "0") {
      arduinoSerial.println("0");
      arduinoSerial.print("3");
      arduinoSerial.println("Relay 1 off");      
    } else if (inputString == "2") {
      arduinoSerial.println("2");
      arduinoSerial.print("3");
      arduinoSerial.println("Relay 2 on");      
    } else if (inputString == "9") {
      arduinoSerial.println("9");
      arduinoSerial.print("3");
      arduinoSerial.println("Relay 2 off");      
    } else if (inputString == "5") {
      arduinoSerial.println("5");
    } else if (inputString == "t") {
      arduinoSerial.println("t");
    } else if (inputString == "6") {
      arduinoSerial.println("6");
    } else if (inputString == "y") {
      arduinoSerial.println("y");
    } else if (inputString == "7") {
      arduinoSerial.println("7");
    } else if (inputString == "u") {
      arduinoSerial.println("u");
    }
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
        rtcDisplayData1 = "Date: " + datePart + "\nTime: " + timePart;
        rtcDisplayData2 = "Time: " + timePart;
        
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
        dhtDisplayData = "Temp: " + temp + " C\nHum: " + humidity + " %";
        
        
      }
    } else if (data.startsWith("MOISTURE,")) {
      int c1 = data.indexOf(',');
      if (c1 > 0) {
        String moisture = data.substring(c1 + 1);
        currentMoisture = moisture.toInt();
        moistureDisplayData = "Moisture: " + moisture + " %";
        
        
    }
   } else if (data.startsWith("ULTRASONIC,")) { // New block for Ultrasonic data
      int c1 = data.indexOf(',');
      if (c1 > 0) {
        String distance = data.substring(c1 + 1);
        currentUltrasonicDistance = distance.toFloat(); // Store the distance in a new variable
        ultrasonicDisplayData = "Distance: " + distance + " cm";
      }
    } 
  } 
 } 






void checkAlarmCondition() {
  if (isAlarmSet && currentHour >= 0 && currentMinute >= 0 && currentSecond >= 0) {
    
    String currentTime = (currentHour < 10 ? "0" : "") + String(currentHour) + ":" +
                           (currentMinute < 10 ? "0" : "") + String(currentMinute) + ":" +
                           (currentSecond < 10 ? "0" : "") + String(currentSecond);
    
    
    
    if (alarmTargetTime.length() == 8) {
        if (currentTime == alarmTargetTime) {
           Serial.println("Pump ON");
           arduinoSerial.println("2"); // Pump ON
           isPumpActive = true;
           pumpStartTime = millis();}}
      
    
  if (isAlarmSet1) {
    if (currentTime == timer[0]) {
        Serial.println("Pump ON");
        arduinoSerial.println("2"); // Pump ON
        isPumpActive = true;
        pumpStartTime = millis();}}

 if (isAlarmSet2) {
    if (currentTime == timer[1]) {
        Serial.println("Pump ON");
        arduinoSerial.println("2"); // Pump ON
        isPumpActive = true;
        pumpStartTime = millis();}}

if (isAlarmSet3) {
    if (currentTime == timer[2]) {
        Serial.println("Pump ON");
        arduinoSerial.println("2"); // Pump ON
        isPumpActive = true;
        pumpStartTime = millis();}}

if (isAlarmSet4) {
      if (currentTime == timer[3]) {
        Serial.println("Pump ON");
        arduinoSerial.println("2"); // Pump ON
        isPumpActive = true;
        pumpStartTime = millis();}}

if (isAlarmSet5) {
     if (currentTime == timer[4]) {
        Serial.println("Pump ON");
        arduinoSerial.println("2"); // Pump ON
        isPumpActive = true;
        pumpStartTime = millis();}}
   
  
      
  }
}






void controlPumpAction() {
  if (isPumpActive && millis() - pumpStartTime >= PUMP_DURATION_MS) {
    arduinoSerial.println("9"); // Pump OFF
    Serial.println("Pump OFF after duration");
    isPumpActive = false;
    pumpStartTime = millis();
    
  }
}
