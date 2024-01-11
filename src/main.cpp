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
#define motorRightPin1 27
#define motorRightPin2 26
#define buttonPin 4
#define triggerPin 5
#define echoPin 18
//define sound speed in cm/uS
#define SOUND_SPEED 0.034

long durationSoundWave;
float distanceCm;

unsigned long startTime = 0;
unsigned long durationButtonClick = 0;

int analogLightValueRight;
int analogLightValueLeft;
const int thresholdLight = 600;

// Variables will change:
int onOff = LOW;  // on or off
int appControl = LOW;  // the current state: App-controlled or autonomous robot
int buttonState = 0;  // the current reading from the input pin
int lastButtonState = 0;   // the previous reading from the input pin
int cm = 0; // distance cm
int motorLeftF; // motorLeftForward Value
int motorRightF;  // motorRightForward Value
int motorLeftB; // motorLeftBackward Value
int motorRightB;  // motorRightBackward Value

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

const char* PARAM_INT1  = "motorLeftF";
const char* PARAM_INT2 = "motorRightF";
const char* PARAM_INT3  = "motorLeftB";
const char* PARAM_INT4 = "motorRightB";

// HTML web page to handle 2 input fields (motorLeftF and 2)
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
  if(var == "motorLeftF"){
    return readFile(SPIFFS, "/motorLeftF.txt");
  }
  else if(var == "motorRightF"){
    return readFile(SPIFFS, "/motorRightF.txt");
  }
  else if(var == "motorLeftB"){
    return readFile(SPIFFS, "/motorLeftB.txt");
  }
  else if(var == "motorRightB"){
    return readFile(SPIFFS, "/motorRightB.txt");
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
        onOff = LOW;
        Serial.println("-----------------------------long-------------------------------------");
      }
      Serial.println(distanceCm);
    }
  }
  lastButtonState = buttonState;
}

//following function from https://www.electronicscuriosities.com/2020/10/ultrasonic-sensor-with-arduino-uno-with.html
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
  distanceCm = durationSoundWave * SOUND_SPEED/2;
}

//Braitenberg-Vehicel 2 connection
void motorLightSensorConnection(String cross_or_parallel) {
  digitalWrite(motorLeftPin2,LOW);
  digitalWrite(motorRightPin2,LOW);
  if ( cross_or_parallel == "cross" ) {
    analogWrite(motorLeftPin1, map(analogLightValueRight, 0, 1023, 0, 255));
    analogWrite(motorRightPin1, map(analogLightValueLeft, 0, 1023, 0, 255));
  } else if ( cross_or_parallel == "parallel" ) {
    analogWrite(motorLeftPin1, map(analogLightValueLeft, 0, 1023, 0, 255));
    analogWrite(motorRightPin1, map(analogLightValueRight, 0, 1023, 0, 255));
  }
}

//to react when an obstacle is nearby
void handleMotor() {
  readUltrasonicDistanceInCm();
  if (distanceCm < 6){
    analogWrite(ledPinLeft,0);
    analogWrite(ledPinRight,0);

    digitalWrite(motorLeftPin1,LOW);    //backwards
    digitalWrite(motorLeftPin2,HIGH);
    digitalWrite(motorRightPin1,LOW);
    digitalWrite(motorRightPin2,HIGH);
    delay(2000);
  }
  if (onOff == 1 && appControl == 1) {
    analogWrite(ledPinLeft,motorLeftF);
    analogWrite(ledPinRight,motorRightF);

    analogWrite(motorLeftPin1,motorLeftF);
    digitalWrite(motorLeftPin2,motorLeftB);
    analogWrite(motorRightPin1,motorRightF);
    digitalWrite(motorRightPin2,motorRightB);
  } /* else if (onOff == 1 && appControl == 0) {
    // reads the input on analog pin (value between 0 and 1023)
    analogLightValueRight = analogRead(SensorPin1);
    analogLightValueLeft = analogRead(SensorPin2);

    analogWrite(ledPinRight, map(analogLightValueRight, 0, 1023, 0, 255));
    analogWrite(ledPinLeft, map(analogLightValueLeft, 0, 1023, 0, 255));
    motorLightSensorConnection( "cross" ); //Vorwärts entsprechend der Helligkeit und Sensor-Motor-Verbindung
  } */
  else {
    analogWrite(ledPinLeft,0);
    analogWrite(ledPinRight,0);

    digitalWrite(motorLeftPin1,LOW);  //stop
    digitalWrite(motorLeftPin2,LOW);
    digitalWrite(motorRightPin1,LOW);
    digitalWrite(motorRightPin2,LOW);
  }
}

void setup() {
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
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {  
  // To access your stored values on inputInt
  int yourInputInt1 = readFile(SPIFFS, "/motorLeftF.txt").toInt();
  //Serial.print("*** Your motorLeftF: ");
  //Serial.println(yourInputInt1);
  
  int yourInputInt2 = readFile(SPIFFS, "/motorRightF.txt").toInt();
  //Serial.print("*** Your motorRightF: ");
  //Serial.println(yourInputInt2);

  int yourInputInt3 = readFile(SPIFFS, "/motorLeftB.txt").toInt();
  //Serial.print("*** Your motorLeftB: ");
  //Serial.println(yourInputInt2);

  int yourInputInt4 = readFile(SPIFFS, "/motorRightB.txt").toInt();
  //Serial.print("*** Your motorRightB: ");
  //Serial.println(yourInputInt2);

  motorLeftF = yourInputInt1;
  motorRightF = yourInputInt2;
  motorLeftB = yourInputInt3;
  motorRightB = yourInputInt4;

  buttonState = digitalRead(buttonPin);
  handleClick();
  handleMotor();
  
  //delay(5000);
}
