#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// Wifi network name and password for the NodeMCU valve control board
#ifndef APSSID
#define APSSID "Test123"
#define APPSK  "password"
#endif

// Port for the NodeMCU webserver - 80 is standard HTTP port
const int VALVE_WEBSERVER_PORT = 80;

// Wifi network name and password for the NodeMCU valve control board
const char *VALVE_AP_SSID = "Test123";
const char *VALVE_AP_PASSWORD = "password";

// Baud rate for serial debugging
const int DEBUG_SERIAL_BAUD = 115200;

// The internal pin numbers differ from the actual pin numbers on the
NodeMCU board
// The Dx numbers are what is actually on the NodeMCU board
const int STEP_PIN = 12; // D6
const int DIR_PIN = 13; // D7

// The stepper driver (SparkFun Easy Driver) in this code utilizes a pin
for changing the direction so
// both the open and closed step values are the same. However other
drivers use
// a negative and non negative number to dictate stepper direction. So open
// might be 1000 and close -1000

const int STEP_POS_OPEN = 1000;
const int STEP_POS_CLOSED = 1000;


// Global instance variables for web server and stepper interface
ESP8266WebServer server(VALVE_WEBSERVER_PORT);

// Global variable for containing status of valve. Once more stepper
functionality is added
// this would go in a sep class
bool valve_open = true;

// Hardware init and control functions
void ap_init();
void stepper_controller_init();
void move_stepper(int step_count, int direction);

// Web server route handler functions
void health_check_handler();

// Handlers for individual HTTP request routes
// /index renders the HTML / javascript form
// /open sends and open request to the valve
// /close sends a close request to the valve

void index_handler();
void open_valve_handler();
void close_valve_handler();

// Main Arduino setup routine
// Set both the step and direction pin as output
void setup() {

   pinMode(STEP_PIN, OUTPUT);
   pinMode(DIR_PIN, OUTPUT);

   ap_init();
   stepper_controller_init();
}

// Infinite loop for handling all incoming HTTP requests
void loop() {
   server.handleClient();

}

// All initialization for AP functionality lives below. Currently this
is standard AP configuration.
// Default IP address for the AP is 192.168.4.1 .This address is
// not user configurable.
void ap_init() {

   delay(1000);

   Serial.begin(DEBUG_SERIAL_BAUD);
   Serial.print("\nSetting up valve controller AP\n");
   WiFi.softAP(VALVE_AP_SSID, VALVE_AP_PASSWORD);

   server.on("/",index_handler);
   server.on("/health", health_check_handler);
   server.on("/open", open_valve_handler);
   server.on("/close", close_valve_handler);

   server.begin();
   Serial.println("HTTP server started");
}

// Sets default position to open.
void stepper_controller_init() {
   move_stepper(STEP_POS_OPEN,1);
   valve_open = true;
}

// Handler for returning HTML and javascript that is rendered by the
user in the browser to control the valve
void index_handler() {
   const char *web_page =
     "<!DOCTYPE html>"
     "<html>"
     "<body>"
       "<button class =\"lbtn\" type=\"button\"
onclick=\"valveOperation('open')\">Open Valve</button>"
       "<button class =\"lbtn\" type=\"button\"
onclick=\"valveOperation('close')\">Close Valve</button>"
       "<style>"
         ".lbtn {"
           "display: block; width: 100%; border: 2px; padding: 100px
28px; font-size: 50px; text-align: center; margin:10px;
background-color:#D3D3D3;"
           "}"
       "</style>"
       "<script>"
         "function valveOperation(operation) { fetch((operation ==
'open' ? '/open' : '/close'));}"
       "</script>"
     "</html>"
     "</body>";

  server.send(200,"text/html",web_page);
}

// Generic health check endpoint you can check to get a response from
the NodeMCU
// Additional watchdog logic can go here
void health_check_handler() {
   const char *health_json_resp = "{\"status\": \"ok\"}";
   server.send(200,"application/json", health_json_resp);
}

// If global valve status is close, it moves stepper to the OPEN step count
// and returns a successful json message.
void open_valve_handler() {
   if (!valve_open) {
      move_stepper(STEP_POS_OPEN,1);
      valve_open = true;
   }

   const char *open_json_resp = "{\"position\": \"open\"}";
   server.send(200, "application/json", open_json_resp);
}

// Same as above, but for closed.
void close_valve_handler() {
   if(valve_open) {
       move_stepper(STEP_POS_CLOSED,0);
      valve_open = false;
   }

   const char *closed_json_resp = "{\"position\": \"closed\"}";
   server.send(200, "application/json", closed_json_resp);
}

// Step count is how many full steps you want to move and direction is
either 1 or 0
void move_stepper(int step_count, int direction) {
   digitalWrite(DIR_PIN, direction ? 1 : 0);
     for(int i = 0; i < step_count ; i++) {
       digitalWrite(STEP_PIN,LOW);
       delay(1);
       digitalWrite(STEP_PIN,HIGH);
     }
}
