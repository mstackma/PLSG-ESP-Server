// Editing of the ESP32 Server - SPIFFS - version:
/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-input-data-html-form/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/
#include <Arduino.h>

#define ledPinLeft 2
#define ledPinRight 15
#define motorLeftPin1 32
#define motorLeftPin2 33
#define motorRightPin1 25
#define motorRightPin2 26
#define buttonPin 4
#define triggerPin 19
#define echoPin 21

#define enablerObstacleSensorPin 5
#define obstacleSensorPin 18
#define lightSensorPin1 34
#define lightSensorPin2 35
// define sound speed in cm/uS
#define SOUND_SPEED 0.034

long durationSoundWave;
const int numDistanceReadings = 5;      // numbers of distance-readings
int distanceArray[numDistanceReadings]; // distance array for calculating the average to eliminate short incorrect distance values
int readIndex = 0;                      // Array-Index
int averageDistance = 0;

unsigned long startTime = 0;
unsigned long durationButtonClick = 0;

volatile unsigned long timeoutTimer = millis();

int analogLightValueLeft;
int analogLightValueRight;

// Variables will change:
int onOff = LOW;                        // robot motor, led state: on/HIGH or off/LOW
int appControl = LOW;                   // the current state: App-controlled or autonomous robot
int buttonState = 0;                    // the current reading from the input buttonPin
int lastButtonState = 0;                // the previous reading from the input buttonPin
int distance = 0;                       // distance cm
int motorLeftF;                         // motorLeftForward Value
int motorRightF;                        // motorRightForward Value
int motorLeftB;                         // motorLeftBackward Value
int motorRightB;                        // motorRightBackward Value
int thresholdLight;                     // thresholdLight Value
int delayBackwardsDrive;                // delay in milliseconds of reversing after an obstacle
int ledLeft;                            // ledLeft Value
int ledRight;                           // ledRight Value
int inputLedLeft;                       // App Input ledLeft Value
int inputLedRight;                      // App Input ledRight Value
int inputMotorLeftF;                    // App Input motorLeftForward Value
int inputMotorRightF;                   // App Input motorRightForward Value
int inputMotorLeftB;                    // App Input motorLeftBackward Value
int inputMotorRightB;                   // App Input motorRightBackward Value
int inputThresholdLight;                // App Input thresholdLight Value
int inputDelayBackwardsDrive;           // App Input delayBackwardsDrive Value
String lightSensorStatus;               // lightSensorStatus On Off
String ultraSonicSensorStatus;          // ultraSonicSensorStatus On Off
String obstacleSensorStatus;            // obstacleSensorStatus On Off
String motorSensorConnection = "cross"; // sensorMotorConnection "cross" or "parallel"

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#else
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <FS.h>
#endif
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char *ssid = "ssid";
const char *password = "password";

const char *PARAM_INT1 = "inputMotorLeftF";                                // inputMotorLeftF
const char *PARAM_INT2 = "inputMotorRightF";                               // inputMotorRightF
const char *PARAM_INT3 = "inputMotorLeftB";                                // inputMotorLeftB
const char *PARAM_INT4 = "inputMotorRightB";                               // inputMotorRightB
const char *PARAM_INT5 = "inputLedLeft";                                   // inputLedLeft
const char *PARAM_INT6 = "inputLedRight";                                  // inputLedRight
const char *PARAM_INT7 = "inputThresholdLight";                            // inputThresholdLight
const char *PARAM_INT8 = "inputDelayBackwardsDrive";                       // inputDelayBackwardsDrive
const char *PARAM_WEB_BUTTON_LIGHT_SENSOR = "lightSensorStatus";           // lightSensorStatus "On" or "Off"
const char *PARAM_WEB_BUTTON_ULTRASONIC_SENSOR = "ultraSonicSensorStatus"; // ultraSonicSensorStatus "On" or "Off"

