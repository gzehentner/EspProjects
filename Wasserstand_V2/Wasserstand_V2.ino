/*
=============================================
Wasserstand_V2

including following features:
- Wasserstand abfragen anhand von vier Relais Ausgängen
- Aktiven Bereich anzeigen
- Hintergrundfarbe abhängig von Warn- oder Alarmlevel
- Info auf WebSeite anzeigen
- Beim Wechsel auf einen gefährlichen Zustand und zurück werden Info Mails (Text-Mail) gesendet

------------------------------
V2.2
----
fixed issues: 
- actual date/time is actually not evaluated to avoid crash when sending email --> this has to be fixed next (unset debug_crash to reactivate)

new features implemented
- HTML Mail
  - Hyperlink auf die Hauptseite

new features comming soon:
- HTML Mail mit Farbe
- Testmail per klick

known issues: OTA download not possible "not enouth space"
-----------------------------------------------------------

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
/**
 * Send_Text-Example shows, how to send a text E-Mail
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2023 mobizt
 */

// for Send-Mail
#include <Arduino.h>
#include <ESP_Mail_Client.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>   // for the webserver
#include <ESP8266HTTPClient.h>  // for the webclient https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient
#include <ESP8266mDNS.h>        // Bonjour/multicast DNS, finds the device on network by name
#include <ArduinoOTA.h>         // OTA Upload via ArduinoIDE

#include <NTPClient.h>  // get time from timeserver
#include <WiFiUdp.h>

//#include <credentials.h>                                   // my credentials - remove before upload

#define VERSION "2.2"  // the version of this sketch

#define debug_disable_sendMail

// enable debugging of NTP time management
// #define DEBUG_TIME

#define BLUE_LED 2



/* *******************************************************************
         the board settings / die Einstellungen der verschiedenen Boards
 ********************************************************************/

#define TXT_BOARDID "164"                                 // an ID for the board
#define TXT_BOARDNAME "Wasserstand-Messung"               // the name of the board
#define CSS_MAINCOLOR "blue"                              // don't get confused by the different webservers and use different colors
const uint16_t clientIntervall = 0;                       // intervall to send data to a server in seconds. Set to 0 if you don't want to send data
const char* sendHttpTo = "http://192.168.178.153/d.php";  // the module will send information to that server/resource. Use an URI or an IP address

/* ============================================================= */
/* Definition for Send-Mail                                      */

/* settings for GMAIL */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT esp_mail_smtp_port_587  // port 465 is not available for Outlook.com

/* The log in credentials */
#define AUTHOR_NAME "Pegelstand Zehentner"
#define AUTHOR_EMAIL "georgzehentneresp@gmail.com"
#define AUTHOR_PASSWORD "lwecoyvlkmordnly"

/* Recipient email address */
#define RECIPIENT_EMAIL "gzehentner@web.de"

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

Session_Config config;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

#include "HeapStat.h"
HeapStat heapInfo;

// const char rootCACert[] PROGMEM = "-----BEGIN CERTIFICATE-----\n"
//                                   "-----END CERTIFICATE-----\n";
/* ============================================================= */


/* *******************************************************************
         other settings / weitere Einstellungen für den Anwender
 ********************************************************************/

#ifndef STASSID                 // either use an external .h file containing STASSID and STAPSK or ...
#define STASSID "Zehentner"     // ... modify these line to your SSID
#define STAPSK "ElisabethScho"  // ... and set your WIFI password
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

/* *******************************************************************
         Globals - Variables and constants
 ********************************************************************/

unsigned long ss = 0;                             // current second since startup
unsigned long ss_last = 0;                        // value of ss out of the last loop run
const uint16_t ajaxIntervall = 5;                 // intervall for AJAX or fetch API call of website in seconds
uint32_t clientPreviousSs = 0 - clientIntervall;  // last second when data was sent to server

/*=================================================================*/
/* Prepare WaterLevel Application */

/* -- Pin-Def -- */
#define GPin_AHH 3
#define GPin_AH 5
#define GPin_AL 4
#define GPin_ALL 14
#define GPout_GND 12

#define Ain_Level 2

int val_AHH;
int val_AH;
int val_AL;
int val_ALL;

int inByte = 0;
int incomingByte = 0;  // for incoming serial data

/* -- Alarm-Level -- */
#define Level_AHH 190  // Oberkante Schacht = 197cm
#define Level_AH 185   // Zwischenstand
#define Level_AL 180
#define Level_ALL 145  // Unterkante KG Rohr \
                       // Aktueller Niedrig-Stand Nov 2023 = 105cm

String alarmStateTxt[5] = { "Lowest", "Low", "Medium", "Warning", "Alarm" };
int alarmState;     // shows the actual water level
int alarmStateOld;  // previous value of alarmState
bool executeSendMail = false;

/*=================================================================*/
/* Variables to connect to timeserver   */
/* Define NTP Client to get time */

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

String currentDate;    // hold the current date
String formattedTime;  // hold the current time

