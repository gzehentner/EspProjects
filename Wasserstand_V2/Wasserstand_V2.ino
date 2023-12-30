/*
=============================================
Wasserstand_V2

including following features:
- Wasserstand abfragen anhand von vier Relais Ausgängen
- Aktiven Bereich anzeigen
- Hintergrundfarbe abhängig von Warn- oder Alarmlevel
- Info auf WebSeite anzeigen

known issues: 
- actual date/time is not refreshed automatically

new features:
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
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/


#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>                              // for the webserver
#include <ESP8266HTTPClient.h>                             // for the webclient https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient
#include <ESP8266mDNS.h>                                   // Bonjour/multicast DNS, finds the device on network by name
#include <ArduinoOTA.h>                                    // OTA Upload via ArduinoIDE

#include <NTPClient.h>                                      // get time from timeserver
#include <WiFiUdp.h>

//#include <credentials.h>                                   // my credentials - remove before upload

#define VERSION "2.1"                                    // the version of this sketch
                                                           
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

/*=================================================================*/
/* Prepare WaterLevel Application */

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

/*=================================================================*/
/* Variables to connect to timeserver   */
/* Define NTP Client to get time */

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

String currentDate;   // hold the current date
String formattedTime; // hold the current time

//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

/* End Timeserver */

/*=================================================================*/

ESP8266WebServer server(80);                     // an instance for the webserver

#ifndef CSS_MAINCOLOR
#define CSS_MAINCOLOR "#8A0829"                  // fallback if no CSS_MAINCOLOR was declared for the board
#endif


/* *******************************************************************
         S E T U P
 ********************************************************************/

void setup(void) {

/*=================================================================*/
/* setup serial  and connect to WLAN */
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

  /* Prepare WaterLevel Application */
  
  // prepare relais input / output
  
  pinMode(GPin_AHH , INPUT_PULLUP);
  pinMode(GPin_AH  , INPUT_PULLUP);
  pinMode(GPin_AL  , INPUT_PULLUP);
  pinMode(GPin_ALL , INPUT_PULLUP);
  pinMode(GPout_GND, OUTPUT);
  
  digitalWrite(GPout_GND, 0);

  /* ----End Setup WaterLevel ------------------------------------------ */
/*=================================================================*/


/*=================================================================*/
/* Setup WebServer and start*/

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

  /*=================================================================*/
  /* IDE OTA */
  ArduinoOTA.setHostname(myhostname);            // give a name to your ESP for the Arduino IDE
  ArduinoOTA.begin();                            // OTA Upload via ArduinoIDE https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html

  /*=================================================================*/
  /* Initialize a NTPClient to get time */

  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);

}

/* *******************************************************************
         M A I N L O O P
 ********************************************************************/

void loop(void) {

  /*=================================================================*/
  /* WebClient (not used yet)*/

  ss = millis() / 1000;
  if (clientIntervall > 0 && (ss - clientPreviousSs) >= clientIntervall)
  {
 //   sendPost();
    clientPreviousSs = ss;
  }
  server.handleClient();

  /*=================================================================*/
  /* Over the Air UPdate */
  ArduinoOTA.handle();       // OTA Upload via ArduinoIDE

  /*=================================================================*/
  /* Read in relais status */
  val_AHH = digitalRead(GPin_AHH);
  val_AH  = digitalRead(GPin_AH);
  val_AL  = digitalRead(GPin_AL);
  val_ALL = digitalRead(GPin_ALL);

  
  /*=================================================================*/
  /*  code for getting time from NTP       */
  timeClient.update();

  time_t epochTime = timeClient.getEpochTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  
  formattedTime = timeClient.getFormattedTime();
  Serial.print("Formatted Time: ");
  Serial.println(formattedTime);  

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int monthDay = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(monthDay);

  int currentMonth = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(currentMonth);

  int currentYear = ptm->tm_year+1900;
  Serial.print("Year: ");
  Serial.println(currentYear);

  //Print complete date:
  currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  Serial.print("Current date: ");
  Serial.println(currentDate);

  Serial.println("");
/* End getting time and date */

}