const char *PARAM_WEB_BUTTON_OBSTACLE_SENSOR = "obstacleSensorStatus"; // obstacleSensorStatus "On" or "Off"
const char *PARAM_WEB_BUTTON_CONNECTIONTYPE = "motorSensorConnection"; // motorSensorConnection "cross" or "parallel"
const char *PARAM_BUTTON_APPCONTROL = "state";                         // appControl state (web and real button)

// HTML web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Robot Control Center</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 2.0rem;}
    body {max-width: 700px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>Robot Control Center</h2>
  %BUTTONPLACEHOLDER%
  <br><br><br>  
  <iframe style="display:none" name="hidden-form"></iframe>
<form action="/put" target="hidden-form">
    inputMotorLeftF (current value %inputMotorLeftF%): <input type="number" name="inputMotorLeftF" min="0" max="255">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/put" target="hidden-form">
    inputMotorLeftB (current value %inputMotorLeftB%): <input type="number" name="inputMotorLeftB" min="0" max="255">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/put" target="hidden-form">
    inputMotorRightF (current value %inputMotorRightF%): <input type="number" name="inputMotorRightF" min="0" max="255">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/put" target="hidden-form">
    inputMotorRightB (current value %inputMotorRightB%): <input type="number" name="inputMotorRightB" min="0" max="255">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/put" target="hidden-form">
    inputLedLeft (current value %inputLedLeft%): <input type="number" name="inputLedLeft" min="0" max="255">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/put" target="hidden-form">
    inputLedRight (current value %inputLedRight%): <input type="number" name="inputLedRight" min="0" max="255">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/put" target="hidden-form">
    inputThresholdLight (current value %inputThresholdLight%): <input type="number" name="inputThresholdLight" min="0" max="4095">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/put" target="hidden-form">
    inputDelayBackwardsDrive in milliseconds (current value %inputDelayBackwardsDrive%): <input type="number" name="inputDelayBackwardsDrive" min="0">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/put" target="hidden-form">
    lightSensorStatus %lightSensorStatus% <input type="button" value="LightSensor" id="lightSensor">
  </form><br>
  <form action="/put" target="hidden-form">
    ultraSonicSensorStatus %ultraSonicSensorStatus% <input type="button" value="UltraSonicSensor" id="ultraSonicSensor">
  </form><br>
  <form action="/put" target="hidden-form">
    obstacleSensorStatus %obstacleSensorStatus% <input type="button" value="ObstacleSensor" id="obstacleSensorPin">
  </form><br>
  <form action="/put" target="hidden-form">
    motorSensorConnection %motorSensorConnection% <input type="button" value="motorSensorConnection" id="connection">
  </form><br>
    <span>Brightness Sensor Left Value</span> 
    <span id="lightL">%LIGHTL%</span>
  <br><br>
    <span>Brightness Sensor Right Value</span> 
    <span id="lightR">%LIGHTR%</span>
	<br><br>
    <span>ultraSonicDistance</span> 
    <span id="ultraSonicDistance">%ULTRADISTANCE%</span>
    <span>cm</span>
  <br><br>
    <span>isObstacle</span> 
    <span id="isObstacle">%ISOBSTACLE%</span>
  <br><br>
  <form action="/put" target="hidden-form">
    <input type="button" value="timerResetter" id="timerResetter">
  </form><br>
</body>
<script>
document.getElementById("lightSensor").addEventListener("click", function() { buttonClick("lightSensorStatus=1");}, false);
document.getElementById("ultraSonicSensor").addEventListener("click", function() { buttonClick("ultraSonicSensorStatus=1");}, false);
document.getElementById("obstacleSensorPin").addEventListener("click", function() { buttonClick("obstacleSensorStatus=1");}, false);
document.getElementById("connection").addEventListener("click", function() { buttonClick("motorSensorConnection=1");}, false);
document.getElementById("timerResetter").addEventListener("click", function() { buttonClick("timerResetter=1");}, false);

function buttonClick(sensorTypeStatus) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/put?" + sensorTypeStatus, true);
  xhr.onreadystatechange = function () {
    if (xhr.readyState == 4 && xhr.status == 200 && !(sensorTypeStatus == "timerResetter=1")) {
      // alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);
    }
  }
  xhr.send();
}

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("appControl").checked = inputChecked;
      document.getElementById("appControlState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;

function submitMessage() {
      // alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }

fetchData("lightL", "/lightL");
fetchData("lightR", "/lightR");
fetchData("ultraSonicDistance", "/ultraSonicDistance");
fetchData("isObstacle", "/isObstacle");

function fetchData(elementId, url) {
  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById(elementId).innerHTML = this.responseText;
        if (elementId == "ultraSonicDistance" || elementId == "isObstacle") {
          var sensorValue = parseFloat(this.responseText);
          if ((elementId == "ultraSonicDistance" && sensorValue < 7) || (elementId == "isObstacle" && sensorValue == 0) ) {
            document.getElementById(elementId).style.backgroundColor = "red";
          } else {
            document.getElementById(elementId).style.backgroundColor = "white";
          }
        }
      }
    };
    xhttp.open("GET", url, true);
    xhttp.send();
  }, 500);
}

