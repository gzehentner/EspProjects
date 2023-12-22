/*
Georgs server.ino
*/
void handleRoot() {
  /*
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266! GZE\r\n");
  digitalWrite(led, 0);
  */
  // create root page with links to subpages
  String message;
  addTop(message);
  message += F("<!DOCTYPE html>\n"
              "<html lang='en'>\n"
              "<head>\n"
              "<title>" TXT_BOARDNAME " - Board " TXT_BOARDID"</title>\n"
              "</head>\n"
              "<body>\n"
              "<article>\n"
              "<h1>Your webserver on " TXT_BOARDNAME " - Board " TXT_BOARDID " is running!</h1>\n"
              "</article>\n"
              "<p></p>\n"
              "<article>\n"
              "<h2>Here is a list of subpages</h2>\n"
              "<p>-  <a href='0.htm'>Page 0 -- handlePage</a></p>\n"
              "<p>-  <a href='1.htm'>Page 1 -- handlePage1</a></p>\n"
              "<p>-  <a href='2.htm'>Page 2 -- handlePage2</a></p>\n"
              "<p>-  <a href='x.htm'>Page x -- handleAnotherPage</a></p>\n"
              "<p>-  <a href='f.css'>Page f.css</a></p>\n"
              "<p>-  <a href='j.js'> Page j.js</a></p>\n"
              "<p>-  <a href='json'> Page json</a></p>\n"
              "<p>-  <a href='c.php?toggle=3' > Toggle LED</a></p>\n"
              "<p>-  <a href='c.php?CMD=RESET'> Reset ESP       </a></p>\n"
              "</article>\n"
              "</body>\n"
              "</html>");
  addBottom(message);
  server.send(200, "text/html", message);

}



void handleNotFound() {
  digitalWrite(led, 1);
  // Output a "404 not found" page. It includes the parameters which comes handy for test purposes.
  Serial.println(F("D015 handleNotFound()"));
  

  String message;
  message += F("404 - File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");
  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);

}

void handleOtherPage()
// a very simple example how to output a HTML page from program memory
{
  String message;
  addTop(message);
  message += F("<!DOCTYPE html>\n"
              "<html lang='en'>\n"
              "<article_wide>\n"
              "<head>\n"
              "<title>" TXT_BOARDNAME " - Board " TXT_BOARDID "</title>\n"
              "</head>\n"
              "<body>\n"
              "<h1>Your webserver on " TXT_BOARDNAME " - Board " TXT_BOARDID " is running!</h1>\n"
              "<p>This is just an example how you can create a webpage.</p>\n"
              "<p>But as most of my webserver should have the same look and feel"
              " I'm using one layout for all html pages.</p>\n"
              "<p>Therefore all my html pages come from the function handlePage()</p>\n"
              "<p>To go back to the formated pages <a href='/'>use this link</a></p>\n"
              "</body>\n"
              "</article_wide>\n"
              "</html>");
  addBottom(message);
  server.send(200, "text/html", message);
}
// Add header to website
void addTop(String& message) {
  message = F("<!DOCTYPE html>\n"
              "<html lang='en'>\n"
              "<head>\n"
              "<title>" TXT_BOARDNAME " - Board " TXT_BOARDID "</title>\n"
              "<link rel='stylesheet' type='text/css' href='/f.css'>\n"
              "<script src='j.js'></script>\n"
              "</head>\n");
}
// Add footer to website
void addBottom(String& message) {
  message += F("<footer>"
              "<article_wide>\n"
               "<p>Author: Georg Zehentner</p>"
              "<p><a href=\"mailto:gzehentner@web.de\">gzehentner@web.de</a></p>"
              "<p><a href=/> Goto Root</a></p>"
              "</article_wide>\n"
              "</footer>)");

}

void handlePage()
{
String message;
addTop(message);

message += F("<article>\n"
"<h2>Homepage</h2>\n" // here you write your html code for your homepage. Let's give some examples...
"<p>This is an example for a webserver on your ESP8266. "
"Values are getting updated with Fetch API/JavaScript and JSON.</p>\n"
"</article>\n");

message += F("<article>\n"
"<h2>Values (with update)</h2>\n");
message += F("<p>Internal Voltage measured by ESP: <span id='internalVcc'>"); 
message += ESP.getVcc();
message += F("</span>mV</p>\n");

message += F("<p>Button 1: <span id='button1'>"); // example how to show values on the webserver
message += digitalRead(BUTTON1_PIN);
message += F("</span></p>\n");

message += F("<p>Output 1: <span id='output1'>"); // example 3
message += digitalRead(OUTPUT1_PIN);
message += F("</span></p>\n");

message += F("<p>Output 2: <span id='output2'>"); // example 4
message += digitalRead(OUTPUT2_PIN);
message += F("</span></p>\n"
"</article>\n");

message += F("<article>\n"
"<h2>Switch</h2>\n" // example how to switch/toggle an output
"<p>Example how to switch/toggle outputs, or to initiate actions. "
"The buttons are 'fake' buttons and only styled by CSS. Click to toggle the output.</p>\n"
"<p class='off'><a href='c.php?toggle=1' target='i'>Output 1</a></p>\n"
"<p class='off'><a href='c.php?toggle=2' target='i'>Output 2</a></p>\n"
"<iframe name='i' style='display:none' ></iframe>\n" // hack to keep the button press in the window
"</article>\n");

addBottom(message);
server.send(200, "text/html", message);
}


