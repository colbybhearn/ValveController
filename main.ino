// Timestamp: 20200715 2355

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

// The internal pin numbers differ from the actual pin numbers on the NodeMCU board
// The Dx numbers are what is actually on the NodeMCU board
const int STEP_PIN = 12; // D6
const int DIR_PIN = 13; // D7

// The stepper driver (SparkFun Easy Driver) in this code utilizes a pin for changing the direction so
// both the open and closed step values are the same. However other drivers use
// a negative and non negative number to dictate stepper direction. So open
// might be 1000 and close -1000

const int STEP_POS_OPEN = 1000;
const int STEP_POS_CLOSED = 1000;

int POS_CURR = 0; // current valve position (unreliable until after initialization)
const int POS_A = 0; // home 12 oclock
const int POS_B = 550; // bottom right
const int POS_C = 1100; // bottom left
const int DIR_CCW = 1; // counter clockwise direction
const int DIR_CW = 0; // clockwise direction
const char *MODE = "idle";

// Global instance variables for web server and stepper interface
ESP8266WebServer server(VALVE_WEBSERVER_PORT);

// Global variable for containing status of valve. Once more stepper functionality is added
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

// All initialization for AP functionality lives below. Currently this is standard AP configuration.
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
   server.on("/startTest", startTest);
   server.on("/stopTest", stopTest);
   server.on("/getStatus", getStatus);
   server.on("/toA", move_valve_A);
   server.on("/toB", move_valve_B);
   server.on("/toC", move_valve_C);

   server.begin();
   Serial.println("HTTP server started");
}

// Sets default position to open.
void stepper_controller_init() {
   move_stepper(STEP_POS_OPEN,DIR_CCW); // counter clockwise
   valve_open = true;
   POS_CURR = 0; // assert that we are home. 
}

// Handler for returning HTML and javascript that is rendered by the user in the browser to control the valve
void index_handler() {
   const char *web_page =
     "<!DOCTYPE html>"
     "<html>"
     "<head>"
     "<style>"
         ".lbtn {"
           "display: block; width: 100%; border: 2px; padding: 100px 28px; font-size: 50px; text-align: center; margin:10px; background-color:#79bbef;" 
           "}"
           ".label {"
           "display: block; width: 100%; border: 2px; padding: 10px 8px; font-size: 50px; text-align: center; margin:10px;" 
           "}"
         ".current {"
            " background-color:#D3D3D3;" // turn the button for the current position gray (as if it's grayed out, even though it's not) 
            "}"
       "</style>"
     "</head>"
     "<body>"
      "<span id='status' class='label' ></span>"
      "<span id='pos' class='label' ></span>"      
      "<button id='btnTest' class='lbtn' type='button' onclick='startTest()'>Test Mode</button>"
      "<button id='btnA' class='lbtn' type='button' onclick='moveToA()'>Position A</button>"
      "<button id='btnB' class='lbtn' type='button' onclick='moveToB()'>Position B</button>"
      "<button id='btnC' class='lbtn' type='button' onclick='moveToC()'>Position C</button>"
       "<script>"
         "let position = 'A';"         
         "function valveOperation(operation) { fetch((operation == 'open' ? '/open' : '/close'));}"
         "function setPosLabel(text) { document.querySelector('#pos').innerText = text }"
         "function setStatusLabel(text) { document.querySelector('#status').innerText = text }"
         "function setCurrent(id){ document.querySelectorAll('.current').forEach((f)=>{f.classList.remove('current');}); document.querySelector('#'+id).classList.add('current'); }"
         "function moveToA(){ fetch('/toA').then(response => response.json()).then(function(data) { setPosLabel(data.position); position = data.position; setCurrent('btnA');}).catch(function(error){console.log('error moving to A',  error.message)});}"
         "function moveToB(){ fetch('/toB').then(response => response.json()).then(function(data) { setPosLabel(data.position); position = data.position; setCurrent('btnB');}).catch(function(error){console.log('error moving to B',  error.message)});}"
         "function moveToC(){ fetch('/toC').then(response => response.json()).then(function(data) { setPosLabel(data.position); position = data.position; setCurrent('btnC');}).catch(function(error){console.log('error moving to C',  error.message)});}"
         "function startTest(){ fetch('/startTest').then(response => response.json()).then(function(data) { setStatusLabel('starting test'); setTimeout(update, 1000); }).catch(function(error){console.log('error displaying testing status',  error.message)});}"
         "function startTest(){ fetch('/stopTest').then(response => response.json()).then(function(data) { setStatusLabel('stopping test'); }).catch(function(error){console.log('error displaying testing status',  error.message)});}"
         "function update(){ fetch('/getStatus').then(response => response.json()).then(function(data) { if(data.mode === 'test') processUpdate_TestMode(data); if(data.mode !== 'idle') setTimeout(update, 1000); }).catch(function(error){console.log('error displaying testing status',  error.message)});}"
         "function processUpdate_TestMode(data){ setStatusLabel('testing' + data.stepsRemaining); }"
       "</script>"
     "</html>"
     "</body>";

  server.send(200,"text/html",web_page);
}