</script>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char *path)
{
  // Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  // Serial.println("- read from file:");
  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
  file.close();
  // Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- write failed");
  }
  file.close();
}

String appControlState()
{
  if (appControl)
  {
    return "checked";
  }
  else
  {
    return "";
  }
  return "";
}

int readUltrasonicDistanceInCm() // int is only necessary for 3 pin ultraSonicSensor
{
  int distanceCm;
  if (ultraSonicSensorStatus == "On")
  {
    pinMode(triggerPin, OUTPUT); // only necessary for 3 Pin ultraSonicSensor
    // Clear the trigger
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);

    // Sets the trigger pin to HIGH state for 10 microseconds
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    pinMode(echoPin, INPUT); // only necessary for 3 Pin ultraSonicSensor
    // Reads the echoPin, returns the sound wave travel time in microseconds
    durationSoundWave = pulseIn(echoPin, HIGH);

    // Calculate the distance
    distanceCm = durationSoundWave * SOUND_SPEED / 2;

    // Calculate the averageDistance to eliminate short incorrect distance values
    if (distanceCm > 0 && distanceCm < 250)
    {
      distanceArray[readIndex] = distanceCm;
      readIndex = (readIndex + 1) % numDistanceReadings;

      int sum = 0;
      for (int i = 0; i < numDistanceReadings; i++)
      {
        sum += distanceArray[i];
      }
      averageDistance = sum / numDistanceReadings;
    }
  }
  else
  {
    averageDistance = 0;
  }
  return averageDistance;
}

bool obstacleCheck()
{
  bool obstacleSensorValue;
  if (obstacleSensorStatus == "On")
  {
    digitalWrite(enablerObstacleSensorPin, HIGH); // activates obstacleSensorPin
    obstacleSensorValue = digitalRead(obstacleSensorPin);
  }
  else
  {
    digitalWrite(enablerObstacleSensorPin, LOW);
    obstacleSensorValue = HIGH; // HIGH = no obstacle
  }
  return obstacleSensorValue;
}

