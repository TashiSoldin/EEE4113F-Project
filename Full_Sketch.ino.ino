// ------------------------------------ WEBSERVER -----------------------------------------------

// Necessary includes
#include <WiFi.h>
#include <WebServer.h>
#include <TimeLib.h>

// SSID & Password for AP connection
const char* ssid = "ESP32"; 
const char* password = "12345678";

// Put IP Address details
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// Start webserver on port 80
WebServer server(80);

// ------------------------------------ LOAD CELL -----------------------------------------------

// Necessary includes
#include <Arduino.h>
#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 27;
const int LOADCELL_SCK_PIN = 22;

// Declare scale variable
HX711 scale;

// ------------------------------------ DATA PROCESSING -----------------------------------------

// Keep track of relevant variables
String status = "SLEEPING"; // Can be changed to "PROCESSING"

float temp; // Holds a sum of temperatures so that the average temperature can later be calculated

int num_elements = 0; // Stores the number of initialized elements in the following 3 arrays:
float previous_temps[10]; // Store a record of 10 previous temperatures in memory in case the observer needs to go away for a while
float previous_weights[10]; // Store a record of 10 previous weights in memory in case the observer needs to go away for a while
char* timestamps[10]; // Store a record of 10 previous timestamps in memory in case the observer needs to go away for a while
int second_times[10]; // Store a record of 10 second times corresponding to the timestamps - these are for graph plotting!

float weight_readings[50]; // 50 data points per weight reading gives a processing range of around 25 seconds
float time_readings[50]; // 50 data points of time (in seconds) from when the processing of weight data started
float temperature_readings[50]; // 50 data points of time (in seconds) from when the processing of weight data started

// ------------------------------------ SETUP ---------------------------------------------------

void setup() {

  // Set baud rate
  Serial.begin(115200);

  // Webserver setup ---------------------------------
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  server.on("/", handle_OnConnect);
  server.on("/results", get_results);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");

  // Initializing arrays -----------------------------
  for (int i = 0; i < 10; i++){
    weight_readings[i] = 0;
    time_readings[i] = 0;
    temperature_readings[i] = 0;
  }

  // Set time appropriately
  setTime(0, 0, 0, 0, 0, 0);

  // Scale setup -------------------------------------
  Serial.println("Initializing the scale");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Check ADC
  Serial.print("ADC test read: \t\t");
  Serial.println(scale.read());

  // Set calibration factor = (reading)/(known weight)
  scale.set_scale(2350);
  scale.tare(); // Reset the scale to 0

  // Ready to print out some readings
  Serial.println("Readings:");
}

// ----------------------------------------------- LOOP -----------------------------------------

void loop() {

  delay(500);
  server.handleClient();

  // Load cell Handling -------------------------------------------

  float weight = round(scale.get_units()*10.0)/10.0; // Multiply and divide by 10.0 is to format to one decimal point

  if (weight > 100){ // If the measured weight is over 100 grams
    temp = 0.0;
    status = "PROCESSING"; // This will change Web Server HTML

    Serial.println("Starting processing!"); // Indicate that processing has started 

    for (int i = 0; i < 50; i++){ // Loop 50 times - takes approximately 25 seconds to read in measurements

      server.handleClient(); // We need to still handle the client while reading in data

      // TAKE MEASUREMENTS FOR DATA PROCESSING

      float weight = round(scale.get_units()*10.0)/10.0; // Multiply and divide by 10.0 is to format to one decimal point
      int sensorValue = analogRead(A0); // Read the input on analog pin 0 (pin 36):
      float voltage = sensorValue * (3 / 1023.0); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
      float temperature = (voltage - 0.5)/(0.01*51); // Convert the voltage to temperature in degrees Celsius (51 is a scaling factor)
      
      temp += temperature; // Add temperature to temp -> We will use this to get the average temperature later....
      
      // APPEND MEASUREMENTS TO ARRAYS FOR DATA PROCESSING

      weight_readings[i] = weight; // Append weight reading to the relevant array
      temperature_readings[i] = temperature; // Append temperature reading to the relevant array
      time_readings[i] = i*0.5; // Append time reading to the relevant array

      // PRINT MEASUREMENTS OUT TO SERIAL MONITOR

      Serial.print("Scale reading:\t");
      Serial.print(weight);
      Serial.print("\t");
      Serial.print("Temperature reading: \t");
      Serial.println(temperature);
      
      delay(500); // 1/2 a second delay between each reading
    }

    int arrayLength = sizeof(weight_readings) / sizeof(float);
    Serial.print("weight_readings = [");
    for (int i = 0; i < arrayLength; i++) {
      Serial.print(weight_readings[i]);
      if (i < arrayLength - 1) {
        Serial.print(", ");
      }
    }
    Serial.println("]");

    Serial.print("time_readings = [");
    for (int i = 0; i < arrayLength; i++) {
      Serial.print(time_readings[i]);
      if (i < arrayLength - 1) {
        Serial.print(", ");
      }
    }
    Serial.println("]");

    Serial.print("temperature_readings = [");
    for (int i = 0; i < arrayLength; i++) {
      Serial.print(temperature_readings[i]);
      if (i < arrayLength - 1) {
        Serial.print(", ");
      }
    }
    Serial.println("]");

    process_data(); // CALL DATA PROCESSING FUNCTION!
    status = "SLEEPING";
  }
}

