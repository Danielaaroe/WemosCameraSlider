/*


ESP8266WebServer - Part of code revolving the ESP8266WebServer is partly sampled and greatly inspired by the solution proposed by Pieter P in "A Beguinner's Guide to the ESP8266" - Thanks.
*/
//OLED DISPLAY//////////////////////////////////////////////////////////
#include <Adafruit_SSD1306.h> //remember that this library must be installed through library manager. Remember to use the Wemos D1 mini version.

//ESP8266 WEB LIBRARIES//////////////////////////////////////////////////
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include "webpage.h"

//OLED///////////////////////////////////////////////////////////////////
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

//STEPPER//////////////////////////////////////////////////////////////
int stepperSpeed = 0;
#define speedPin A0 //potmeter input pin
#define MS1Pin D0
#define MS2Pin D8
#define MS3Pin D7 //only applicable for A4988 16uSteps
#define stepPin D6
#define dirPin D5  // if direction matters, this must be inverted when switching between A4988 and TMC2208
#define enablePin D3 //Pull down to enable, up to disable.
int webSpeedDir = 0; //Remove when code is refactored.
int speedDir = 0;
int webSpeedValue = 0; //speed
String webDirection = ""; //left or right(motor on left)
int webDuration = 0; //duration in seconds
bool webStartStop = 0;

//WEB FEATURES////////////////////////////////////////////////////
IPAddress serverIP(10, 0, 0, 1);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);
//ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
void handleRoot();              // function prototypes for HTTP handlers
void handleConfig();
//void handleStart();
//void handleStop();
//void handleReturn();
const char *ssid = "Slider";
const char *password = "sliderslider"; //remember password length longer than 10 chars.

//VARIABLES////////////////////////////////////////////////////////////////
long lastTimeAlive, timePassed = 0;

bool webMode = LOW;

void setup()
{
  delay(1000);
  //SERIAL PORT//
  Serial.begin(115200);
  Serial.println();
  //STEPPER//
  pinMode(A0, INPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(stepPin, OUTPUT);

  pinMode(MS1Pin, OUTPUT);
  pinMode(MS2Pin, OUTPUT);
  pinMode(MS3Pin, OUTPUT);
  pinMode(enablePin, OUTPUT);

  digitalWrite(MS1Pin, HIGH);
  digitalWrite(MS2Pin, HIGH);
  digitalWrite(MS3Pin, LOW);       //CHANGED FOR TMC2208

  digitalWrite(enablePin, LOW);
  digitalWrite(dirPin, HIGH);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  display.display(); //display whats in the display buffer on the display.

  //WEB FEATURES//
  // WiFi.mode(STA_AP);
  WiFi.disconnect(true); //there seems to be an issue with IP conflict or similar if it is connected to the home network in STA mode while in AP mode. This clears configuration on boot.
  WiFi.softAPConfig(serverIP, gateway, subnet);
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_ANY, handleRoot);        // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/startstop", HTTP_POST, handleStartStop);       //Handle POST Start/stop command from web 
  server.onNotFound(handleNotFound);           // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin();                            // Actually start the server
  Serial.println("HTTP server started");
}

void loop()
{
  webMode = (analogRead(speedPin) < 10); //set webmode if potmeter is turned all the way to the left.
  server.handleClient();
  setSliderSpeed();
  updateDisplay();

  //Print alive every 1 second.
  if ((millis() - lastTimeAlive) >= 1000) { 
    Serial.println("Still alive and has been for " + String(millis()));
    lastTimeAlive = millis();
  }

}

void handleRoot() {                          // When URI / is requested, send a web page with the form to configure settings.
  //server.send(200, "text/html", getRootHTML());
  Serial.println(server.method());
  if (server.method() == HTTP_GET) {
    Serial.println("Got GET request");
    server.send(200, "text/html", getRootHTML());
  }else if(server.method() == HTTP_POST) {
    Serial.println("Got POST request");
    server.send(200, "text/html", createHTML());
  }
}

void handleStartStop() {

  //for (int i = 1; i < server.args(); i++){
  //  Serial.println(server.argName(i) + ": " + server.arg(i)); 
  //}
  if(server.method() == HTTP_POST) {
    Serial.println("Got POST Start request");
    server.send(200, "text/html", createHTML());
  }
}