// Generic health check endpoint you can check to get a response from the NodeMCU
// Additional watchdog logic can go here
void health_check_handler() {
   const char *health_json_resp = "{'status': 'ok'}";
   server.send(200,"application/json", health_json_resp);
}

int getStepsToTarget(int target){
   if(POS_CURR > target)
      return POS_CURR-target;
   return target-POS_CURR;
}

int getDirectionToTarget(int target){
   if(POS_CURR > target)
      return DIR_CCW;
   return DIR_CW;
}


void startTest(){
   MODE = "test";
   const char *open_json_resp = "{'mode': '"+MODE+"'}";
   server.send(200, "application/json", open_json_resp);
   
   // I need to run the test.  
   // I need to know where to tie into an infinite loop
   // I need to know how to properly sleep through the loop
      
   // start tracking test position
   // while in test mode
      // target next test position
      // delay 1000
      // advance test position
   
   // finally, set the mode back to idle
   MODE = "idle";
   
   //   int steps = getStepsToTarget(POS_A);
   // int direction = getDirectionToTarget(POS_A);
   // move_stepper(steps,direction);     
}

void getStatus(){   
   // if we're in test mode,
      // return getTestStatus();
   
   // return getIdleStatus();
}

void getTestStatus(){
   const char *open_json_resp = "{'mode': '"+MODE+"'}";
   server.send(200, "application/json", open_json_resp);   
}

void getIdleStatus(){
   const char *open_json_resp = "{'mode': '"+MODE+"'}";
   server.send(200, "application/json", open_json_resp);   
}

void move_valve_A(){
   int steps = getStepsToTarget(POS_A);
   int direction = getDirectionToTarget(POS_A);
   move_stepper(steps,direction);
   
   const char *open_json_resp = "{'position': 'A'}";
   server.send(200, "application/json", open_json_resp);
}

void move_valve_B(){
   int steps = getStepsToTarget(POS_B);
   int direction = getDirectionToTarget(POS_B);
   move_stepper(steps,direction);
   
   const char *open_json_resp = "{'position': 'B'}";
   server.send(200, "application/json", open_json_resp);
}

void move_valve_C(){
   int steps = getStepsToTarget(POS_C);
   int direction = getDirectionToTarget(POS_C);
   move_stepper(steps,direction);
   
   const char *open_json_resp = "{'position': 'C'}";
   server.send(200, "application/json", open_json_resp);
}


// If global valve status is close, it moves stepper to the OPEN step count
// and returns a successful json message.
void open_valve_handler() {
   if (!valve_open) {
      move_stepper(STEP_POS_OPEN,DIR_CCW);
      valve_open = true;
   }

   const char *open_json_resp = "{'position': 'open'}";
   server.send(200, "application/json", open_json_resp);
}

// Same as above, but for closed.
void close_valve_handler() {
   if(valve_open) {
       move_stepper(STEP_POS_CLOSED,DIR_CW);
      valve_open = false;
   }

   const char *closed_json_resp = "{'position': 'closed'}";
   server.send(200, "application/json", closed_json_resp);
}



// Step count is how many full steps you want to move and direction is either 1 or 0
void move_stepper(int step_count, int direction) {
   digitalWrite(DIR_PIN, direction ? DIR_CCW : DIR_CW);
   int stepDelta = 1;
   if(direction == DIR_CCW)
      stepDelta *= -1;

     for(int i = 0; i < step_count ; i++) {
       
       digitalWrite(STEP_PIN,LOW);
       delay(1);
       digitalWrite(STEP_PIN,HIGH);
       
          POS_CURR += stepDelta; // record progress in the desired direction
       
     }
}

// Authors: unknown, colbybhearn@gmail.com
