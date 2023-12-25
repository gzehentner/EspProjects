void sendPost()
{
  WiFiClient wificlient;
  HTTPClient client;
  const uint16_t MESSAGE_SIZE_MAX = 300;                   // maximum bytes for Message Buffer
  char message[MESSAGE_SIZE_MAX];                          // the temporary sending message - html body
  char val[32];                                            // buffer to convert floats and integers before appending

  strcpy(message, "board=");                               // Append chars
  strcat(message, TXT_BOARDID);

  strcat(message, "&vcc=");                                // Append integers
  itoa(ESP.getVcc(), val, 10);
  strcat(message, val);

  strcat(message, "&output1=");
  itoa(digitalRead(OUTPUT1_PIN), val, 10);
  strcat(message, val);

  strcat(message, "&output2=");
  itoa(digitalRead(OUTPUT2_PIN), val, 10);
  strcat(message, val);

  strcat(message, "&button1=");
  itoa(digitalRead(BUTTON1_PIN), val, 10);
  strcat(message, val);

  client.begin(wificlient, sendHttpTo);                                        // Specify request destination
  client.addHeader("Content-Type", "application/x-www-form-urlencoded");       // Specify content-type header
  int httpCode = client.POST(message);                                         // Send the request
  client.writeToStream(&Serial);                                               // Debug only: Output of received data
  Serial.print(F("\nhttpCode: "));                                             
  Serial.println(httpCode);                                                    // Print HTTP return code

  client.end();  //Close connection
}