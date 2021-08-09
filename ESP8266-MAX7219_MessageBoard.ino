// Webserver for displaying a text on a scrolled display (based on the MAX7219)
// On startup it tryes to connect to a WiFi network.
// the ssid and password are stored in the flash memory.
// If it is not possible to connect to an existing WiFi network it will act as a access point.
// In access point modes the ssid will be DotMtrx with the IP adress 192.168.4.1
// Tested on a Wemos D1, Wemos D1 mini, NodeMCU V3

#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <MD_MAX72xx.h>
#include <ESP_EEPROM.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES  8
#define CLK_PIN   D5 // 13  // or SCK
#define DATA_PIN  D7 // 11  // or MOSI
#define CS_PIN    D8 // 10  // or SS
#define  DELAYTIME  100  // in milliseconds

String message = "";
char displayTekst[255] ="";
String htmlPage;

ESP8266WebServer web_server(80);
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

void setup() {
  mx.begin();
  start_wifi();  
  web_server.on("/", handle_http_root);
  web_server.on("/wifi", handle_http_wifi);
  web_server.on("/wifisave", handle_http_wifisave);
  web_server.begin();
  delay(100);
}

void loop() {
  web_server.handleClient();
  message.toCharArray(displayTekst,message.length()+1);
  scrollText(displayTekst);
  delay ( 1000 );
}

void scrollText(char *p) {
  uint8_t charWidth;
  uint8_t cBuf[8];  // this should be ok for all built-in fonts
  mx.clear();
  while (*p != '\0')
  {
    web_server.handleClient();
    charWidth = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
    for (uint8_t i=0; i<=charWidth; i++)  // allow space between characters
    {
      mx.transform(MD_MAX72XX::TSL);
      if (i < charWidth)
      mx.setColumn(0, cBuf[i]);
      delay(DELAYTIME);
    }
  }
}

void handle_http_root() {
  web_server.send(200, "text/html", "<form method='get'><table><tr><td><label>Message?: </label></td><td><input name='message' ></td></tr><tr><td><input type='submit' value='Save'></td></tr></table></form></html>");
  if (web_server.arg("message").length() >0){
    message=web_server.arg("message");
  }
}

void handle_http_wifi() {
  web_server.send(200, "text/html", get_wifiPage());
} 
String get_wifiPage(){
  String htmlPage = "Please enter Wlan settings<br/>";
  htmlPage += "<form method='get' action='wifisave'><table><tr><td><label>SSID: </label></td><td><input name='ssid' length=32></td></tr>";
  htmlPage += "<tr><td><label>Password: </label></td><td><input name='pass' length=64></td></tr><tr><td><input type='submit' value='Save'></td></tr></table></form>";
  htmlPage += "</html>";
  return htmlPage;
}

void handle_http_wifisave() {
  String qsid = web_server.arg("ssid");
  String qpass = web_server.arg("pass");
  Serial.println(qsid);
  Serial.println(qpass);
  EEPROM_write(qsid, 0);
  EEPROM_write(qpass, 32);
  web_server.send(200, "application/json", "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}");
  delay ( 1000 );
  setup();
}

void start_wifi() {
  // Connect to WiFi network
  WiFi.disconnect();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(EEPROM_read(0,32).c_str(), EEPROM_read(32,64).c_str());
  int trys = 0;
  while ( WiFi.status() != WL_CONNECTED && trys<= 5) {
    trys++;
    delay ( 1000 );
  }
  if(WiFi.isConnected()) {
    message= "Link => " + EEPROM_read(0,32) + " " + WiFi.localIP().toString() + " " + WiFi.macAddress();
  }
  else {
    // Start AP Mode
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP("DotMtrx","");
    message= "AP Mode ( DotMtrx ) " + WiFi.softAPIP().toString() + " " + WiFi.macAddress();
  }
}

void EEPROM_write(String buffer, int N) {
  EEPROM.begin(512); delay(10);
  for (int L = 0; L < 32; ++L) {
    EEPROM.write(N + L, buffer[L]);
  }
  EEPROM.commit();
}

String EEPROM_read(int min, int max) {
  EEPROM.begin(512); delay(10); String buffer;
  for (int L = min; L < max; ++L)
    if (isAlphaNumeric(EEPROM.read(L)))
      buffer += char(EEPROM.read(L));
  return buffer;
}
