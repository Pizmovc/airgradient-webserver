/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

The codes needs the following libraries installed:
"AirGradient Air Quality Sensor by AirGradient", tested with version 1.3.5
"WifiManager by tzapu, tablatronix", tested with version 2.0.4-beta
"ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse, Fabrice Weinberg", tested with version 4.2.1

Configuration:
Below are some boolean values that you can set to true/false

MIT License
*/

#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

#include <Wire.h>
#include "SSD1306Wire.h"

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

ESP8266WebServer server(80);

// Set sensors that you do not use to false
boolean hasPM=true;
boolean hasCO2=true;
boolean hasSHT=true;

// Set to true if you want to connect to wifi
boolean connectWIFI=true;

void setup(){
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(),HEX), true);

  if (hasPM) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);

  if (connectWIFI) {
    connectToWifi();
    setupWebServer();
  }
  delay(2000);
}

// These hold values
int wifi = -1;
int pm2 = -1;
int co2 = -1;
int temp = -1;
int hum = -1;

// Query sensors, display the data on the LCD, handle any web clients
void loop(){
  wifi = WiFi.RSSI();

  if (hasPM) {
    pm2 = ag.getPM2_Raw();
  }

  if (hasCO2) {
    co2 = ag.getCO2_Raw();
  }

  if (hasSHT) {
    TMP_RH result = ag.periodicFetchData();
    // Its nicer working with one value type, so convert to int
    // ... but we don't want to loose the precision, so we multiply by 10
    temp = (int)(result.t * 10);
    hum = result.rh;
  }

  displayField("PM2", pm2);
  waitAndHandleClients();

  displayField("CO2", co2);
  waitAndHandleClients();

  displayField("TEM", temp);
  waitAndHandleClients();

  displayField("HUM", hum);
  waitAndHandleClients();
}

// Displays a two line screen with `name` and `value`
void displayField(String name, int value) {
  String displayValue = String(value);
  if (name == "TEM") {
    displayValue = String((float)value / 10);
  }
  showTextRectangle(name, displayValue, false);
}

// Waits 100ms, handles any clients, waits again, handles any clients, ...
// This way we get more responsive web server
void waitAndHandleClients() {
  for(int i = 0; i < 12; i++) {
    delay(100);
    if (connectWIFI){
      server.handleClient();
    }
  }
}


// Initialize the WiFi
void connectToWifi(){
  WiFiManager wifiManager;
  //WiFi.disconnect(); // to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-"+String(ESP.getChipId(),HEX);
  wifiManager.setTimeout(120);
  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str(), "qwerty12345")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
  }
}

// Creates endpoints for the webserver and starts it up
void setupWebServer(){
  // Query this endpoint to get JSON with data
  server.on("/api", []() {
    server.send(200, "application/json", getJSONData());
  });

  // Visit this endpoint in the browser to view data in a table
  server.on("/", []() {
    server.send(200, "text/html;charset=UTF-8", getHTML());
  });

  // Prometheus spec metrics API
  server.on("/metrics", []() {
    server.send(200, "text/plain;charset=UTF-8", getMetrics());
  });

  server.begin();
}

// Creates a JSON string containing all the values
String getJSONData() {
  String payload = "{";
  payload += getField("wifi", wifi) + ",";
  payload += getField("pm2", pm2) + ",";
  payload += getField("co2", co2) + ",";
  payload += getField("temperature", temp) + ",";
  payload += getField("humidity", hum) + "}";

  return payload;
}

// Returns one line of JSON: `"name": value`
String getField(String name, int value) {
  String castValue = String(value);
  if (name == "temperature") {
    castValue = String((float)value / 10);
  }

  return "\"" + name + "\":" + castValue;
}

// Returns a HTML page with a data table
String getHTML() {
  String html = "<!DOCTYPE html>";
  html += "<head> <style> body { font-size: 1.5rem; } table { font-family: arial, sans-serif; border-collapse: collapse; width: 100%; } td, th { border: 1px solid #dddddd; text-align: left; padding: 8px; } tr:nth-child(even) { background-color: #dddddd; } </style> </head> ";
  html += "<body><table>";
  html += getTableRow("Particulate matter 2.5", pm2, "μg/m3");
  html += getTableRow("CO2", co2, " ppm");
  html += getTableRow("Temperature", temp, "°C");
  html += getTableRow("Humidity", hum, " %");
  html += "</table></body></html>";

  return html;
}

// Returns HTML for one table row
String getTableRow(String name, int value, String unit) {
  String displayValue = String(value);
  if (name == "Temperature") {
    displayValue = String((float)value / 10);
  }
  return "<tr><td>" + name + "</td><td>" + displayValue + "</td><td>" + unit + "</td></tr>";
}

// Creates Prometheus spec metrics page
String getMetrics() {
  String response = "";
  response += "\n# HELP wifi Current WiFi signal strength, in dB";
  response += "\n# TYPE wifi gauge";
  response += "\nwifi " + String(wifi);
  response += "\n";
  response += "\n# HELP pm2 Particulate Matter PM2.5 value, in μg/m3";
  response += "\n# TYPE pm2 gauge";
  response += "\npm2 " + String(pm2);
  response += "\n";
  response += "\n# HELP co2 CO2 value, in ppm";
  response += "\n# TYPE co2 gauge";
  response += "\nco2 " + String(co2);
  response += "\n";
  response += "\n# HELP temp Temperature, in degrees Celsius";
  response += "\n# TYPE temp gauge";
  response += "\ntemp " + String((float)temp / 10);
  response += "\n";
  response += "\n# HELP hum Relative humidity, in percent";
  response += "\n# TYPE hum gauge";
  response += "\nhum " + String(hum);

  return response;
}

// Helper function to show some text on the screen
void showTextRectangle(String ln1, String ln2, boolean small) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (small) {
    display.setFont(ArialMT_Plain_16);
  } else {
    display.setFont(ArialMT_Plain_24);
  }
  display.drawString(32, 16, ln1);
  display.drawString(32, 36, ln2);
  display.display();
}