//Week Days
String weekDays[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

//Month names
String months[12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

/* End Timeserver */

/*=================================================================*/

ESP8266WebServer server(80);  // an instance for the webserver

#ifndef CSS_MAINCOLOR
#define CSS_MAINCOLOR "#8A0829"  // fallback if no CSS_MAINCOLOR was declared for the board
#endif


/* *******************************************************************
         S E T U P
 ********************************************************************/

void setup(void) {

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(BLUE_LED, OUTPUT);

  /*=================================================================*/
  /* setup serial  and connect to WLAN */
  Serial.begin(9600);
  Serial.println(F("\n" TXT_BOARDNAME "\nVersion: " VERSION " Board " TXT_BOARDID " "));
  Serial.print(__DATE__);
  Serial.print(F(" "));
  Serial.println(__TIME__);

  // Connect to WIFI
  char myhostname[8] = { "esp" };
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

  /*=================================================================*/
  /* Prepare SendMail */

  MailClient.networkReconnect(true);
  smtp.debug(1);

  smtp.callback(smtpCallback);

  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;

  config.login.user_domain = F("127.0.0.1");

  /*
  Set the NTP config time
  For times east of the Prime Meridian use 0-12
  For times west of the Prime Meridian add 12 to the offset.
  Ex. American/Denver GMT would be -6. 6 + 12 = 18
  See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
  */
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 1;
  config.time.day_light_offset = 0;

  /*=================================================================*/
  /* Prepare WaterLevel Application */

  // prepare relais input / output

  pinMode(GPin_AHH, INPUT_PULLUP);
  pinMode(GPin_AH, INPUT_PULLUP);
  pinMode(GPin_AL, INPUT_PULLUP);
  pinMode(GPin_ALL, INPUT_PULLUP);
  pinMode(GPout_GND, OUTPUT);

  digitalWrite(GPout_GND, 0);

  /* ----End Setup WaterLevel ------------------------------------------ */
  /*=================================================================*/


  /*=================================================================*/
  /* Setup WebServer and start*/

 //define the pages and other content for the webserver
 server.on("/", handlePage);       // send root page
 server.on("/0.htm", handlePage);  // a request can reuse another handler
 // server.on("/1.htm", handlePage1);
 // server.on("/2.htm", handlePage2);
 // server.on("/x.htm", handleOtherPage);          // just another page to explain my usage of HTML pages ...

 server.on("/f.css", handleCss);  // a stylesheet
 server.on("/j.js", handleJs);    // javscript based on fetch API to update the page
 //server.on("/j.js",  handleAjax);             // a javascript to handle AJAX/JSON update of the page  https://werner.rothschopf.net/201809_arduino_esp8266_server_client_2_ajax.htm
 server.on("/json", handleJson);     // send data in JSON format
                                     //  server.on("/c.php", handleCommand);            // process commands
                                     //  server.on("/favicon.ico", handle204);          // process commands
 server.onNotFound(handleNotFound);  // show a typical HTTP Error 404 page

 //the next two handlers are necessary to receive and show data from another module
 // server.on("/d.php", handleData);               // receives data from another module
 // server.on("/r.htm", handlePageR);              // show data as received from the remote module

  server.begin();  // start the webserver
  Serial.println(F("HTTP server started"));

  /*=================================================================*/
  /* IDE OTA */
  ArduinoOTA.setHostname(myhostname);  // give a name to your ESP for the Arduino IDE
  ArduinoOTA.begin();                  // OTA Upload via ArduinoIDE https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html

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
  if (clientIntervall > 0 && (ss - clientPreviousSs) >= clientIntervall) {
    //   sendPost();
    clientPreviousSs = ss;
  }
  server.handleClient();

  /*=================================================================*/
  /* Over the Air UPdate */
  ArduinoOTA.handle();       // OTA Upload via ArduinoIDE

  /*=================================================================*/
  /* evaluate water level */
  /*=================================================================*/
  // Read in relais status
  val_AHH = digitalRead(GPin_AHH);
  val_AH  = digitalRead(GPin_AH);
  val_AL  = digitalRead(GPin_AL);
  val_ALL = digitalRead(GPin_ALL);

  // set alarmSte
  alarmStateOld = alarmState;

  if ((val_AHH == 0) && (val_AH == 0)) {
    alarmState = 5;
  } else if (val_AH == 0) {
    alarmState = 4;
  } else if (val_AL == 1) {
    alarmState = 3;
  } else if ((val_AL == 0) && (val_ALL == 1)) {
    alarmState = 2;
  } else if ((val_ALL == 0)) {
    alarmState = 1;
  }

  // send mail depending on alarmState
  String subject;
  String textMsg;
  String htmlMsg;

   
  if (alarmStateOld > 0) {           // alarmStateOld == 0 means, it is the first run / dont send mail at the first run
    if (alarmStateOld < alarmState)
    { // water level is increasing
      if (alarmState == 4)
      {
        // send warning mail
        Serial.println(F("warning mail should be sent"));
        subject =  F("Pegel Zehentner -- Warnung ");
        htmlMsg =  F("<p>Wasserstand Zehentner ist in den Warnbereich gestiegen <br>");
        htmlMsg += F("Pegelstand über die Web-Seite: </p>; // <a href='http://zehentner.dynv6.net:400'>Wasserstand-Messung</a> beobachten </p>");
        executeSendMail = true;
      }
      else if (alarmState == 5)
      {
        // send alarm mail
        Serial.println("alarm mail should be sent");
        subject =  F("Pegel Zehentner -- Alarm ");
        htmlMsg =  F("<p>Wasserstand Zehentner ist jetzt im Alarmbareich<br>");
        htmlMsg += F("es muss umgehend eine Pumpe in Betrieb genommen werden. <br>");
        htmlMsg += F("Pegelstand über die Web-Seite: <a href='http://zehentner.dynv6.net:400'>Wasserstand-Messung</a> beobachten </p>");
        executeSendMail = true;
      }
    }
    else if (alarmStateOld > alarmState)
    { // water level is decreasing
      if (alarmState == 4)
      {
        // info that level comes from alarm and goes to warning
        Serial.println(F("level decreasing, now warning"));
        subject =  F("Pegel Zehentner -- Warnung ");
        htmlMsg =  F("<p>Wasserstand Zehentner ist wieder zurück in den Warnbereich gesunken<br>");
        htmlMsg += F("Pegelstand über die Web-Seite: <a href='http://zehentner.dynv6.net:400'>Wasserstand-Messung</a> beobachten </p>");
        executeSendMail = true;
      }
      else if (alarmState == 3)
      {
        // info that level is now ok
        Serial.println(F("level decreased to OK"));
        subject = F("Pegel Zehentner -- OK ");
        htmlMsg = F("<p>Wasserstand Zehentner ist wieder im Normalbereich</p>");
        executeSendMail = true;
      }
    }
    else if (alarmStateOld == alarmState)
    {
      // do nothing
      executeSendMail = false;
    }
  }

  /*=================================================================*/
  /*  code for getting time from NTP       */
    timeClient.update();
  
  
    time_t epochTime = timeClient.getEpochTime();
  
    formattedTime = timeClient.getFormattedTime();
  
    //Get a time structure
    struct tm* ptm = gmtime((time_t*)&epochTime);
  
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon + 1;
    int currentYear = ptm->tm_year + 1900;
  
    //Print complete date:
    currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  

  #ifdef DEBUG_TIME
    Serial.print("Formatted Time: ");
    Serial.println(formattedTime);
  
    Serial.print("Month day: ");
    Serial.println(monthDay);
  
    Serial.print("Month: ");
    Serial.println(currentMonth);
  
    Serial.print("Year: ");
    Serial.println(currentYear);
  
    Serial.print("Epoch Time: ");
    Serial.println(epochTime);
  
    Serial.print("Current date: ");
    Serial.println(currentDate);
  
    Serial.println("");
  
    delay(1000);
  
  #endif
  /* End getting time and date */

  // set a delay to avoid ESP is busy all the time
  delay(1);

  /*=================================================================*/
  /* Send Email reusing session   */
  /*=================================================================*/

    if (executeSendMail)
    {
        executeSendMail = false;

        SMTP_Message message;

        message.sender.name = F("Pegel Zehentner");
        message.sender.email = AUTHOR_EMAIL;
        message.subject = subject;

        message.addRecipient(F("Schorsch"), RECIPIENT_EMAIL);

        // htmlMsg already set by Waterlevel
        message.html.content = htmlMsg;
        message.text.content = F("");


        Serial.println();
        Serial.println(F("Sending Email..."));

        if (!smtp.isLoggedIn())
        {
            /* Set the TCP response read timeout in seconds */
            // smtp.setTCPTimeout(10);

            if (!smtp.connect(&config))
            {
                MailClient.printf("Connection error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
                goto exit;
            }

            if (!smtp.isLoggedIn())
            {
                Serial.println(F("Error, Not yet logged in."));
                goto exit;
            }
            else
            {
                if (smtp.isAuthenticated())
                    Serial.println(F("Successfully logged in."));
                else
                    Serial.println(F("Connected with no Auth."));
            }
        }

        if (!MailClient.sendMail(&smtp, &message, false))
            MailClient.printf("Error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());


    exit:

        heapInfo.collect();
        heapInfo.print();


  /*=END Send_Reuse_Session =====================================*/

    }

}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{

    Serial.println(status.info());

    if (status.success())
    {

        Serial.println   (F("----------------"));
        MailClient.printf(  "Message sent success: %d\n", status.completedCount());
        MailClient.printf(  "Message sent failed: %d\n", status.failedCount());
        Serial.println   (F("----------------\n"));

        for (size_t i = 0; i < smtp.sendingResult.size(); i++)
        {
            SMTP_Result result = smtp.sendingResult.getItem(i);

            MailClient.printf("Message No: %d\n", i + 1);
            MailClient.printf("Status: %s\n", result.completed ? "success" : "failed");
            MailClient.printf("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
            MailClient.printf("Recipient: %s\n", result.recipients.c_str());
            MailClient.printf("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");

        smtp.sendingResult.clear();
    }
}