//void handleConfig() {
//
//  String webSpeedString = server.arg("speedinmmsec");
//  webSpeedDir = webSpeedString.toInt();
//  //futureProofing = int(server.arg("Futureproofing"));
//  Serial.print("running handleconfig. speedinmmsec = ");
//  Serial.println(webSpeedDir);
//  server.send(200, "text/html", getConfigHTML() );
//}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void updateDisplay() {
  double paceInMms = speedDir * speedDir / 72.727;
  double speedInSm = (1000 / paceInMms) / 60; //seconds per meter
  if (!webMode) {
    //  clear display and set cursor on the top left corner
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.println(paceInMms);
    display.setCursor(0, 15); //(X/Y) position
    display.setTextSize(1);
    display.println("mm/min");
    display.setTextSize(2);
    display.setCursor(0, 24); //(X/Y) position
    display.print(speedInSm);
    display.setCursor(0, 39); //(X/Y) position
    display.setTextSize(1);
    display.print("min/m");
    display.display(); //Print display data buffer to screen
  }
  if (webMode) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("WEBMODE");
    display.setCursor(0, 10); //(X/Y) position
    display.setTextSize(1);
    display.println("SSID:");
    display.setCursor(0, 20); //(X/Y) position
    display.setTextSize(1);
    display.println(String(ssid));
    display.setTextSize(1);
    display.setCursor(0, 30); //(X/Y) position
    display.println("IP Address");
    display.setCursor(0, 40); //(X/Y) position
    display.setTextSize(1);
    display.print(WiFi.softAPIP());
    display.display(); //Print display data buffer to screen
  }
}

void setSliderSpeed() { //Analogwritefeq must never be 0. It makes the code crash.

  if (!webMode) {
    speedDir = map(analogRead(speedPin), 0, 1023, -5000, 5000);
    if (speedDir > 0) {
      digitalWrite(dirPin, HIGH);
    } else {
      digitalWrite(dirPin, LOW);
    }
    if (abs(speedDir) == 0){
      analogWriteFreq(1);
    }else{
      analogWriteFreq(abs(speedDir));
    }
  }
  if (webMode) {
    if (webSpeedDir > 0) {
      digitalWrite(dirPin, HIGH);
    } else {
      digitalWrite(dirPin, LOW);
    }
    if (abs(speedDir) == 0){
      analogWriteFreq(1);
    }else{
      analogWriteFreq(abs(speedDir));
    }

  }

  analogWrite(stepPin, 100);
}


//WEB PAGES//////////////////////////////////////////////////////////// Note that all the \ in the HTML is to cancel out the following " in order to create a continous string.
//NOTE THAT HTML STRINGS MAY ONLY BE DECLARED AT THE TOP OF THE SKETCH IF THEY ARE CONSTANT. IF VARIABLES ARE TO BE INCLUDED, THEY MUST BE UPDATED SOMEWHERE AFTER COMPILE TIME.
String getRootHTML() {
  return index_html_get;
}

