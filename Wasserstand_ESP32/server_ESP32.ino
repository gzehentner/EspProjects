/* *******************************************************************
   Webserver

   How it all works together:

   page   0.htm         includes javascript j.js
   script j.js          the javascript requests the JSON
          json          returns data as JSON
   css    f.css         css for all on flash (program) memory
   php                  not really php - only a resource doing actions and not returning content (just http header 204 ok)
   ***************************************************************** */

/*
void handleOtherPage()
// a very simple example how to output a HTML page from program memory
{
  String message;
  message =  F("<!DOCTYPE html>\n"
               "<html lang='en'>\n"
               "<head>\n"
               "<title>" TXT_BOARDNAME " - Board " TXT_BOARDID "</title>\n"
               "</head>\n"
               "<body>\n"
               "<h1>Your webserver on " TXT_BOARDNAME " - Board " TXT_BOARDID " is running!</h1>\n"
               "<p>This is an example how you can create a webpage.</p>\n"
               "<p>But as most of my webserver should have the same look and feel I'm using one layout for all html pages.</p>\n"
               "<p>Therefore my html pages come from the functions starting with handlePage and just share a common top and bottom.</p>\n"
               "<p>To go back to the formated pages <a href='0.htm'>use this link</a></p>\n"
               "</body>\n"
               "</html>");
  server.send(200, "text/html", message);
}
*/

