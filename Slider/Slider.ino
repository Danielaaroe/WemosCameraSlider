/*


ESP8266WebServer - Part of code revolving the ESP8266WebServer is partly sampled and greatly inspired by the solution proposed by Pieter P in "A Beguinner's Guide to the ESP8266" - Thanks.
*/
//OLED DISPLAY//////////////////////////////////////////////////////////
#include <Adafruit_SSD1306.h> //remember that this library must be installed through library manager. Remember to use the Wemos D1 mini version.

//ESP8266 WEB LIBRARIES//////////////////////////////////////////////////
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

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
int speedDir = 0;
int webSpeedDir = 0;

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
  server.on("/", HTTP_GET, handleRoot);        // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/config", HTTP_POST, handleConfig); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.onNotFound(handleNotFound);           // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin();                            // Actually start the server
  Serial.println("HTTP server started");
}

void loop()
{
  webMode = (analogRead(speedPin) < 100); //set webmode if potmeter is turned all the way to the left.
  server.handleClient();
  setSliderSpeed();
  updateDisplay();
}

void handleRoot() {                          // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", getRootHTML());
}

void handleConfig() {

  String webSpeedString = server.arg("speedinmmsec");
  webSpeedDir = webSpeedString.toInt();
  //futureProofing = int(server.arg("Futureproofing"));
  Serial.print("running handleconfig. speedinmmsec = ");
  Serial.println(webSpeedDir);
  server.send(200, "text/html", getConfigHTML() );
}

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
    display.println("mm/sec");
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

void setSliderSpeed() {

  if (!webMode) {
    speedDir = map(analogRead(speedPin), 0, 1023, -5000, 5000);
    if (speedDir > 0) {
      digitalWrite(dirPin, HIGH);
    } else {
      digitalWrite(dirPin, LOW);
    }
    analogWriteFreq(abs(speedDir));
  }
  if (webMode) {
    if (webSpeedDir > 0) {
      digitalWrite(dirPin, HIGH);
    } else {
      digitalWrite(dirPin, LOW);
    }
    analogWriteFreq(abs(webSpeedDir));
    //Serial.print("webmode speed: ");
    //Serial.println(webSpeedDir);
  }

  analogWrite(stepPin, 100);
}
/*
  void handleConfig() {                         // If a POST request is made to URI /login
  if( ! server.hasArg("speedinmmsec") || ! server.hasArg("futureproofing")
      || server.arg("username") == NULL || server.arg("password") == NULL) { // If the POST request doesn't have username and password data
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }
  if(server.arg("username") == "John Doe" && server.arg("password") == "password123") { // If both the username and the password are correct
    server.send(200, "text/html", "<h1>Welcome, " + server.arg("username") + "!</h1><p>Login successful</p>");
  } else {                                                                              // Username and password don't match
    server.send(401, "text/plain", "401: Unauthorized");
  }
  }
*/

//WEB PAGES//////////////////////////////////////////////////////////// Note that all the \ in the HTML is to cancel out the following " in order to create a continous string.
//NOTE THAT HTML STRINGS MAY ONLY BE DECLARED AT THE TOP OF THE SKETCH IF THEY ARE CONSTANT. IF VARIABLES ARE TO BE INCLUDED, THEY MUST BE UPDATED SOMEWHERE AFTER COMPILE TIME.
String getRootHTML() {
  String HTML = "SET SLIDER VALUES BELOW!!! "
                "<form action=\"/config\"method=\"POST\"><input type=\"text\" name=\"speedinmmsec\"placeholder=\"" + String(webSpeedDir) + "\"> <- Enter speed in whole mm/sec. </br> "
                "</br>"
                "<input type=\"text\" name=\"futureproofing\" placeholder=\"Furureproofing\"> <- Enter Futureproofing variable value</br>"
                "</br>"
                "<input type=\"submit\" value=\"Submit parameters\">"
                "</form>"
                "<p>You must submit in order to start</p> ";
  return HTML;
}

String getConfigHTML() {
  String HTML = "<form action=\"/\"method=\"POST\">"
                "You have changed the parameters </br> Rress button below to return to home page.</br> "
                "<input type=\"submit\" value=\"Return\">"
                "</form>";
  return HTML;
}
