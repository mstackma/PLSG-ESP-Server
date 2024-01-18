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
#define motorRightPin1 26
#define motorRightPin2 27
#define buttonPin 4
#define triggerPin 5
#define echoPin 18
#define lightSensorPin1 34
#define lightSensorPin2 35
// define sound speed in cm/uS
#define SOUND_SPEED 0.034

long durationSoundWave;

unsigned long startTime = 0;
unsigned long durationButtonClick = 0;

int analogLightValueLeft;
int analogLightValueRight;
const int thresholdLight = 1400;

// Variables will change:
int onOff = LOW;               // robot motor, led state: on/HIGH or off/LOW
int appControl = LOW;          // the current state: App-controlled or autonomous robot
int buttonState = 0;           // the current reading from the input buttonPin
int lastButtonState = 0;       // the previous reading from the input buttonPin
int cm = 0;                    // distance cm
int motorLeftF;                // motorLeftForward Value
int motorRightF;               // motorRightForward Value
int motorLeftB;                // motorLeftBackward Value
int motorRightB;               // motorRightBackward Value
String lightSensorStatus;      // lightSensorStatus On Off
String ultraSonicSensorStatus; // ultraSonicSensorStatus On Off

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
const char *PARAM_WEB_BUTTON_LIGHT_SENSOR = "lightSensorStatus";           // lightSensorStatus "On" or "Off"
const char *PARAM_WEB_BUTTON_ULTRASONIC_SENSOR = "ultraSonicSensorStatus"; // ultraSonicSensorStatus "On" or "Off"
const char *PARAM_INPUT_1 = "state";                                       // state

// HTML web page to handle 4 input fields (motorLeftF..) and a button
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Robot Control Center</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 2.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .units {font-size: 1.2rem;}
    .sensor {font-size: 1.5rem; padding-bottom: 15px;}
  </style>
</head>
<body>
  <h2>Robot Control Center</h2>
  %BUTTONPLACEHOLDER%
  <br><br><br><br>  
  <iframe style="display:none" name="hidden-form"></iframe>
<form action="/get" target="hidden-form">
    inputMotorLeftF (current value %inputMotorLeftF%): <input type="number" name="inputMotorLeftF">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputMotorRightF (current value %inputMotorRightF%): <input type="number" name="inputMotorRightF">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputMotorLeftB (current value %inputMotorLeftB%): <input type="number" name="inputMotorLeftB">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputMotorRightB (current value %inputMotorRightB%): <input type="number" name="inputMotorRightB">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    lightSensorStatus %lightSensorStatus% <input type="button" value="LightSensor" id="lightSensor">
  </form><br>
  <form action="/get" target="hidden-form">
    ultraSonicSensorStatus %ultraSonicSensorStatus% <input type="button" value="UltraSonicSensor" id="ultraSonicSensor">
  </form><br>
  <p>
    <span class="sensor">Brightness Sensor Left Value</span> 
    <span id="lightL">%LIGHTL%</span>
  </p>
  <p>
    <span class="sensor">Brightness Sensor Right Value</span> 
    <span id="lightR">%LIGHTR%</span>
  </p>
  <p>
    <span class="sensor">Distance in front</span> 
    <span id="ultraSonicDistance">%ULTRADISTANCE%</span>
    <span class="sensor">cm</span>
  </p>
</body>
<script>
document.getElementById("lightSensor").addEventListener("click", function() { buttonClick("lightSensorStatus=1");}, false);
document.getElementById("ultraSonicSensor").addEventListener("click", function() { buttonClick("ultraSonicSensorStatus=1");}, false);

function buttonClick(sensorTypeStatus) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/get?" + sensorTypeStatus, true);
  xhr.onreadystatechange = function () {
    if (xhr.readyState == 4 && xhr.status == 200) {
      alert("Saved value to ESP SPIFFS");
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
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }

fetchData("lightL", "/lightL");
fetchData("lightR", "/lightR");
fetchData("ultraSonicDistance", "/ultraSonicDistance");

function fetchData(elementId, url) {
  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById(elementId).innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", url, true);
    xhttp.send();
  }, 10000);
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

int analogReadLightSensor(int sensorPin)
{
  int sensorData;
  if (lightSensorStatus == "On")
  {
    sensorData = analogRead(sensorPin);
  }
  else
  {
    sensorData = 0;
  }
  return sensorData;
}

