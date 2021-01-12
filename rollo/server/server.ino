/*
  Window Blinds by Viet Nguyen
  Features:
    - Auto Update firmware with /update or curl -F "image=@HelloServerWorking.ino.nodemcu.bin" esp8266-v.local/update
    - To open/close use endpoints /open /close
      - By default it uses the speed and duration set in the EEPROM
      - Overridable values: speed, duration, dutyCycle
    - Configuration
      - Get: GET /config
      - Set: GET /config with the following values in the query string:
        - Speed: Speed at which the motor turns when opening / closing
          - Allowed values: 1 for slow 2 or fast
        - Duration: Duration at which the motor turns when opening / closing
          - Allowed values: 1000 - 9999 (milliseconds)
    - Version: /version
  https://github.com/vietquocnguyen/NodeMCU-ESP8266-Servo-Smart-Blinds    
*/

/*
Libraries
Esp8266
ArduinoJson 6
RF24
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>
#include <math.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define VERSION 0
#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASSWORD ""
#define DEFAULT_HOSTNAME "esp-bridge"
#define DEFAULT_SPEED "2" // 1 or 2
#define DEFAULT_DURATION "3500" // 9 characters 1000-9999
#define DEFAULT_LAST_ACTION "0"
#define VER "2" // EEPROM version 
#define DEFAULT_AP_SSID "esp-bridge-123"

RF24 radio(D2, D8); //CE, CSN
const byte address[6] = "00001";

char host[25];
char ssid[25];
char password[25];
int tryCount = 0;

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

void setup(void){

  Serial.begin(115200);
  EEPROM.begin(512);

  Serial.println('START APP');

  if(getData(84,86,"0") == VER) {
    // Yay I know you
  } else {
    // I don't know you
    clearEEPROM();
    setData(84, 86, VER); // Getting to know you
  }

  String theSSID = getData(0,25, DEFAULT_WIFI_SSID);
  String thePassword = getData(26,50, DEFAULT_WIFI_PASSWORD);
  String theHostname = getData(61,80, DEFAULT_HOSTNAME);

  if (theSSID.length() == 0) {

    Serial.print("Setting soft-AP ... ");
    Serial.println(WiFi.softAP(DEFAULT_AP_SSID) ? "Ready" : "Failed!");

    Serial.print("Soft-AP IP address: ");
    Serial.println(WiFi.softAPIP());

  } else {
    Serial.println("Trying " + theSSID + " / " + thePassword + " / " + theHostname);
    
    theSSID.toCharArray(ssid, 25);
    thePassword.toCharArray(password, 25);
    theHostname.toCharArray(host, 25);
    
    WiFi.begin(ssid, password);
    Serial.println("Started");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      tryCount = tryCount + 1;
      Serial.println("Try #" + String(tryCount) + " ");
      if(tryCount > 30){
        Serial.println("Giving up, reverting back to default Wifi settings");
        setData(0,25, DEFAULT_WIFI_SSID);
        setData(26,50, DEFAULT_WIFI_PASSWORD);
        setData(61,80, DEFAULT_HOSTNAME);
        delay(1000);
        WiFi.disconnect();
        ESP.reset();
      }
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin(host)) {
      Serial.println("MDNS responder started");
    }
  }

  server.on("/", []() {
    if (server.method() == HTTP_GET) {
      server.send(200, "text/html", "<html><head><title>ESP Bridge</title><meta name='viewport' content='width=device-width, initial-scale=1'></head><body><form action='/' method='POST'><input type='text' name='SSID' placeholder='SSID' /><br/><input type='password' name='PASSWORD' placeholder='Password' /><br/><input type='text' name='HOST' placeholder='Hostname' value='esp-bridge' /><br/><button>Save</button></form></body></html>");
    } else {
      Serial.println('New settings');
      if(server.hasArg("SSID")){
        Serial.println("New settings - SSID: " + String(server.arg("SSID")));
        setData(0,25, server.arg("SSID"));
      }
      if(server.hasArg("PASSWORD")){
        Serial.println("New settings - PASSWORD");
        setData(26,50, server.arg("PASSWORD"));
      }

      if(server.hasArg("HOST")){
        Serial.println("New settings - HOST");
        setData(61,80, server.arg("HOST"));
      }

      server.send(200, "text/html", "<html><body>Changes detected, restarting device</body></html>");
      delay(1000);
      ESP.reset();
    }
  });

  server.on("/version", [](){
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, "{}");
    JsonObject json = jsonBuffer.as<JsonObject>();
    Serial.println("version");
    json["version"] = VERSION;
    String output;
    serializeJsonPretty(jsonBuffer, output);
    server.send(200, "application/json", output);
  });

  server.on("/status", [](){
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, "{}");
    JsonObject json = jsonBuffer.as<JsonObject>();
    Serial.println("status");
    json["isOpen"] = getData(81,83,DEFAULT_LAST_ACTION) == "1";
    String output;
    serializeJson(jsonBuffer, output);
    Serial.print("Output: ");
    Serial.println(output.c_str());

    server.send(200, "application/json", output.c_str());
  });

  server.on("/clear", [](){
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, "{}");
    JsonObject json = jsonBuffer.as<JsonObject>();
    Serial.println("clear");
    json["message"] = "Clearing EEPROM data and resetting";
    String output;
    serializeJsonPretty(jsonBuffer, output);
    server.send(200, "application/json", output);

    clearEEPROM();
    WiFi.disconnect();
    ESP.reset();
  });

  server.on("/reset", [](){
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, "{}");
    JsonObject json = jsonBuffer.as<JsonObject>();
    json["message"] = "resetting";
    Serial.println("Resetting");
    String output;
    serializeJsonPretty(json, output);
    server.send(200, "application/json", output);
    WiFi.disconnect();
    ESP.reset();
  });

  server.on("/open", [](){
    openOrClose(1);
  });

  server.on("/close", [](){
    openOrClose(0);
  });

  server.on("/config", [](){
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, "{}");
    JsonObject json = jsonBuffer.as<JsonObject>();

    int theSpeed = getData(51,55, DEFAULT_SPEED).toInt();
    if(server.hasArg("speed")){
      theSpeed = server.arg("speed").toInt();
      setData(51,55, String(theSpeed));
    }
    json["speed"] = theSpeed;

    int theDuration = getData(81,90, DEFAULT_DURATION).toInt();
    if(server.hasArg("duration")){
      theDuration = server.arg("duration").toInt();
      setData(81,90, String(theDuration));
    }
    json["duration"] = theDuration;


    String output;
    serializeJsonPretty(jsonBuffer, output);
    server.send(200, "application/json", output);
  });

  server.on("/wifi", [](){
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, "{}");
    JsonObject json = jsonBuffer.as<JsonObject>();
    bool resetMe = false;

    String aSsid = getData(0,25, DEFAULT_WIFI_SSID);
    if(server.hasArg("ssid")){
      aSsid = setData(0,25, server.arg("ssid"));
      resetMe = true;
    }
    json["ssid"] = aSsid;

    if(server.hasArg("password")){
      aPassword = setData(26,50, server.arg("password"));
      resetMe = true;
    }

    String aHostname = getData(61,80, DEFAULT_HOSTNAME);
    if(server.hasArg("hostname")){
      aHostname = setData(61,80, server.arg("hostname"));
      resetMe = true;
    }
    json["hostname"] = aHostname;

    json["ip"] = WiFi.localIP().toString();

    String clientMac = "";
    unsigned char mac[6];
    WiFi.macAddress(mac);
    clientMac += macToStr(mac);
    clientMac.toUpperCase();
    json["mac"] = clientMac;

    if(resetMe){
      json["message"] = "Changes detected, now resetting.";
    }

    String output;
    serializeJsonPretty(jsonBuffer, output);
    server.send(200, "application/json", output);

    if(resetMe){
      ESP.reset();
    }
  });

  server.on("/msg", [](){
    DynamicJsonDocument jsonBuffer(1024);
    deserializeJson(jsonBuffer, "{}");
    JsonObject json = jsonBuffer.as<JsonObject>();
    Serial.println("message");
    String msg = "hello world!!!";
    if(server.hasArg("text")) {
      msg = server.arg("text");
    }
    radio.write(msg.c_str(), msg.length());
    json["message"] = "Sent message to radio";
    json["text"] = msg;
    String output;
    serializeJsonPretty(jsonBuffer, output);
    server.send(200, "application/json", output);
  });

  server.onNotFound(handleNotFound);

  httpUpdater.setup(&server);
  server.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.println("HTTP server started");

  Serial.println("SETUP TRANSMITER");
  radio.begin();
  Serial.println(radio.isChipConnected());
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
}

String macToStr(const uint8_t* mac){
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void openOrClose(int direction) {
  DynamicJsonDocument jsonBuffer(1024);
  JsonObject json = jsonBuffer.as<JsonObject>();

  bool toSpin = true;

  String lastAction = getData(81,83, DEFAULT_LAST_ACTION);
  if(lastAction == String(direction)){
    toSpin = false;
    json["message"] = "did not spin";
  }

  int theSpeed = getData(51,55, DEFAULT_SPEED).toInt();
  if(server.hasArg("speed")){
    theSpeed = server.arg("speed").toInt();
  }
  int theDuration = getData(81,90, DEFAULT_DURATION).toInt();
  if(server.hasArg("duration")){
    theDuration = server.arg("duration").toInt();
  }
  int dutyCycle = calculateDutyCycleFromSpeedAndDirection(theSpeed, direction);
  if(server.hasArg("dutyCycle")){
    dutyCycle = server.arg("dutyCycle").toInt();
  }

  json["action"] = direction ? "open" : "close";
  json["speed"] = theSpeed;
  json["duration"] = theDuration;
  json["calulatedDutyCycle"] = dutyCycle;

  String output;
  serializeJsonPretty(json, output);
  server.send(200, "application/json", output);

  if(toSpin) {
    setData(81,83,String(direction));
  }
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

int calculateDutyCycleFromSpeedAndDirection(int speed, int direction){
  if(speed == 1 && direction == 1){
    return 80;
  } else if(speed == 2 && direction == 1){
    return 0;
  } else if(speed == 1 && direction == 0){
    return 95;
  } else if(speed == 2 && direction == 0){
    return 180;
  } else{
    return 180;
  }
}

// Get EEPROM Data
String getData(int startingIndex, int endingIndex, String defaultValue){
  String data;
  for (int i = startingIndex; i < endingIndex; ++i) {
    String result = String(char(EEPROM.read(i)));
    if(result != ""){
      data += result; 
    }
  }
  if(!data.length()){
    return defaultValue;
  }else{
    return data;
  }
}

// Set EEPROM Data
String setData(int startingIndex, int endingIndex, String settingValue){
  for (int i = startingIndex; i < endingIndex; ++i){
    EEPROM.write(i, settingValue[i-startingIndex]);
    Serial.print("Wrote: ");
    Serial.println(settingValue[i-startingIndex]); 
  }

  EEPROM.commit();
  delay(250);
  return settingValue;
}

void presentation() {
    // Present locally attached sensors here
}

void loop() {
  server.handleClient();
  /*
  if (radio.available()) {
    Serial.print("Have message: ");
    char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);
    radio.flush_rx();
  }
  */
}

void clearEEPROM(){
  for (int i = 0 ; i < 512 ; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  delay(500);
}