/*=============================================================*/
void handleCss()
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
              "article_wide{vertical-align:top;display:inline-block;margin:0.2em;padding:0.1em;border-style:solid;border-color:#C0C0C0;background-color:#E5E5E5;width:40em;text-align:left}\n" // if you don't like the floating effect (vor portrait mode on smartphones!) - remove display:inline-block
              "article h2{margin-top:0;padding-bottom:1px}\n"
              "section {margin-bottom:0.2em;clear:both;}\n"
              "table{border-collapse:separate;border-spacing:0 0.2em}\n"
              "th, td{background-color:#C0C0C0}\n"
              "button{margin-top:0.3em}\n"
              "footer p{font-size:0.7em;color:dimgray;background:silver;text-align:center;margin-bottom:5px}\n"
              "nav{background-color:silver;margin:1px;padding:5px;font-size:0.8em}\n"
              "nav a{color:dimgrey;padding:10px;text-decoration:none}\n"
              "nav a:hover{text-decoration:underline}\n"
              "nav p{margin:0px;padding:0px}\n"
              ".on, .off{margin-top:0;margin-bottom:0.2em;margin-left:4em;font-size:1.4em;background-color:#C0C0C0;border-style:solid;border-radius:10px;border-style:outset;width:5em;height:1.5em;text-decoration:none;text-align:center}\n"
              ".on{border-color:green}\n"
              ".off{border-color:red}\n");
  server.send(200, "text/css", message);
}

void handleJs() {
  // Output: a fetch API / JavaScript
  
  String message;
  message += F("const url ='json';\n"
               "function renew(){\n"
               " document.getElementById('sec').style.color = 'blue'\n"   
               " fetch(url)\n" // Call the fetch function passing the url of the API as a parameter
               " .then(response => {return response.json();})\n"
               " .then(jo => {\n"
               "   document.getElementById('sec').innerHTML = Math.floor(jo['ss'] % 60); \n"     
               "   for (var i in jo)\n"
               "    {if (document.getElementById(i)) document.getElementById(i).innerHTML = jo[i];}\n"    
               // add other fields here (e.g. the delivered JSON name doesn't fit to the html id
               // finally, update the runtime
               "   if (jo['ss'] > 60) { document.getElementById('min').innerHTML = Math.floor(jo['ss'] / 60 % 60);}\n"
               "   if (jo['ss'] > 3600) {document.getElementById('hour').innerHTML = Math.floor(jo['ss'] / 3600 % 24);}\n"
               "   if (jo['ss'] > 86400) {document.getElementById('day').innerHTML = Math.floor(jo['ss'] / 86400 % 7);}\n"
               "   if (jo['ss'] > 604800) {document.getElementById('week').innerHTML = Math.floor(jo['ss'] / 604800 % 52);}\n"
               "   document.getElementById('sec').style.color = 'dimgray';\n"  
               " })\n"
               " .catch(function() {\n"            
               "  document.getElementById('sec').style.color = 'red';\n"
               " });\n"
               "}\n"
               "document.addEventListener('DOMContentLoaded', renew, setInterval(renew, ");
  message += ajaxIntervall * 1000;
  message += F("));");
              
  server.send(200, "text/javascript", message);
}

void handleJson()
{
  // Output: send data to browser as JSON

  String message = "";
  message = (F("{\"ss\":")); // Start of JSON and the first object "ss":
  message += millis() / 1000;
  message += (F(",\"internalVcc\":"));
  message += ESP.getVcc();
  message += (F(",\"button1\":"));
  message += digitalRead(BUTTON1_PIN);
  message += (F(",\"output1\":"));
  message += digitalRead(OUTPUT1_PIN);
  message += (F("}")); // End of JSON
  server.send(200, "application/json", message);
}

void handleCommand()
{
  // receive command and handle accordingly
  Serial.println(F("D223 handleCommand"));
  for (uint8_t i = 0; i < server.args(); i++)
  {
    Serial.print(server.argName(i));
    Serial.print(F(": "));
    Serial.println(server.arg(i));
  }
  if (server.argName(0) == "toggle") // the parameter which was sent to this server
  {
    if (server.arg(0) == "1") // the value for that parameter
    {
      Serial.println(F("D232 toggle output1"));
      if (digitalRead(OUTPUT1_PIN))
      { // toggle: if the pin was on - switch it of and vice versa
        digitalWrite(OUTPUT1_PIN, LOW);
      }
      else
      {
        digitalWrite(OUTPUT1_PIN, HIGH);
      }
    }
    if (server.arg(0) == "2") // the value for that parameter
    {
      Serial.println(F("D232 toggle output2"));
      if (digitalRead(OUTPUT2_PIN))
      {
        digitalWrite(OUTPUT2_PIN, LOW);
      }
      else
      {
        digitalWrite(OUTPUT2_PIN, HIGH);
      }
    }
    if (server.arg(0) == "3") // the value for that parameter(led))
    {
      Serial.println(F("D232 toggle LED"));
      if (digitalRead(led))
      { // toggle: if the pin was on - switch it of and vice versa
        digitalWrite(led, LOW);
      }
      else
      {
        digitalWrite(led, HIGH);
      }
    }
  }
  else if (server.argName(0) == "CMD" && server.arg(0) == "RESET") // Example how to reset the module. Just send ?CMD=RESET
  {
    Serial.println(F("D238 will reset"));
    ESP.restart();
  }
  server.send(204, "text/plain", "No Content"); // this page doesn't send back content --> 204
}