float readUltrasonicDistanceInCm()
{
  float distanceCm;
  if (ultraSonicSensorStatus == "On")
  {
    // Clear the trigger
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);

    // Sets the trigger pin to HIGH state for 10 microseconds
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    // Reads the echoPin, returns the sound wave travel time in microseconds
    durationSoundWave = pulseIn(echoPin, HIGH);

    // Calculate the distance
    distanceCm = durationSoundWave * SOUND_SPEED / 2;
  }
  else
  {
    distanceCm = 0;
  }
  return distanceCm;
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
  else if (var == "lightSensorStatus")
  {
    return readFile(SPIFFS, "/lightSensorStatus.txt");
  }
  else if (var == "ultraSonicSensorStatus")
  {
    return readFile(SPIFFS, "/ultraSonicSensorStatus.txt");
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
    analogLightValueLeft = analogReadLightSensor(lightSensorPin1);
    Serial.println("analogLightValueLeft 1");
    Serial.println(analogLightValueLeft);
    return String(analogLightValueLeft);
  }
  else if (var == "LIGHTR")
  {
    // reads the input on analog pin (value between 0 and 4095)
    analogLightValueRight = analogReadLightSensor(lightSensorPin2);
    Serial.println("analogLightValueRight 1");
    Serial.println(analogLightValueRight);
    return String(analogLightValueRight);
  }
  else if (var == "ULTRADISTANCE")
  {
    float distanceCm = readUltrasonicDistanceInCm();
    Serial.println("distanceCm 1");
    Serial.println(distanceCm);
    return String(distanceCm);
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
        Serial.println("analogLightValueLeft 2");
        Serial.println(analogLightValueLeft);
        Serial.println("analogLightValueRight 2");
        Serial.println(analogLightValueRight);
        Serial.println("lightSensorStatus");
        Serial.println(lightSensorStatus);
        Serial.println("ultraSonicSensorStatus");
        Serial.println(ultraSonicSensorStatus);
      }
      else
      { // long button press
        appControl = LOW;
        onOff = LOW;
        Serial.println("-----------------------------long-------------------------------------");
      }
      Serial.println(readUltrasonicDistanceInCm());
    }
  }
  lastButtonState = buttonState;
}

// Braitenberg-Vehicel 2 connection
void motorLightSensorConnection(String cross_or_parallel)
{
  analogWrite(motorLeftPin2, 0);
  analogWrite(motorRightPin2, 0);
  if (cross_or_parallel == "cross")
  {
    analogWrite(motorLeftPin1, map(analogLightValueRight, thresholdLight, 4095, 70, 255));
    analogWrite(motorRightPin1, map(analogLightValueLeft, thresholdLight, 4095, 70, 255));
  }
  else if (cross_or_parallel == "parallel")
  {
    analogWrite(motorLeftPin1, map(analogLightValueLeft, thresholdLight, 4095, 70, 255));
    analogWrite(motorRightPin1, map(analogLightValueRight, thresholdLight, 4095, 70, 255));
  }
}

void stopMotor()
{
  analogWrite(motorLeftPin1, 0);
  analogWrite(motorLeftPin2, 0);
  analogWrite(motorRightPin1, 0);
  analogWrite(motorRightPin2, 0);
}

