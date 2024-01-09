//Editing of the ESP32 Server - SPIFFS - version:
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
#define motorLeftPin1 33
#define motorLeftPin2 32
//#define motorRightPin1 35
//#define motorRightPin2 34
#define buttonPin 4

unsigned long startTime = 0;
unsigned long duration = 0;

const int thresholdLight = 600;

// Variables will change:
int motorConectionState = LOW;// the current state of the motor-sensor-connection
int buttonState = 0;             // the current reading from the input pin
int lastButtonState = 0;   // the previous reading from the input pin
int cm = 0;
int motorLeft;
int motorRight;

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

const char* PARAM_INT1  = "inputInt1";
const char* PARAM_INT2 = "inputInt2";

// HTML web page to handle 2 input fields (inputInt1 and 2)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
    inputInt1 (current value %inputInt1%): <input type="number" name="inputInt1">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputInt2 (current value %inputInt2%): <input type="number" name="inputInt2">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  //Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  //Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  //Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "inputInt1"){
    return readFile(SPIFFS, "/inputInt1.txt");
  }
  else if(var == "inputInt2"){
    return readFile(SPIFFS, "/inputInt2.txt");
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
      duration = millis() - startTime;

      if (duration < 500)
      { // short button press
        motorConectionState = !motorConectionState;
        Serial.println("-----------------------------short------------------------------------");
      }
      else
      { // long button press
        motorConectionState = LOW;
        Serial.println("-----------------------------long-------------------------------------");
      }
    }
  }

  lastButtonState = buttonState;
}
/*
//following function from https://www.electronicscuriosities.com/2020/10/ultrasonic-sensor-with-arduino-uno-with.html
long readUltrasonicDistance(int triggerPin, int echoPin)
{
  pinMode(triggerPin, OUTPUT);  // Clear the trigger
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  // Sets the trigger pin to HIGH state for 10 microseconds
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  pinMode(echoPin, INPUT);
  
  // Reads the echo pin, and returns the sound wave travel time in microseconds
  return pulseIn(echoPin, HIGH);
}
*/
//to react when an obstacle is nearby
void handleMotor() {
  // measure the ping time in cm
  
  /*
  cm = 0.01723 * readUltrasonicDistance(7, 8);
  Serial.println(cm);

  if (cm < 6){
    digitalWrite(motorLeftPin1,LOW);    //backwards
    digitalWrite(motorLeftPin2,HIGH);
    //digitalWrite(motorRightPin1,LOW);
    //digitalWrite(motorRightPin2,HIGH);
    delay(2000);
  } */
  if (motorConectionState == 1) {
    analogWrite(ledPinLeft, motorLeft);
    analogWrite(ledPinRight, motorRight);

    analogWrite(motorLeftPin1, motorLeft);  //forwards
    digitalWrite(motorLeftPin2,LOW);
    //analogWrite(motorRightPin1, motorRight);
    //digitalWrite(motorRightPin2,LOW);
  } else {
    analogWrite(ledPinLeft,0);
    analogWrite(ledPinRight,0);

    digitalWrite(motorLeftPin1,LOW);  //stop
    digitalWrite(motorLeftPin2,LOW);
    //digitalWrite(motorRightPin1,LOW);
    //digitalWrite(motorRightPin2,LOW);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPinLeft, OUTPUT);
  digitalWrite(ledPinLeft, LOW);
  pinMode(ledPinRight, OUTPUT);
  digitalWrite(ledPinRight, LOW);
  pinMode(motorLeftPin1, OUTPUT);
  pinMode(motorLeftPin2, OUTPUT);
  //pinMode(motorRightPin1, OUTPUT);
  //pinMode(motorRightPin2, OUTPUT);

  pinMode(buttonPin, INPUT);

  // Initialize SPIFFS
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET inputInt1
    if (request->hasParam(PARAM_INT1)) {
      inputMessage = request->getParam(PARAM_INT1)->value();
      writeFile(SPIFFS, "/inputInt1.txt", inputMessage.c_str());
    }
    // GET inputInt2
    else if (request->hasParam(PARAM_INT2)) {
      inputMessage = request->getParam(PARAM_INT2)->value();
      writeFile(SPIFFS, "/inputInt2.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    //Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {  
  // To access your stored values on inputInt
  int yourInputInt1 = readFile(SPIFFS, "/inputInt1.txt").toInt();
  //Serial.print("*** Your inputInt1: ");
  //Serial.println(yourInputInt1);
  
  int yourInputInt2 = readFile(SPIFFS, "/inputInt2.txt").toInt();
  //Serial.print("*** Your inputInt2: ");
  //Serial.println(yourInputInt2);

  if (yourInputInt1 <= 255) {
    motorLeft = yourInputInt1;
  }
  if (yourInputInt2 <= 255) {
    motorRight = yourInputInt2;
  }

  buttonState = digitalRead(buttonPin);
  handleClick();
  handleMotor();
  
  //delay(5000);
}