/* =======================================*/
void handleNotFound() {
/* =======================================*/
  // Output a "404 not found" page. It includes the parameters which comes handy for test purposes.
  Serial.println(F("D015 handleNotFound()"));
  String message;
  message += F("404 - File Not Found\n"
               "URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


/* =======================================*/
void handle204()
/* =======================================*/
{  
  server.send(204);                // this page doesn't send back content
}


/* =======================================*/
/* add a header  to each page including refs for all other pages */
void addTop(String &message)
/* =======================================*/
{
  message =  F("<!DOCTYPE html>\n"
               "<html lang='en'>\n"
               "<head>\n"
               "<title>" TXT_BOARDNAME " - Board " TXT_BOARDID "</title>\n"
               "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\n"
               "<meta name=\"viewport\" content=\"width=device-width\">\n"
               "<link rel='stylesheet' type='text/css' href='/f.css'>\n"
               "<script src='j.js'></script>\n"
               "</head>\n");
  message += F("<body>\n");
  message += F("<header>\n<h1>" TXT_BOARDNAME " - Board " TXT_BOARDID "</h1>\n");
  message += F("<nav> <a href=\"/\">[Home]</a> <a href=\"2.htm\">[The&nbsp;Webclient]</a> </p></nav>\n</header>\n"
               "<main>\n");
}


/* =======================================*/
/* The footer will display the uptime, the IP-address the version of the sketch and the compile date/time */
/* add code to calculate the uptime */
void addBottom(String &message) {
/* =======================================*/

  message += F("</main>\n"
               "<footer>\n<p>");           
  message += F("<p>Actual Date and Time: "); 

  message += currentDate;
  message += F(" -- " ); 
  message += formattedTime; 
  message += F("<br>" ); 

  if (ss > 604800)
  {
    message += F("<span id='week'>");
    message +=  ((ss / 604800) % 52);
    message += F("</span> weeks ");
  }
  if (ss > 86400)
  {
    message += F("<span id='day'>");
    message += ((ss / 86400) % 7);
    message += F("</span> days ");
  }
  if (ss > 3600)
  {
    message += F("<span id='hour'>");
    message += ((ss / 3600) % 24);
    message += F("</span> hours ");
  }

  message += F("<span id='min'>");
  message += ((ss / 60) % 60);
  message += F("</span> minutes ");

  message += F("<span id='sec'>");
  message += (ss % 60);
  message += F("</span> seconds since startup | Version " VERSION " | IP: ");
  message += WiFi.localIP().toString();
  message += F(" | " __DATE__ " " __TIME__ "</p>\n</footer>\n</body>\n</html>");
  server.send(200, "text/html", message);
}


// the html output
// finally check your output if it is valid html: https://validator.w3.org
// *** HOME ***  0.htm
/* =======================================*/
/* main page of this application:
 *   - display water level as raw data
 *   - diaplay actual warn or alarm satus
 */
void handlePage()
/* =======================================*/
{
  String message;
  addTop(message);

  message += F("<article>"
               "<h2>Wasserstand Zehentner Teisendorf</h2>"                                                   // here you write your html code for your homepage. Let's give some examples...
               "<p>Hier kann man den aktuellen Wasserstand in der Regenwasser-Zisterne  "
               "von Georg Zehentner, Streiblweg 19, Teisendorf ablesen.<br> "
               " Bei Überschreiten des Höchststand muss eine Pumpe aktiviert werden</p>\n"
               "</article>\n");

  message += F("<article>\n"
               "<h2>Rohdaten</h2>\n");
  
  // input signals are low active
  message += F("Level Alarm   high: <span id='val_AHH'>");  if (val_AHH==0 ) {message += F("active");} else {message += F("--");} message += F("</span><br>");
  message += F("Level Warning high: <span id='val_AH' >");  if (val_AH ==0 ) {message += F("active");} else {message += F("--");} message += F("</span><br>");
  message += F("Level Warning low:  <span id='val_AL' >");  if (val_AL ==0 ) {message += F("active");} else {message += F("--");} message += F("</span><br>");
  message += F("Level Alarm   low:  <span id='val_ALL'>");  if (val_ALL==0 ) {message += F("active");} else {message += F("--");} message += F("</span><br>");
  message += F("</article>\n");

  message += F("<article>\n"
               "<h2>Auswertung Wasserstand</h2>\n");
  if (alarmState == 5) {
    message += F("<message_err> Achtung Hochwasser -- Pumpe einschalten <br>Wasserstand > ");message += Level_AHH;message += F("<br></message_err>");
  } else if  (alarmState == 4) {
    message += F("<message_warn> Wasserstand ist zwischen "); message += Level_AH;  message += F(" und ");  message += Level_AHH; message += F( "<br></message_warn>");
  } else if  (alarmState == 3) {
    message += F("<message_ok> Wasserstand ist zwischen "); message += Level_AL;  message += F(" und ");  message += Level_AH;  message += F( "<br></message_ok>");
  } else if  (alarmState == 2) {
    message += F("<message_ok> Wasserstand ist zwischen "); message += Level_ALL; message += F(" und ");  message += Level_AL;  message += F( "<br></message_ok>");
  } else if  (alarmState == 1) {
    message += F("<message_ok>   Wasserstand < ");message += Level_ALL;message += F("<br></message_ok>");
  }
  message += F("</article>\n");

  addBottom(message);
  server.send(200, "text/html", message);
}



/* =======================================*/
void handleCss()
/* =======================================*/
{
  // output of stylesheet
  // this is a straight forward example how to generat a static page from program memory
  String message;
  message = F("*{font-family:sans-serif}\n"
              "body{margin:10px}\n"
              "h1, h2{color:white;background:" CSS_MAINCOLOR ";text-align:center}\n"
              "h1{font-size:1.2em;margin:1px;padding:5px}\n"
              "h2{font-size:1.0em}\n"
              "h3{font-size:0.9em}\n"
              "a{text-decoration:none;color:dimgray;text-align:center}\n"
              "main{text-align:center}\n"
              "article{vertical-align:top;display:inline-block;margin:0.2em;padding:0.1em;border-style:solid;border-color:#C0C0C0;background-color:#E5E5E5;width:20em;text-align:left}\n" // if you don't like the floating effect (vor portrait mode on smartphones!) - remove display:inline-block
              "article h2{margin-top:0;padding-bottom:1px}\n"
              "section {margin-bottom:0.2em;clear:both;}\n"
              "table{border-collapse:separate;border-spacing:0 0.2em}\n"
              "th, td{background-color:#C0C0C0}\n"
              "button{margin-top:0.3em}\n"
              "footer p{font-size:0.8em;color:dimgray;background:silver;text-align:center;margin-bottom:5px}\n"
              "nav{background-color:silver;margin:1px;padding:5px;font-size:0.8em}\n"
              "nav a{color:dimgrey;padding:10px;text-decoration:none}\n"
              "nav a:hover{text-decoration:underline}\n"
              "nav p{margin:0px;padding:0px}\n"
              ".on, .off{margin-top:0;margin-bottom:0.2em;margin-left:4em;font-size:1.4em;background-color:#C0C0C0;border-style:solid;border-radius:10px;border-style:outset;width:5em;height:1.5em;text-decoration:none;text-align:center}\n"
              ".on{border-color:green}\n"
              ".off{border-color:red}\n"
              "message_ok  {color:white;vertical-align:top;display:inline-block;margin:0.2em;padding:0.1em;border-style:solid;border-color:#C0C0C0;background-color:green ;width:19em;text-align:center}\n" 
              "message_warn{color:white;vertical-align:top;display:inline-block;margin:0.2em;padding:0.1em;border-style:solid;border-color:#C0C0C0;background-color:orange;width:19em;text-align:center}\n" 
              "message_err {color:white;vertical-align:top;display:inline-block;margin:0.2em;padding:0.1em;border-style:solid;border-color:#C0C0C0;background-color:red   ;width:19em;text-align:center}\n" 
              );
  server.send(200, "text/css", message);
}

/* =======================================*/
  // Output: send data to browser as JSON
  // after modification always check if JSON is still valid. Just call the JSON (json) in your webbrowser and check.
void handleJson() {
/* =======================================*/
//  Serial.println(F("D268 requested json"));
  String message = "";
  message = (F("{\"ss\":"));                     // Start of JSON and the first object "ss":
  message += millis() / 1000;
  message += (F(",\"val_AHH\":"));
  message += digitalRead(GPin_AHH);
  message += (F(",\"val_AH\":"));
  message += digitalRead(GPin_AH);
  message += (F(",\"val_AL\":"));
  message += digitalRead(GPin_AL);
  message += (F(",\"val_ALL\":"));
  message += digitalRead(GPin_ALL);
  message += (F("}"));                           // End of JSON
  server.send(200, "application/json", message); // set MIME type https://www.ietf.org/rfc/rfc4627.txt
}


/* =======================================*/
  // Output: a fetch API / JavaScript
  // a function in the JavaScript uses fetch API to request a JSON file from the webserver and updates the values on the page if the object names and ID are the same
void handleJs() {
/* =======================================*/
  String message;
  message += F("const url ='json';\n"
               "function renew(){\n"
               " document.getElementById('sec').style.color = 'blue'\n"                                              // if the timer starts the request, the second gets blue
               " fetch(url)\n" // Call the fetch function passing the url of the API as a parameter
               " .then(response => {return response.json();})\n"
               " .then(jo => {\n"
               "   document.getElementById('sec').innerHTML = Math.floor(jo['ss'] % 60);\n"                         // example how to change a value in the HTML page
               "   for (var i in jo)\n"
               "    {if (document.getElementById(i)) document.getElementById(i).innerHTML = jo[i];}\n"               // as long as the JSON name fits to the HTML id, the value will be replaced
               // add other fields here (e.g. the delivered JSON name doesn't fit to the html id
               // finally, update the runtime
               "   if (jo['ss'] > 60) { document.getElementById('min').innerHTML = Math.floor(jo['ss'] / 60 % 60);}\n"
               "   if (jo['ss'] > 3600) {document.getElementById('hour').innerHTML = Math.floor(jo['ss'] / 3600 % 24);}\n"
               "   if (jo['ss'] > 86400) {document.getElementById('day').innerHTML = Math.floor(jo['ss'] / 86400 % 7);}\n"
               "   if (jo['ss'] > 604800) {document.getElementById('week').innerHTML = Math.floor(jo['ss'] / 604800 % 52);}\n"
               "   document.getElementById('sec').style.color = 'dimgray';\n"  // if everything was ok, the second will be grey again.
               " })\n"
               " .catch(function() {\n"                                        // this is where you run code if the server returns any errors
               "  document.getElementById('sec').style.color = 'red';\n"
               " });\n"
               "}\n"
               "document.addEventListener('DOMContentLoaded', renew, setInterval(renew, ");
  message += ajaxIntervall * 1000;
  message += F("));");

  server.send(200, "text/javascript", message);
}


