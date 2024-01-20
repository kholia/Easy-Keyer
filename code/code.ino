// Borrowed from https://github.com/vu3ave/wifi-cw-keyboard
//
// Version 1.2 AVE RnD CW Keyer (1 / 1 / 2023)
//
// Version 2.0 Dhiru (Jan-2024) - Cleanups and maintainability
//
// MCU will run in AP mode and create a WiFi hotspot named "92.168.4.1". Log in
// to that wifi network (no password needed) using a computer or phone. Now ,
// navigate to http://192.168.4.1/ using your web browser Type in English using
// default keyboard and that will be converted to telegraphy messages (via Pin
// 4) and sent as tone as well as digital out (via Pin 5).
//
// Use browser cookies instead of simulated EEPROM on ESP8266 modules ;)

#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

int count = 0;
int last_sent_location = 0;
int messagetocw();
String data;
#define P_CW  5  // ESP12 -(GPIO 10) // CW digital output
int KEY_PIN = 5; // relay drive to transimit cw in port of ESP8266
int sm_time;
int TIME_UNIT = 100; // default time for DIT
bool dxing_mode = true; // put false for normal mode
String morsecode;
String message;
String dit_time;
String freq;
int present_message_length;
String present_message;
int tonepin = 4;
int my_tone_frequency = 1000;

const String  characters = "abcdefghijklmnopqrstuvwxyz1234567890?.,/-; ";

String mappings[] = {
  ".-",  // A
  "-...",
  "-.-.",
  "-..",
  ".",
  "..-.",
  "--.",
  "....",
  "..",
  ".---",
  "-.-",
  ".-..",
  "--",
  "-.",
  "---",
  ".--.",
  "--.-",
  ".-.",
  "...",
  "-",
  "..-",
  "...-",
  ".--",
  "-..-",
  "-.--",
  "--..", // Z
  ".----",  // 1
  "..---",
  "...--",
  "....-",
  ".....",
  "-....",
  "--...",
  "---..",
  "----.",
  "-----",   // 0
  "..--..",  // ?
  ".-.-.-",  // .
  "--..--",  // ,
  "-..-.",   // /
  "-...-",   // -  // spacing symbol
  ".-.-.",   // ;  //  "AR" symbol
  ""
};


// SSID and Password of your WiFi router
const char* ssid = "192.168.4.1";
const char* password = "";

ESP8266WebServer server(80);

void handleRoot() {
  File index = SPIFFS.open("/index.html", "r");
  server.streamFile(index, "text/html");
  index.close();
}

void handleRootCSS() {
  File index = SPIFFS.open("/style.css", "r");
  server.streamFile(index, "text/css");
  index.close();
}

void handleRootICO() {
  File index = SPIFFS.open("/favicon.ico", "r");
  server.streamFile(index, "image/x-icon");
  index.close();
}

void handleWPMTXT() {
  dit_time = server.arg("mytxt");
  Serial.println("received dit_time is :");
  Serial.println(dit_time);
  Serial.println("");
  server.send(200, "text/plain", dit_time);
  data = dit_time;
  TIME_UNIT = data.toInt(); // CONVERTING STRING TO INT and STORING IN TIME_UNIT (NEW WPM STORED )
}

void handleTONETXT() {
  freq = server.arg("mytxt");
  Serial.println("received frequency  is :");
  Serial.println(freq );
  Serial.println("");
  server.send(200, "text/plain", freq);
  my_tone_frequency = data.toInt();
}

void handleCWTXT() {
  message = server.arg("mytxt");
  Serial.println(message);
  server.send(200, "text/plain", message);
  present_message += message;
  message = "";
}

void setup(void) {
  pinMode(P_CW, OUTPUT);
  pinMode(KEY_PIN, OUTPUT);
  pinMode(tonepin, OUTPUT);

  Serial.begin(115200);
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/style.css", handleRootCSS);
  server.on("/favicon.ico", handleRootICO);
  server.on("/cwtxt", handleCWTXT);
  server.on("/wpmtxt", handleWPMTXT);
  server.on("/tonetxt", handleTONETXT);
  server.begin();                  //Start server
  Serial.println("");
  Serial.println("HTTP server started");

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Start with the defaults
  TIME_UNIT = 100;
  my_tone_frequency = 1000;
}

void loop(void) {
  int result = messagetocw();

  server.handleClient();
}

int messagetocw()
{
  present_message_length = present_message.length();

  if (last_sent_location > present_message_length) {
    last_sent_location = present_message_length;
  }

  const int DOT = TIME_UNIT;
  const int DASH = 3 * TIME_UNIT;

  int ss;
  int ls;
  int ws;
  if (dxing_mode) {
    ss = DOT;
    ls = 0.2 * DOT;
    ws = 1 * DOT;
  }
  else {
    ss = DOT;
    ls = 2 * DOT;
    ws = 5 * DOT;
  }

  const int SYMBOL_SPACE = ss;
  const int LETTER_SPACE = ls;
  const int WORD_SPACE = ws;
  present_message.toLowerCase();
  char ch = characters.charAt(characters.indexOf(present_message.charAt(last_sent_location)));//
  if (ch != '\0') {
    int position = characters.indexOf(present_message.charAt(last_sent_location));
    last_sent_location = last_sent_location + 1;
    morsecode = mappings[position];
    int count = morsecode.length();
    for ( int j = 0; j < count; j++) {
      char symbol = morsecode[j];
      if (symbol == '.') {
        sm_time = DOT;
      }

      if (symbol == '-')
      {
        sm_time = DASH;
      }
      digitalWrite(KEY_PIN, HIGH);

      tone(tonepin, my_tone_frequency);
      delay(sm_time);
      digitalWrite(KEY_PIN, LOW);
      noTone(tonepin);
      delay(SYMBOL_SPACE);
    }
    delay(LETTER_SPACE);
  }
  delay(WORD_SPACE);

  return 0;
}

// Key the transmitter / lambic keyer
void keyAndBeep(int speed)
{
  digitalWrite(P_CW, HIGH);
  for (int i = 0; i < speed; i++) {
    delay(2);
  }
  digitalWrite(P_CW, LOW);
}
