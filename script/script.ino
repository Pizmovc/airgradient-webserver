/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

For build instructions please visit https://www.airgradient.com/diy/

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT30/31 (Temperature/Humidity Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

The codes needs the following libraries installed:
"WifiManager by tzapu, tablatronix" tested with Version 2.0.3-alpha
"ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse, Fabrice Weinberg" tested with Version 4.1.0

Configuration:
Please set in the code below which sensor you are using and if you want to connect it to WiFi.

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/schools/

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

// set sensors that you do not use to false
boolean hasPM=true;
boolean hasCO2=true;
boolean hasSHT=true;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI=true;

void setup(){
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(),HEX),true);

  if (hasPM) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);

  if (connectWIFI) {
    connectToWifi();
    setupWebServer();
  }
  delay(2000);
}

String payload;
int wifi = -1;
int pm2 = -1;
int co2 = -1;
int temp = -1;
int hum = -1;

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
    temp = (int)(result.t * 10);
    hum = result.rh;
  }

  displayField("PM2", pm2);
  displayField("CO2", co2);
  displayField("TEM", temp);
  displayField("HUM", hum);
}

void displayField(String name, int value) {
  String displayValue = String(value);
  if (name == "TEM") {
    displayValue = String((float)value / 10);
  }
  showTextRectangle(name, displayValue, false);

  for(int i = 0; i < 15; i++) {
    delay(100);
    handleClient();
  }
}

void handleClient() {
  if (connectWIFI){
    server.handleClient();
  }
}

// DISPLAY
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

// Wifi Manager
void connectToWifi(){
  WiFiManager wifiManager;
  WiFiManagerParameter custom_hostname("hostname", "desired device hostname", "luft-ena", 40);
  wifiManager.addParameter(&custom_hostname);
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-"+String(ESP.getChipId(),HEX);
  wifiManager.setTimeout(120);
  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str(), "qwerty12345")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
  }
  wifiManager.setHostname(custom_hostname.getValue());
}

String getPayload() {
  String payload = "{";
  payload += getField("wifi", wifi) + ",";
  payload += getField("pm2", pm2) + ",";
  payload += getField("co2", co2) + ",";
  payload += getField("temperature", temp) + ",";
  payload += getField("humidity", hum) + "}";

  return payload;
}

String getField(String name, int value) {
  String castValue = String(value);
  if (name == "temperature") {
    castValue = String((float)value / 10);
  }

  return "\"" + name + "\":" + castValue;
}

void setupWebServer(){
  server.on("/api", []() {
    server.send(200, "application/json", getPayload());
  });
  server.on("/", []() {
    server.send(200, "text/html;charset=UTF-8", getHTML());
  });
  server.begin();
}

String getTableRow(String name, int value, String unit) {
  String displayValue = String(value);
  if (name == "Temperature") {
    displayValue = String((float)value / 10);
  }
  return "<tr><td>" + name + "</td><td>" + displayValue + "</td><td>" + unit + "</td></tr>";
}

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

