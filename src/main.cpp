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
// define sound speed in cm/uS
#define SOUND_SPEED 0.034

long durationSoundWave;
float distanceCm;

unsigned long startTime = 0;
unsigned long durationButtonClick = 0;

int analogLightValueRight;
int analogLightValueLeft;
const int thresholdLight = 600;

// Variables will change:
int onOff = LOW;         // on or off
int appControl = LOW;    // the current state: App-controlled or autonomous robot
int buttonState = 0;     // the current reading from the input pin
int lastButtonState = 0; // the previous reading from the input pin
int cm = 0;              // distance cm
int motorLeftF;          // motorLeftForward Value
int motorRightF;         // motorRightForward Value
int motorLeftB;          // motorLeftBackward Value
int motorRightB;         // motorRightBackward Value

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
const char* ssid = "ssid";
const char* password = "password";

const char *PARAM_INT1 = "motorLeftF";
const char *PARAM_INT2 = "motorRightF";
const char *PARAM_INT3 = "motorLeftB";
const char *PARAM_INT4 = "motorRightB";

const char *PARAM_INPUT_1 = "state";

// HTML web page to handle 4 input fields (motorLeftF..) and a button
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
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
</script>
<form action="/get" target="hidden-form">
    motorLeftF (current value %motorLeftF%): <input type="number" name="motorLeftF">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    motorRightF (current value %motorRightF%): <input type="number" name="motorRightF">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    motorLeftB (current value %motorLeftB%): <input type="number" name="motorLeftB">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    motorRightB (current value %motorRightB%): <input type="number" name="motorRightB">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body>
</html>
)rawliteral";

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

// Replaces placeholder with stored values
String processor(const String &var)
{
  // Serial.println(var);
  if (var == "motorLeftF")
  {
    return readFile(SPIFFS, "/motorLeftF.txt");
  }
  else if (var == "motorRightF")
  {
    return readFile(SPIFFS, "/motorRightF.txt");
  }
  else if (var == "motorLeftB")
  {
    return readFile(SPIFFS, "/motorLeftB.txt");
  }
  else if (var == "motorRightB")
  {
    return readFile(SPIFFS, "/motorRightB.txt");
  }
  else if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    String appControlStateValue = appControlState();
    buttons += "<h4>appControl - State <span id=\"appControlState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"appControl\" " + appControlStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
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
      }
      else
      { // long button press
        appControl = LOW;
        onOff = LOW;
        Serial.println("-----------------------------long-------------------------------------");
      }
      Serial.println(distanceCm);
    }
  }
  lastButtonState = buttonState;
}

// following function from https://www.electronicscuriosities.com/2020/10/ultrasonic-sensor-with-arduino-uno-with.html
void readUltrasonicDistanceInCm()
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

// Braitenberg-Vehicel 2 connection
void motorLightSensorConnection(String cross_or_parallel)
{
  analogWrite(motorLeftPin2, 0);
  analogWrite(motorRightPin2, 0);
  if (cross_or_parallel == "cross")
  {
    analogWrite(motorLeftPin1, map(analogLightValueRight, 0, 1023, 0, 255));
    analogWrite(motorRightPin1, map(analogLightValueLeft, 0, 1023, 0, 255));
  }
  else if (cross_or_parallel == "parallel")
  {
    analogWrite(motorLeftPin1, map(analogLightValueLeft, 0, 1023, 0, 255));
    analogWrite(motorRightPin1, map(analogLightValueRight, 0, 1023, 0, 255));
  }
}

// to react when an obstacle is nearby
void handleMotor()
{
  readUltrasonicDistanceInCm();
  if (onOff == 1 && distanceCm < 6)
  {
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
  } /* else if (onOff == 1 && appControl == 0) {
    // reads the input on analog pin (value between 0 and 1023)
    analogLightValueRight = analogRead(SensorPin1);
    analogLightValueLeft = analogRead(SensorPin2);

    analogWrite(ledPinRight, map(analogLightValueRight, 0, 1023, 0, 255));
    analogWrite(ledPinLeft, map(analogLightValueLeft, 0, 1023, 0, 255));
    motorLightSensorConnection( "cross" ); //Vorwärts entsprechend der Helligkeit und Sensor-Motor-Verbindung
  } */
  else
  {
    analogWrite(ledPinLeft, 0);
    analogWrite(ledPinRight, 0);

    analogWrite(motorLeftPin1, 0); // stop
    analogWrite(motorLeftPin2, 0);
    analogWrite(motorRightPin1, 0);
    analogWrite(motorRightPin2, 0);
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

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    // GET motorLeftF
    if (request->hasParam(PARAM_INT1)) {
      inputMessage = request->getParam(PARAM_INT1)->value();
      writeFile(SPIFFS, "/motorLeftF.txt", inputMessage.c_str());
    }
    // GET motorRightF
    else if (request->hasParam(PARAM_INT2)) {
      inputMessage = request->getParam(PARAM_INT2)->value();
      writeFile(SPIFFS, "/motorRightF.txt", inputMessage.c_str());
    }
    // GET motorLeftB
    else if (request->hasParam(PARAM_INT3)) {
      inputMessage = request->getParam(PARAM_INT3)->value();
      writeFile(SPIFFS, "/motorLeftB.txt", inputMessage.c_str());
    }
    // GET motorRightB
    else if (request->hasParam(PARAM_INT4)) {
      inputMessage = request->getParam(PARAM_INT4)->value();
      writeFile(SPIFFS, "/motorRightB.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    //Serial.println(inputMessage);
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
    Serial.println("inputMessage");
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK"); });

  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(appControl).c_str()); });

  server.onNotFound(notFound);
  server.begin();
}

void loop()
{
  // To access your stored values on motor..
  int yourInputInt1 = readFile(SPIFFS, "/motorLeftF.txt").toInt();
  // Serial.print("*** Your motorLeftF: ");
  // Serial.println(yourInputInt1);

  int yourInputInt2 = readFile(SPIFFS, "/motorRightF.txt").toInt();
  // Serial.print("*** Your motorRightF: ");
  // Serial.println(yourInputInt2);

  int yourInputInt3 = readFile(SPIFFS, "/motorLeftB.txt").toInt();
  // Serial.print("*** Your motorLeftB: ");
  // Serial.println(yourInputInt3);

  int yourInputInt4 = readFile(SPIFFS, "/motorRightB.txt").toInt();
  // Serial.print("*** Your motorRightB: ");
  // Serial.println(yourInputInt4);

  motorLeftF = yourInputInt1;
  motorRightF = yourInputInt2;
  motorLeftB = yourInputInt3;
  motorRightB = yourInputInt4;

  buttonState = digitalRead(buttonPin);
  handleClick();
  handleMotor();

  // delay(5000);
}