// Replaces placeholder with stored values
String processor(const String &var)
{
  // Serial.println(var);
  if (var == "inputMotorLeftF")
  {
    return readFile(SPIFFS, "/inputMotorLeftF.txt");
  }
  else if (var == "inputMotorRightF")
  {
    return readFile(SPIFFS, "/inputMotorRightF.txt");
  }
  else if (var == "inputMotorLeftB")
  {
    return readFile(SPIFFS, "/inputMotorLeftB.txt");
  }
  else if (var == "inputMotorRightB")
  {
    return readFile(SPIFFS, "/inputMotorRightB.txt");
  }
  else if (var == "inputLedLeft")
  {
    return readFile(SPIFFS, "/inputLedLeft.txt");
  }
  else if (var == "inputLedRight")
  {
    return readFile(SPIFFS, "/inputLedRight.txt");
  }
  else if (var == "inputThresholdLight")
  {
    return readFile(SPIFFS, "/inputThresholdLight.txt");
  }
  else if (var == "inputDelayBackwardsDrive")
  {
    return readFile(SPIFFS, "/inputDelayBackwardsDrive.txt");
  }
  else if (var == "lightSensorStatus")
  {
    return readFile(SPIFFS, "/lightSensorStatus.txt");
  }
  else if (var == "ultraSonicSensorStatus")
  {
    return readFile(SPIFFS, "/ultraSonicSensorStatus.txt");
  }
  else if (var == "obstacleSensorStatus")
  {
    return readFile(SPIFFS, "/obstacleSensorStatus.txt");
  }
  else if (var == "motorSensorConnection")
  {
    return readFile(SPIFFS, "/motorSensorConnection.txt");
  }
  else if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    String appControlStateValue = appControlState();
    // HTML text to display the button with the right state:
    buttons += "<h4>appControl - State <span id=\"appControlState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"appControl\" " + appControlStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  else if (var == "LIGHTL")
  {
    // reads the input on analog pin (value between 0 and 4095)
    analogLightValueLeft = analogRead(lightSensorPin1);
    return String(analogLightValueLeft);
  }
  else if (var == "LIGHTR")
  {
    // reads the input on analog pin (value between 0 and 4095)
    analogLightValueRight = analogRead(lightSensorPin2);
    return String(analogLightValueRight);
  }
  else if (var == "ULTRADISTANCE")
  {
    return String(distance);
  }
  else if (var == "ISOBSTACLE")
  {
    return String(obstacleCheck());
  }
  return String();
}

void handleClick()
{
  if (buttonState != lastButtonState)
  {
    if (buttonState == HIGH)
    {
      startTime = millis();
    }
    else
    {
      durationButtonClick = millis() - startTime;
      if (durationButtonClick < 500)
      { // short button press
        appControl = !appControl;
        Serial.println(appControl);
        onOff = HIGH;
        Serial.println("-----------------------------short------------------------------------");
        Serial.println("lightSensorStatus");
        Serial.println(lightSensorStatus);
        Serial.println("ultraSonicSensorStatus");
        Serial.println(ultraSonicSensorStatus);
        Serial.println("obstacleSensorStatus");
        Serial.println(obstacleSensorStatus);
        Serial.println("motorSensorConnection");
        Serial.println(motorSensorConnection);
      }
      else
      { // long button press
        appControl = LOW;
        onOff = LOW;
        Serial.println("-----------------------------long-------------------------------------");
      }
    }
  }
  lastButtonState = buttonState;
}

// handle the motor depending on the motor light sensor connection
void braitenbergVehicleTwo(String cross_or_parallel)
{
  motorLeftB = 0;
  motorRightB = 0;
  if (cross_or_parallel == "cross")
  {
    motorLeftF = map(analogLightValueRight, thresholdLight, 4095, 70, 255);
    motorRightF = map(analogLightValueLeft, thresholdLight, 4095, 70, 255);
  }
  else if (cross_or_parallel == "parallel")
  {
    motorLeftF = map(analogLightValueLeft, thresholdLight, 4095, 70, 255);
    motorRightF = map(analogLightValueRight, thresholdLight, 4095, 70, 255);
  }
}

void stopMotor()
{
  motorLeftF = 0;
  motorLeftB = 0;
  motorRightF = 0;
  motorRightB = 0;
}