// ------------------------------------ DATA PROCESSING -----------------------------------------


// Natasha's block
void process_data(){

  // You don't need to worry about handling client requests to the server in this function since the server is largely disabled anyway

  Serial.println("processing block entered");
  time_t time = now(); // get the current timestamp from when time was started in the setup function() 
                       // This should be one of the first things done in this function
                       // since the time should be captured straight after the 50 measurements are taken.


  //  At the end of this function, once the weight has been extracted, 
  //  add the weight, timestamp (cf. Time.h library), & temperature to the FRONT of these arrays:
  //  
  //  float previous_temps[10];
  //  float previous_weights[10];
  //  char timestamps[10];

  float weight = 10.0*num_elements; // Dummy weight for testing


  // Get timestamp
  char timestamp[9];
  int h = hour(time);
  int m = minute(time);
  int s = second(time);
  sprintf(timestamp, "%02d:%02d:%02d", h, m, s);

  // Add the calculated temperature, weight, and timestamp to an array of collected values
  add_to_front(round(temp*1.0)/50.0, weight, timestamp);

  // Update the number of elements counter. This enables proper rendering on the web server
  if (num_elements < 10) {
    num_elements +=1;
  }

}


// ------------------------------------ HELPER FUNCTIONS ----------------------------------------


//  This function adds a temperature, a weight, and a time to the sized-ten arrays for webserver logging
void add_to_front(float temp, float weight, char *timestamp) {
  // Shift the existing elements in the arrays towards the end
  for (int i = 9; i > 0; i--) {
    previous_temps[i] = previous_temps[i-1];
    previous_weights[i] = previous_weights[i-1];
    free(timestamps[i]); // Free the memory pointed to by the shifted-out pointer
    timestamps[i] = timestamps[i-1];
  }
  // Add the new values to the front of the arrays
  previous_temps[0] = temp;
  previous_weights[0] = weight;
  timestamps[0] = (char*) malloc(sizeof(char) * 9); // Reserve memory
  strcpy(timestamps[0], timestamp); // Add element into reserved memory slot
}


// ------------------------------------ WEBSERVER HTML & FUNCTIONS ------------------------------


void handle_OnConnect() {
  Serial.println("on connect!"); 
  server.send(200, "text/html", SendMainHTML()); 
}

void get_results() {
  Serial.println("getting results!");
  server.send(200, "text/html", SendResultsHTML()); 
}

void handle_NotFound(){
  Serial.println("NOT FOUND!");
  server.send(404, "text/plain", "Error: Page Not found");
}

String SendMainHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>ESP32 Web Server</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {font-size: 14px; background-color: #3498db;}\n";
  ptr +=".button-on:active {font-size: 14px; background-color: #2980b9;}\n";
  ptr +="p {font-size: 12px; font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<h1>Home</h1>\n";
  ptr +="<body>\n";
  if (status == "PROCESSING"){
    ptr +="<h3>ESP-32 STATUS = PROCESSING MODE</h3>\n";
    ptr +="</body>\n";
    ptr +="</html>\n";
    return ptr;
  }
  ptr +="<h3>ESP-32 STATUS = SLEEP MODE</h3>\n";
  ptr +="<p>See Results</p><a class=\"button button-on\" href=\"/results\">RESULTS</a>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;  
}

