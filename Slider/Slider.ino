
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

//WEB FEATURES////////////////////////////////////////////////////
//ESP8266WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
void handleRoot();              // function prototypes for HTTP handlers
void handleSetParameters();
void handleStart();
void handleStop();
void handleReturn();

const char *ssid = "Sliderfuck";
const char *password = "password";

  
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
 //WiFi.disconnect();
  WiFi.softAP(ssid, password);
  //Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_GET, handleRoot);        // Call the 'handleRoot' function when a client requests URI "/"
  //server.on("/login", HTTP_POST, handleLogin); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.onNotFound(handleNotFound);           // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin();                            // Actually start the server
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient(); 
  //unsigned long nextkeep = next;
  //Serial.println(nextkeep);

  speedDir = map(analogRead(speedPin), 0, 1023, -5000, 5000);
  if (speedDir > 0) {
    digitalWrite(dirPin, HIGH);
  } else {
    digitalWrite(dirPin, LOW);
  }



  analogWriteFreq(abs(speedDir));
  analogWrite(stepPin, 100);


  double paceInMms = speedDir * speedDir / 72.727;
  double speedInSm = (1000 / paceInMms) / 60; //seconds per meter
  //Serial.println(speedInmms);
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

void handleRoot() {                          // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", "<form action=\"/login\" method=\"POST\"><input type=\"text\" name=\"username\" placeholder=\"Username\"></br><input type=\"password\" name=\"password\" placeholder=\"Password\"></br><input type=\"submit\" value=\"Login\"></form><p>Try 'John Doe' and 'password123' ...</p>");
}
/*
void handleLogin() {                         // If a POST request is made to URI /login
  if( ! server.hasArg("username") || ! server.hasArg("password") 
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
void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