void getStoredSPIFFSValues()
{
  // Access stored values on motor, light sensor status, ultrasonic sensor status
  inputMotorLeftF = readFile(SPIFFS, "/inputMotorLeftF.txt").toInt();
  // Serial.print("*** Your inputMotorLeftF: ");
  // Serial.println(inputMotorLeftF);

  inputMotorLeftB = readFile(SPIFFS, "/inputMotorLeftB.txt").toInt();
  // Serial.print("*** Your inputMotorLeftB: ");
  // Serial.println(inputMotorLeftB);

  inputMotorRightF = readFile(SPIFFS, "/inputMotorRightF.txt").toInt();
  // Serial.print("*** Your inputMotorRightF: ");
  // Serial.println(inputMotorRightF);

  inputMotorRightB = readFile(SPIFFS, "/inputMotorRightB.txt").toInt();
  // Serial.print("*** Your inputMotorRightB: ");
  // Serial.println(inputMotorRightB);
  
  inputLedLeft = readFile(SPIFFS, "/inputLedLeft.txt").toInt();
  // Serial.print("*** Your inputLedLeft: ");
  // Serial.println(inputLedLeft);
  
  inputLedRight = readFile(SPIFFS, "/inputLedRight.txt").toInt();
  // Serial.print("*** Your inputLedRight: ");
  // Serial.println(inputLedRight);

  inputThresholdLight = readFile(SPIFFS, "/inputThresholdLight.txt").toInt();
  // Serial.print("*** Your inputThresholdLight: ");
  // Serial.println(inputThresholdLight);
  thresholdLight = inputThresholdLight;

  inputDelayBackwardsDrive = readFile(SPIFFS, "/inputDelayBackwardsDrive.txt").toInt();
  // Serial.print("*** Your inputDelayBackwardsDrive: ");
  // Serial.println(inputDelayBackwardsDrive);
  delayBackwardsDrive = inputDelayBackwardsDrive;

  lightSensorStatus = readFile(SPIFFS, "/lightSensorStatus.txt");
  ultraSonicSensorStatus = readFile(SPIFFS, "/ultraSonicSensorStatus.txt");
  obstacleSensorStatus = readFile(SPIFFS, "/obstacleSensorStatus.txt");
  motorSensorConnection = readFile(SPIFFS, "/motorSensorConnection.txt");
}

void updateMotorLed()
{
  analogWrite(ledPinLeft, ledLeft);
  analogWrite(ledPinRight, ledRight);
  analogWrite(motorLeftPin1, motorLeftF);
  analogWrite(motorLeftPin2, motorLeftB);
  analogWrite(motorRightPin1, motorRightF);
  analogWrite(motorRightPin2, motorRightB);
}

void handleMotorLed()
{
  ledLeft = inputLedLeft;
  ledRight = inputLedRight;
  /*
  It is important to run the readUltrasonicDistanceInCm() at regular intervals,
  as otherwise incorrect distance values may result!
  Therefore, only the distance is passed on here.
  The calculation takes place via the regular http_get request to display the values in the web browser.
  take a look at html JS Code: ...xhttp.open("GET", url, true);
    xhttp.send();
  }, 600);}
  -> 600 ms interval
  and cpp setup():
  server.on("/ultraSonicDistance", HTTP_GET, [](AsyncWebServerRequest *request)
            { distance = readUltrasonicDistanceInCm(); ...
  */
  // if (onOff == 1 && distance < 7 && ultraSonicSensorStatus == "On") // [&& ultraSonicSensorStatus == "On"] is important, because if ultraSonicSensorStatus is off, the distance is 0, so this if would always be true
  bool obstacleSensorValue = obstacleCheck();
  if (onOff == 1 && obstacleSensorValue == LOW)
  {
    // !!! Implement here: Send signal: "Obstacle !!!" !!!

    // Serial.println(distanceCm);
    motorLeftF = 0;
    motorLeftB = 200;
    motorRightF = 0;
    motorRightB = 200;
    ledLeft = 255;
    ledRight = 255;
    updateMotorLed();
    delay(delayBackwardsDrive); // time that is driven backwards

    getStoredSPIFFSValues();
  }
  if (onOff == 1 && appControl == 1)
  {
    motorLeftF = inputMotorLeftF;
    motorLeftB = inputMotorLeftB;
    motorRightF = inputMotorRightF;
    motorRightB = inputMotorRightB;
  }
  else if (onOff == 1 && appControl == 0 && lightSensorStatus == "On")
  {
    // reads the light input on analog pin (value between 0 and 4095)
    analogLightValueLeft = analogRead(lightSensorPin1);
    analogLightValueRight = analogRead(lightSensorPin2);
    ledLeft = map(analogLightValueLeft, thresholdLight, 4095, 10, 255);
    ledRight = map(analogLightValueRight, thresholdLight, 4095, 10, 255);

    if (analogLightValueRight > thresholdLight || analogLightValueLeft > thresholdLight)
    {
      braitenbergVehicleTwo(motorSensorConnection); // Forward according to brightness and sensor-motor connection
    }
    else
    {
      stopMotor();
    }
  }
  else if (onOff == 0)
  {
    stopMotor();
  }
}