String SendResultsHTML(){

  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Results</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";

  ptr +=".button-home {font-size: 14px;background-color: #34495e;}\n";
  ptr +=".button-home:active {font-size: 14px;background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Measurement Results</h1>\n";

  // DEFINE SOME CSS FOR TABLE STYLING
  ptr += "<style>";
  ptr += "table { border-collapse: separate; border-spacing: 0; border-radius: 10px; overflow: hidden; margin: 0 auto; }";
  ptr += "th, td { text-align: left; padding: 10px; border-top: 1px solid black; }";
  ptr += "th:first-child, td:first-child { border-left: 1px solid black; }";
  ptr += "th:last-child, td:last-child { border-right: 1px solid black; }";
  ptr += "tr:nth-child(even) { background-color: #f2f2f2; }";
  ptr += "thead th { background-color: #f2f2f2; }";
  ptr += "table:before, table:after { content: ''; display: table; }";
  ptr += "table:after { clear: both; }";
  ptr += "table:before, th:first-child, td:first-child { border-left: none; }";
  ptr += "table:before, th:last-child, td:last-child { border-right: none; }";
  ptr += "table:before, thead th:first-child { border-top-left-radius: 10px; }";
  ptr += "table:before, thead th:last-child { border-top-right-radius: 10px; }";
  ptr += "table:before, tbody td:first-child { border-bottom-left-radius: 10px; }";
  ptr += "table:before, tbody td:last-child { border-bottom-right-radius: 10px; }";
  ptr += "</style>";
  ptr += "<table>";
  ptr += "<thead><tr><th>ID</th><th>Weight (g)</th><th>Temp (C)</th><th>Timestamp</th></tr></thead>";

  // Add data from sized-10 arrays to results table
  for (int i = 0; i < num_elements; i++){
    String ID = String(i+1);
    String temperature = String(previous_temps[i]);
    String weight = String(previous_weights[i]);
    String timestamp = String(timestamps[i]);
    ptr += "<tbody><tr><td>"+ID+"</td><td>"+weight+"</td><td>"+temperature+"</td><td>"+timestamp+"</td></tr>";
  }

  ptr += "</table>";
  
  // GRAPH TESTING

  ptr += "<head><title>Graph Plotting Example</title><style>.graph {width: 500px;height: 300px;border: 1px solid black;margin: 20px auto;position: relative;}";
	ptr += ".graph .x-axis {position: absolute;bottom: 0;width: 100%;height: 1px;background-color: black;}";
	ptr += ".graph .y-axis {position: absolute;left: 0;width: 1px;height: 100%;background-color: black;}";
	ptr += ".graph .point {position: absolute;border-radius: 50%;background-color: red;width: 10px;height: 10px;z-index: 1;}";
	ptr += ".graph .label {position: absolute;font-size: 12px;z-index: 2;}";
	ptr += "</style></head><body><div class='graph'><div class='x-axis'></div><div class='y-axis'></div>";
	ptr += "<?php $previous_weights = array(50, 45, 42, 39, 36);$previous_times = array(1, 2, 3, 4, 5);";
  ptr += "$weights_max = max($previous_weights);$weights_min = min($previous_weights);$times_max = max($previous_times);$times_min = min($previous_times);";
  ptr += "for ($i = 0; $i < count($previous_weights); $i++) {$weight_pos = ($previous_weights[$i] - $weights_min) / ($weights_max - $weights_min) * 100;";
	ptr += "$weight_pos = ($previous_weights[$i] - $weights_min) / ($weights_max - $weights_min) * 100;";
	ptr += "$time_pos = ($previous_times[$i] - $times_min) / ($times_max - $times_min) * 100;";
  ptr += "echo '<div class='point' style='left:'.$time_pos.'%; bottom:'.$weight_pos.'%;'></div>';";
	ptr += "echo '<div class='label' style='left:'.$time_pos.'%; bottom:'.($weight_pos + 12).'%;'>'.$second_times[$i].'s, '.$previous_weights[$i].'kg</div>';}?></div>";



  ptr +="<p>Return to Home Page</p><a class=\"button button-home\" href=\"/\">HOME</a>\n"; // MAKE SURE HREF IS CORRECT HERE!!! 

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}