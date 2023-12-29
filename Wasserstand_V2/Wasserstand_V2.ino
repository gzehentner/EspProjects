/*
=============================================
Wasserstand_V2

- Wasserstand abfragen anhand von vier Relais Ausgängen
- Aktiven Bereich anzeigen
- Hintergrundfarbe abhängig von Warn- oder Alarmlevel
- Info auf WebSeite anzeigen
- Info auf AskSensors weiterleiten (hier soll eine Info-Mail generiert werden bei Überschreiben des Alarmlevel)
- Evtl. als neues Feature: Info-Mail

Code Basiert auf dem ServerClientTutorial (beschrieben gleich hier darunter)

*/
/* *******************************************************************
   ESP8266 Server and Client
   by noiasca:

   Hardware
   - NodeMCU / ESP8266

   Short
   - simple webserver with some pages
   - Stylesheet (css) optimized for mobile devices
   - webseite update with fetch API
   - control pins via webpage
   - a webclient can send data to another webserver
   - ArduinoOTA upload with Arduino IDE
   - see the full tutorial on: https://werner.rothschopf.net/201809_arduino_esp8266_server_client_0.htm

   Open Tasks
   - open serial monitor and send an g to test client and

   Version
   2021-07-21 (compiles with ESP8266 core 2.7.4 without warnings)
***************************************************************** */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>                              // for the webserver
#include <ESP8266HTTPClient.h>                             // for the webclient https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient
#include <ESP8266mDNS.h>                                   // Bonjour/multicast DNS, finds the device on network by name
#include <ArduinoOTA.h>                                    // OTA Upload via ArduinoIDE

//#include <credentials.h>                                   // my credentials - remove before upload

#define VERSION "2.0"                                    // the version of this sketch
                                                           
/* *******************************************************************
         the board settings / die Einstellungen der verschiedenen Boards
 ********************************************************************/

#define TXT_BOARDID "164"                                  // an ID for the board
#define TXT_BOARDNAME "Wasserstand-Messung"                // the name of the board
#define CSS_MAINCOLOR "blue"                               // don't get confused by the different webservers and use different colors
const uint16_t clientIntervall = 0;                        // intervall to send data to a server in seconds. Set to 0 if you don't want to send data
const char* sendHttpTo = "http://192.168.178.153/d.php";     // the module will send information to that server/resource. Use an URI or an IP address



/* *******************************************************************
         other settings / weitere Einstellungen für den Anwender
 ********************************************************************/

#ifndef STASSID                        // either use an external .h file containing STASSID and STAPSK or ...
#define STASSID "Zehentner"            // ... modify these line to your SSID
#define STAPSK  "ElisabethScho"        // ... and set your WIFI password
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

/* *******************************************************************
         Globals - Variables and constants
 ********************************************************************/

unsigned long ss = 0;                            // current second since startup
const uint16_t ajaxIntervall = 5;                // intervall for AJAX or fetch API call of website in seconds
uint32_t clientPreviousSs = 0 - clientIntervall; // last second when data was sent to server

/* ----Prepare WaterLevel ------------------------------------------ */
// #define Sim_Relais

/* -- Pin-Def -- */
#define GPin_AHH 3
#define GPin_AH  5
#define GPin_AL  4
#define GPin_ALL 14
#define GPout_GND 12

#define Ain_Level 2

int val_AHH;
int val_AH ;
int val_AL ;
int val_ALL;

int inByte = 0;
int incomingByte = 0; // for incoming serial data

/* -- Alarm-Level -- */
#define Level_AHH 190     // Oberkante Schacht = 197cm
#define Level_AH  185     // Zwischenstand
#define Level_AL  180     
#define Level_ALL 145     // Unterkante KG Rohr
                         // Aktueller Niedrig-Stand Nov 2023 = 105cm

ESP8266WebServer server(80);                     // an instance for the webserver

#ifndef CSS_MAINCOLOR
#define CSS_MAINCOLOR "#8A0829"                  // fallback if no CSS_MAINCOLOR was declared for the board
#endif


/* *******************************************************************
         S E T U P
 ********************************************************************/

void setup(void) {

  Serial.begin(9600);
  Serial.println(F("\n" TXT_BOARDNAME "\nVersion: " VERSION " Board " TXT_BOARDID " "));
  Serial.print  (__DATE__);
  Serial.print  (F(" "));
  Serial.println(__TIME__);

  char myhostname[8] = {"esp"};
  strcat(myhostname, TXT_BOARDID);
  WiFi.hostname(myhostname);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F(""));
  Serial.print(F("Connected to "));
  Serial.println(ssid);
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  if (MDNS.begin("esp8266")) {
    Serial.println(F("MDNS responder started"));
  }

  /* ----Setup WaterLevel ------------------------------------------ */
  // prepare relais input / output
  
  pinMode(GPin_AHH , INPUT_PULLUP);
  pinMode(GPin_AH  , INPUT_PULLUP);
  pinMode(GPin_AL  , INPUT_PULLUP);
  pinMode(GPin_ALL , INPUT_PULLUP);
  pinMode(GPout_GND, OUTPUT);
  
  digitalWrite(GPout_GND, 0);

  /* ----End Setup WaterLevel ------------------------------------------ */


  //define the pages and other content for the webserver
  server.on("/",      handlePage);               // send root page
  server.on("/0.htm", handlePage);               // a request can reuse another handler
  // server.on("/1.htm", handlePage1);               
  // server.on("/2.htm", handlePage2);               
  // server.on("/x.htm", handleOtherPage);          // just another page to explain my usage of HTML pages ...
  
  server.on("/f.css", handleCss);                // a stylesheet                                             
  server.on("/j.js",  handleJs);                 // javscript based on fetch API to update the page
  //server.on("/j.js",  handleAjax);             // a javascript to handle AJAX/JSON update of the page  https://werner.rothschopf.net/201809_arduino_esp8266_server_client_2_ajax.htm                                                
  server.on("/json",  handleJson);               // send data in JSON format
//  server.on("/c.php", handleCommand);            // process commands
//  server.on("/favicon.ico", handle204);          // process commands
  server.onNotFound(handleNotFound);             // show a typical HTTP Error 404 page

  //the next two handlers are necessary to receive and show data from another module
  // server.on("/d.php", handleData);               // receives data from another module
  // server.on("/r.htm", handlePageR);              // show data as received from the remote module

  server.begin();                                // start the webserver
  Serial.println(F("HTTP server started"));

  //IDE OTA
  ArduinoOTA.setHostname(myhostname);            // give a name to your ESP for the Arduino IDE
  ArduinoOTA.begin();                            // OTA Upload via ArduinoIDE https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html
}

/* *******************************************************************
         M A I N L O O P
 ********************************************************************/

void loop(void) {
  ss = millis() / 1000;
  if (clientIntervall > 0 && (ss - clientPreviousSs) >= clientIntervall)
  {
 //   sendPost();
    clientPreviousSs = ss;
  }
  server.handleClient();
  ArduinoOTA.handle();       // OTA Upload via ArduinoIDE

  val_AHH = digitalRead(GPin_AHH);
  val_AH  = digitalRead(GPin_AH);
  val_AL  = digitalRead(GPin_AL);
  val_ALL = digitalRead(GPin_ALL);

  // Serial.println(val_AHH);
  // Serial.println(val_AH );
  // Serial.println(val_AL );
  // Serial.println(val_ALL);

  //delay(1000);

}