void clientCheckTimeout(int timeoutInSeconds)
{
  if (millis() - timeoutTimer > timeoutInSeconds * 1000)
  {
    appControl = LOW;
    Serial.println("Client request not received => timeout");
    timeoutTimer = millis();
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(obstacleSensorPin, INPUT);
  pinMode(enablerObstacleSensorPin, OUTPUT);

  pinMode(ledPinLeft, OUTPUT);
  pinMode(ledPinRight, OUTPUT);
  pinMode(motorLeftPin1, OUTPUT);
  pinMode(motorLeftPin2, OUTPUT);
  pinMode(motorRightPin1, OUTPUT);
  pinMode(motorRightPin2, OUTPUT);

  pinMode(buttonPin, INPUT);

// Initialize SPIFFS
#ifdef ESP32
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
#else
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  // Send a GET request to <ESP_IP>/get?inputMotorLeftF
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String outputMessage;
    // GET inputMotorLeftF
    if (request->hasParam(PARAM_INT1)) {
      String currentValue = readFile(SPIFFS, "/inputMotorLeftF.txt");
      outputMessage = "inputMotorLeftF: ";
      outputMessage += currentValue;
    }
    // GET inputMotorLeftB
    else if (request->hasParam(PARAM_INT3)) {
      String currentValue = readFile(SPIFFS, "/inputMotorLeftB.txt");
      outputMessage = "inputMotorLeftB: ";
      outputMessage += currentValue;
    }
    // GET inputMotorRightF
    else if (request->hasParam(PARAM_INT2)) {
      String currentValue = readFile(SPIFFS, "/inputMotorRightF.txt");
      outputMessage = "inputMotorRightF: ";
      outputMessage += currentValue;
    }
    // GET inputMotorRightB
    else if (request->hasParam(PARAM_INT4)) {
      String currentValue = readFile(SPIFFS, "/inputMotorRightB.txt");
      outputMessage = "inputMotorRightB: ";
      outputMessage += currentValue;
    }
    // GET inputLedLeft
    else if (request->hasParam(PARAM_INT5)) {
      String currentValue = readFile(SPIFFS, "/inputLedLeft.txt");
      outputMessage = "inputLedLeft: ";
      outputMessage += currentValue;
    }
    // GET inputLedRight
    else if (request->hasParam(PARAM_INT6)) {
      String currentValue = readFile(SPIFFS, "/inputLedRight.txt");
      outputMessage = "inputLedRight: ";
      outputMessage += currentValue;
    }
    // GET inputThresholdLight
    else if (request->hasParam(PARAM_INT7)) {
      String currentValue = readFile(SPIFFS, "/inputThresholdLight.txt");
      outputMessage = "inputThresholdLight: ";
      outputMessage += currentValue;
    } // GET inputDelayBackwardsDrive
    else if (request->hasParam(PARAM_INT8)) {
      String currentValue = readFile(SPIFFS, "/inputDelayBackwardsDrive.txt");
      outputMessage = "inputDelayBackwardsDrive: ";
      outputMessage += currentValue;
    } // GET lightSensorStatus
    else if (request->hasParam(PARAM_WEB_BUTTON_LIGHT_SENSOR)) {
      String currentValue = readFile(SPIFFS, "/lightSensorStatus.txt");
      outputMessage = "lightSensorStatus: ";
      outputMessage += currentValue;
    } // GET ultraSonicSensorStatus
    else if (request->hasParam(PARAM_WEB_BUTTON_ULTRASONIC_SENSOR)) {
      String currentValue = readFile(SPIFFS, "/ultraSonicSensorStatus.txt");
      outputMessage = "ultraSonicSensorStatus: ";
      outputMessage += currentValue;
    }
    // GET obstacleSensorStatus
    else if (request->hasParam(PARAM_WEB_BUTTON_OBSTACLE_SENSOR)) {
      String currentValue = readFile(SPIFFS, "/obstacleSensorStatus.txt");
      outputMessage = "obstacleSensorStatus: ";
      outputMessage += currentValue;
    } // GET motorSensorConnection
    else if (request->hasParam(PARAM_WEB_BUTTON_CONNECTIONTYPE)) {
      String currentValue = readFile(SPIFFS, "/motorSensorConnection.txt");
      outputMessage = "motorSensorConnection: ";
      outputMessage += currentValue;
    } // GET timerResetter
    else if (request->hasParam("timerResetter")) {
      outputMessage = "timerResetter";
    }
    else if (request->hasParam("lightL")) {
      outputMessage = "lightL: ";
      outputMessage += String(analogRead(lightSensorPin1)).c_str();
    }
    else if (request->hasParam("lightR")) {
      outputMessage = "lightR: ";
      outputMessage += String(analogRead(lightSensorPin2)).c_str();
    }
    else if (request->hasParam("ultraSonicDistance")) {
      outputMessage = "ultraSonicDistance: ";
      outputMessage += String(distance).c_str();
    }
    else if (request->hasParam("isObstacle")) {
      outputMessage = "isObstacle: ";
      outputMessage += String(obstacleCheck()).c_str();
    }
    else {
      outputMessage = "No message sent";
    }
    // Serial.println(inputMessage);
    request->send(200, "text/getter", outputMessage); });

  // Send a GET request to <ESP_IP>/put?INPUTNAME(daher zb inputMotorLeftF)=intWert
  server.on("/put", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    // PUT inputMotorLeftF
    if (request->hasParam(PARAM_INT1))
    {
      inputMessage = request->getParam(PARAM_INT1)->value();
      inputMotorLeftF = inputMessage.toInt();
      writeFile(SPIFFS, "/inputMotorLeftF.txt", inputMessage.c_str());
    }
    // PUT inputMotorLeftB
    else if (request->hasParam(PARAM_INT3))
    {
      inputMessage = request->getParam(PARAM_INT3)->value();
      inputMotorLeftB = inputMessage.toInt();
      writeFile(SPIFFS, "/inputMotorLeftB.txt", inputMessage.c_str());
    }
    // PUT inputMotorRightF
    else if (request->hasParam(PARAM_INT2))
    {
      inputMessage = request->getParam(PARAM_INT2)->value();
      inputMotorRightF = inputMessage.toInt();
      writeFile(SPIFFS, "/inputMotorRightF.txt", inputMessage.c_str());
    }
    // PUT inputMotorRightB
    else if (request->hasParam(PARAM_INT4))
    {
      inputMessage = request->getParam(PARAM_INT4)->value();
      inputMotorRightB = inputMessage.toInt();
      writeFile(SPIFFS, "/inputMotorRightB.txt", inputMessage.c_str());
    }
    // PUT inputLedLeft
    else if (request->hasParam(PARAM_INT5))
    {
      inputMessage = request->getParam(PARAM_INT5)->value();
      inputLedLeft = inputMessage.toInt();
      writeFile(SPIFFS, "/inputLedLeft.txt", inputMessage.c_str());
    }
    // PUT inputLedRight
    else if (request->hasParam(PARAM_INT6))
    {
      inputMessage = request->getParam(PARAM_INT6)->value();
      inputLedRight = inputMessage.toInt();
      writeFile(SPIFFS, "/inputLedRight.txt", inputMessage.c_str());
    }
    // PUT inputThresholdLight
    else if (request->hasParam(PARAM_INT7))
    {
      inputMessage = request->getParam(PARAM_INT7)->value();
      inputThresholdLight = inputMessage.toInt();
      thresholdLight = inputThresholdLight;
      writeFile(SPIFFS, "/inputThresholdLight.txt", inputMessage.c_str());
    }
    // PUT inputDelayBackwardsDrive
    else if (request->hasParam(PARAM_INT8))
    {
      inputMessage = request->getParam(PARAM_INT8)->value();
      inputDelayBackwardsDrive = inputMessage.toInt();
      delayBackwardsDrive = inputDelayBackwardsDrive;
      writeFile(SPIFFS, "/inputDelayBackwardsDrive.txt", inputMessage.c_str());
    }
    // PUT lightSensorStatus
    else if (request->hasParam(PARAM_WEB_BUTTON_LIGHT_SENSOR))
    {
      String currentStatus = readFile(SPIFFS, "/lightSensorStatus.txt");
      if (currentStatus == "On")
      {
        inputMessage = "Off";
      }
      else
      {
        inputMessage = "On";
      }
      lightSensorStatus = inputMessage;
      writeFile(SPIFFS, "/lightSensorStatus.txt", inputMessage.c_str());
    }
    // PUT ultraSonicSensorStatus
    else if (request->hasParam(PARAM_WEB_BUTTON_ULTRASONIC_SENSOR)) {
      String currentStatus = readFile(SPIFFS, "/ultraSonicSensorStatus.txt");
      if (currentStatus == "On") {
        inputMessage = "Off";
      } else {
        inputMessage = "On";
      }
      ultraSonicSensorStatus = inputMessage;
      writeFile(SPIFFS, "/ultraSonicSensorStatus.txt", inputMessage.c_str());
    }
    // PUT obstacleSensorStatus
    else if (request->hasParam(PARAM_WEB_BUTTON_OBSTACLE_SENSOR))
    {
      String currentStatus = readFile(SPIFFS, "/obstacleSensorStatus.txt");
      if (currentStatus == "On")
      {
        inputMessage = "Off";
      }
      else
      {
        inputMessage = "On";
      }
      obstacleSensorStatus = inputMessage;
      writeFile(SPIFFS, "/obstacleSensorStatus.txt", inputMessage.c_str());
    }
    // PUT motorSensorConnection
    else if (request->hasParam(PARAM_WEB_BUTTON_CONNECTIONTYPE))
    {
      String currentStatus = readFile(SPIFFS, "/motorSensorConnection.txt");
      if (currentStatus == "cross")
      {
        inputMessage = "parallel";
      }
      else
      {
        inputMessage = "cross";
      }
      motorSensorConnection = inputMessage;
      writeFile(SPIFFS, "/motorSensorConnection.txt", inputMessage.c_str());
    }
    // PUT timerResetter
    else if (request->hasParam("timerResetter"))
    {
      timeoutTimer = millis();
      inputMessage = "Timer reset";
    }
    else
    {
      inputMessage = "No message sent";
    }
    // Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage); });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    String inputParam;
    // GET appControl state value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_BUTTON_APPCONTROL)) {
      inputMessage = request->getParam(PARAM_BUTTON_APPCONTROL)->value();
      inputParam = PARAM_BUTTON_APPCONTROL;
      onOff = HIGH;
      appControl = inputMessage.toInt();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    request->send(200, "text/plain", "OK"); });

  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(appControl).c_str()); });

  server.on("/lightL", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", String(analogRead(lightSensorPin1)).c_str()); });

  server.on("/lightR", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", String(analogRead(lightSensorPin2)).c_str()); });

  server.on("/ultraSonicDistance", HTTP_GET, [](AsyncWebServerRequest *request)
            { distance = readUltrasonicDistanceInCm();
            request->send_P(200, "text/plain", String(distance).c_str()); });
  
  server.on("/isObstacle", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", String(obstacleCheck()).c_str()); });
  
  server.onNotFound(notFound);
  server.begin();

  getStoredSPIFFSValues();
}

void loop()
{
  buttonState = digitalRead(buttonPin);
  handleClick();
  handleMotorLed();
  updateMotorLed();
  // clientCheckTimeout(8); // timeoutInSeconds
}
