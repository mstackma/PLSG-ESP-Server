/*********
 * Own editing of the original ESP32 Server - SPIFFS - version of:
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-input-data-html-form/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include <Arduino.h>

int ledPinRight = 9;
int ledPinLeft = 10;
#define motor_left_pin1 33
#define motor_left_pin2 32
#define motor_right_pin1 35
#define motor_right_pin2 34

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

// HTML web page to handle 3 input fields (inputString, inputInt, inputFloat)
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
    inputInt1 (current value %inputInt1%): <input type="number1 " name="inputInt1">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputInt2 (current value %inputInt2%): <input type="number2 " name="inputInt2">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
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

void setup() {
  Serial.begin(115200);

  pinMode(motor_left_pin1, OUTPUT);
  pinMode(motor_left_pin2, OUTPUT);
  pinMode(motor_right_pin1, OUTPUT);
  pinMode(motor_right_pin2, OUTPUT);

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
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  // To access your stored values on inputString, inputInt, inputFloat
  String yourInputInt1 = readFile(SPIFFS, "/inputInt1.txt");
  Serial.print("*** Your inputInt1: ");
  Serial.println(yourInputInt1);
  
  int yourInputInt2 = readFile(SPIFFS, "/inputInt2.txt").toInt();
  Serial.print("*** Your inputInt2: ");
  Serial.println(yourInputInt2);
  
  delay(1000);
}