String createHTML() {
  //for (int i = 1; i < server.args(); i++){
  //  Serial.println(server.argName(i) + ": " + server.arg(i)); 
  //}

  webSpeedValue = server.arg(1).toInt();
  webDirection = String(server.arg(2));
  webDuration = server.arg(3).toInt();

String HTML =

"<!DOCTYPE html>"
"<html lang=\"en\">"
"<head>"
"    <meta charset=\"UTF-8\">"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"    <title>Camera Slider Control</title>"
"    <style>"
"        body {"
"            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;"
"            max-width: 400px;"
"            margin: 50px auto;"
"            padding: 20px;"
"            background: #0a0a0a;"
"        }"
"        "
"        .container {"
"            background: linear-gradient(145deg, #1e1e1e, #2a2a2a);"
"            padding: 40px;"
"            border-radius: 24px;"
"            box-shadow: 0 20px 60px rgba(0,0,0,0.8), 0 0 1px rgba(255,255,255,0.1) inset;"
"            border: 1px solid rgba(255,255,255,0.05);"
"        }"
"        "
"        h1 {"
"            margin-top: 0;"
"            margin-bottom: 30px;"
"            color: #ffffff;"
"            font-weight: 600;"
"            font-size: 28px;"
"            letter-spacing: -0.5px;"
"        }"
"        "
"        .form-group {"
"            margin-bottom: 20px;"
"        }"
"        "
"        label {"
"            display: block;"
"            margin-bottom: 8px;"
"            color: #a0a0a0;"
"            font-weight: 500;"
"            font-size: 14px;"
"            letter-spacing: 0.3px;"
"        }"
"        "
"        input[type=\"number\"],"
"        select {"
"            width: 100%;"
"            padding: 12px 16px;"
"            border: 1px solid rgba(255,255,255,0.1);"
"            border-radius: 12px;"
"            font-size: 16px;"
"            box-sizing: border-box;"
"            background: rgba(0,0,0,0.3);"
"            color: #ffffff;"
"            transition: all 0.3s ease;"
"        }"
"        "
"        input[type=\"number\"]:focus,"
"        select:focus {"
"            outline: none;"
"            border-color: #007bff;"
"            background: rgba(0,0,0,0.4);"
"        }"
"        "
"        input[type=\"range\"] {"
"            width: 100%;"
"            margin-bottom: 10px;"
"            height: 6px;"
"            background: rgba(255,255,255,0.1);"
"            border-radius: 10px;"
"            outline: none;"
"            -webkit-appearance: none;"
"        }"
"        "
"        input[type=\"range\"]::-webkit-slider-thumb {"
"            -webkit-appearance: none;"
"            appearance: none;"
"            width: 20px;"
"            height: 20px;"
"            background: #007bff;"
"            border-radius: 50%;"
"            cursor: pointer;"
"            box-shadow: 0 2px 8px rgba(0,123,255,0.4);"
"        }"
"        "
"        input[type=\"range\"]::-moz-range-thumb {"
"            width: 20px;"
"            height: 20px;"
"            background: #007bff;"
"            border-radius: 50%;"
"            cursor: pointer;"
"            border: none;"
"            box-shadow: 0 2px 8px rgba(0,123,255,0.4);"
"        }"
"        "
"        input[type=\"number\"] {"
"            width: 120px;"
"        }"
"        "
"        button {"
"            width: 100%;"
"            padding: 14px;"
"            background: linear-gradient(135deg, #007bff, #0056b3);"
"            color: white;"
"            border: none;"
"            border-radius: 12px;"
"            font-size: 16px;"
"            cursor: pointer;"
"            font-weight: 600;"
"            transition: all 0.3s ease;"
"            box-shadow: 0 4px 15px rgba(0,123,255,0.3);"
"        }"
"        "
"        button:hover {"
"            transform: translateY(-2px);"
"            box-shadow: 0 6px 20px rgba(0,123,255,0.4);"
"        }"
"        "
"        button:active {"
"            transform: translateY(0);"
"        }"
"    </style>"
"</head>"
"<body>"
"    <div class=\"container\">"
"        <h1>Camera Slider</h1>"
"        <form action=\"\" method=\"POST\">"
"            <div class=\"form-group\">"
"                <label for=\"speed\">Speed (mm/min) (" + String(webSpeedValue) + "mm/min)</label>"
"                <input type=\"range\" id=\"speed\" name=\"speed\" min=\"1\" max=\"1000\" value=\"" + String(webSpeedValue) + "\" oninput=\"document.getElementById('speedValue').value = this.value\">"
"                <input type=\"number\" id=\"speedValue\" name=\"speedValue\" min=\"1\" max=\"1000\" value=\"" + String(webSpeedValue) + "\" oninput=\"document.getElementById('speed').value = this.value\">"
"            </div>"
"            "
"            <div class=\"form-group\">"
"                <label for=\"direction\">Direction (" + String(webDirection) + ")</label>"
"                <select id=\"direction\" name=\"direction\" required>"
"                    <option value=\"left\">Left</option>"
"                    <option value=\"right\">Right</option>"
"                </select>"
"            </div>"
"            "
"            <div class=\"form-group\">"
"                <label for=\"duration\">Duration (seconds) (" + String(webDuration) + "s)</label>"
"                <input type=\"number\" id=\"duration\" name=\"duration\" min=\"1\" max=\"1000\" value=\"" + String(webDuration) + "\" required>"
"            </div>"
"            "
"            <button type=\"submit\">Set values</button>"
"        </form>"
"        <form action=\"/startstop\" method=\"POST\">"
"            <button type=\"submit\">Start</button>"
"        </form>"
"    </div>"
"</body>"
"</html>";

  return HTML;
}