// to react when an obstacle is nearby
void handleMotor()
{
  if (onOff == 1 && readUltrasonicDistanceInCm() < 6)
  {
    Serial.println("distanceCm < 6");
    analogWrite(ledPinLeft, 0);
    analogWrite(ledPinRight, 0);

    analogWrite(motorLeftPin1, 0); // backwards
    analogWrite(motorLeftPin2, 200);
    analogWrite(motorRightPin1, 0);
    analogWrite(motorRightPin2, 200);
    delay(2000);
  }
  if (onOff == 1 && appControl == 1)
  {
    analogWrite(ledPinLeft, motorLeftF);
    analogWrite(ledPinRight, motorRightF);

    analogWrite(motorLeftPin1, motorLeftF);
    analogWrite(motorLeftPin2, motorLeftB);
    analogWrite(motorRightPin1, motorRightF);
    analogWrite(motorRightPin2, motorRightB);
  }
  else if (onOff == 1 && appControl == 0)
  {
    // reads the input on analog pin (value between 0 and 4095)
    analogLightValueLeft = analogReadLightSensor(lightSensorPin1);
    analogLightValueRight = analogReadLightSensor(lightSensorPin2);
    analogWrite(ledPinLeft, map(analogLightValueLeft, 0, 4095, 0, 255));
    analogWrite(ledPinRight, map(analogLightValueRight, 0, 4095, 0, 255));
    if ((analogLightValueRight > thresholdLight || analogLightValueLeft > thresholdLight))
    {
      motorLightSensorConnection("cross"); // Vorwärts entsprechend der Helligkeit und Sensor-Motor-Verbindung
    }
    else
    {
      stopMotor();
    }
  }
  else
  {
    analogWrite(ledPinLeft, 0);
    analogWrite(ledPinRight, 0);
    stopMotor();
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
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

  // Send a GET request to <ESP_IP>/get?INPUTNAME(daher zb inputMotorLeftF)=intWert
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    // GET inputMotorLeftF
    if (request->hasParam(PARAM_INT1)) {
      inputMessage = request->getParam(PARAM_INT1)->value();
      writeFile(SPIFFS, "/inputMotorLeftF.txt", inputMessage.c_str());
    }
    // GET inputMotorRightF
    else if (request->hasParam(PARAM_INT2)) {
      inputMessage = request->getParam(PARAM_INT2)->value();
      writeFile(SPIFFS, "/inputMotorRightF.txt", inputMessage.c_str());
    }
    // GET inputMotorLeftB
    else if (request->hasParam(PARAM_INT3)) {
      inputMessage = request->getParam(PARAM_INT3)->value();
      writeFile(SPIFFS, "/inputMotorLeftB.txt", inputMessage.c_str());
    }
    // GET inputMotorRightB
    else if (request->hasParam(PARAM_INT4)) {
      inputMessage = request->getParam(PARAM_INT4)->value();
      writeFile(SPIFFS, "/inputMotorRightB.txt", inputMessage.c_str());
    } // GET lightSensorStatus
    else if (request->hasParam(PARAM_WEB_BUTTON_LIGHT_SENSOR)) {
      String currentStatus = readFile(SPIFFS, "/lightSensorStatus.txt");
      if (currentStatus == "On") {
        inputMessage = "Off";
      } else {
        inputMessage = "On";
      }
      writeFile(SPIFFS, "/lightSensorStatus.txt", inputMessage.c_str());
    } // GET ultraSonicSensorStatus
    else if (request->hasParam(PARAM_WEB_BUTTON_ULTRASONIC_SENSOR)) {
      String currentStatus = readFile(SPIFFS, "/ultraSonicSensorStatus.txt");
      if (currentStatus == "On") {
        inputMessage = "Off";
      } else {
        inputMessage = "On";
      }
      writeFile(SPIFFS, "/ultraSonicSensorStatus.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    // Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage); });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      onOff = HIGH;
      appControl = !appControl;
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
            { request->send_P(200, "text/plain", String(analogReadLightSensor(lightSensorPin1)).c_str()); });

  server.on("/lightR", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", String(analogReadLightSensor(lightSensorPin2)).c_str()); });

  server.on("/ultraSonicDistance", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", String(readUltrasonicDistanceInCm()).c_str()); });

  server.onNotFound(notFound);
  server.begin();
}

void loop()
{
  // To access your stored values on motor..
  motorLeftF = readFile(SPIFFS, "/inputMotorLeftF.txt").toInt();
  // Serial.print("*** Your inputMotorLeftF: ");
  // Serial.println(motorLeftF);

  motorRightF = readFile(SPIFFS, "/inputMotorRightF.txt").toInt();
  // Serial.print("*** Your inputMotorRightF: ");
  // Serial.println(motorRightF);

  motorLeftB = readFile(SPIFFS, "/inputMotorLeftB.txt").toInt();
  // Serial.print("*** Your inputMotorLeftB: ");
  // Serial.println(motorLeftB);

  motorRightB = readFile(SPIFFS, "/inputMotorRightB.txt").toInt();
  // Serial.print("*** Your inputMotorRightB: ");
  // Serial.println(motorRightB);

  lightSensorStatus = readFile(SPIFFS, "/lightSensorStatus.txt");
  ultraSonicSensorStatus = readFile(SPIFFS, "/ultraSonicSensorStatus.txt");

  buttonState = digitalRead(buttonPin);
  handleClick();
  handleMotor();

  // delay(5000);
}
