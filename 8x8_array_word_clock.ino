#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

#define DEBUG_ON

const char *ssid = "ClockBot Access Point"; // The name of the Wi-Fi network that will be created
const char *password = "";   // Blank password for iOS bug

static const char ntpServerName[] = "time.google.com";
const int timeZone = -4;  // Eastern Daylight Time (USA)

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
//void digitalClockDisplay();
//void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

ESP8266WebServer server(80);

const char INDEX_HTML[] =
"<!DOCTYPE HTML>"
"<html>"
"<head>"
"<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
"<title>ClockBot Offline Update</title>"
"<style>"
"body { background-color: #eee; font-family: Arial, Helvetica, Sans-Serif; color: #888; margin: 50px }"
"p { margin: 10px; }"
"label { width: 120px; display: inline-block; font-size: 18px;}"
"select { font-size: 18px; }"
"input { font-size: 20px; }"
"input[type=\"submit\"] { padding: 10px; background-color: green; color: white; border-radius: 8px; margin: 10px; }"
"</style>"
"</head>"
"<body>"
"<h1>ClockBot Settings</h1>"
"<FORM action=\"/\" method=\"post\">"
"<INPUT type=\"hidden\" name=\"clockUpdate\" value=\"1\">"
"<p><label for=\"color\">Word Color</label><select name=\"color\" id=\"color\"><option value=\"0\">White</option><option value=\"1\" selected>Green</option><option value=\"2\">Red</option><option value=\"3\">Gold</option><option value=\"4\">Grey</option><option value=\"5\">Blue</option><option value=\"6\">Orange</option><option value=\"7\">Magenta</option><option value=\"8\">Yellow</option><option value=\"9\">Pink</option><option value=\"10\">Cyan</option><option value=\"11\">DkBlue</option></select></p>"
"<p><label for=\"hour\">Hour</label><input type=\"text\" name=\"hour\" id=\"hour\" value=\"\" maxlength=\"2\" size=\"4\"></p>"
"<p><label for=\"minute\">Minute</label><input type=\"text\" name=\"minute\" id=\"minute\" value=\"\" maxlength=\"2\" size=\"4\"></p>"
"<INPUT type=\"submit\" value=\"Update\">"
"</P>"
"</FORM>"
"</body>"
"</html>";

#include <Adafruit_NeoPixel.h>
#define PIN            5
#define NUMPIXELS      64

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

byte clockSecond, clockMinute, clockHour, clockDayOfWeek, clockMonth, clockYear;

int setHour = 0;
int setMinute = 0;
int setColor = 1;

int delayval = 500; // delay for half a second

int WordIts[] = {0,1,-1};
int WordHalf[] = {2,3,4,5,-1};
int WordMinTen[] = {6,7,-1};

int WordQuarter[] = {8,9,10,11,-1};
int WordTwenty[] = {12,13,14,15,-1};

int WordMinFive[] = {16,17,-1};
int WordMinutes[] = {18,19,20,21,-1};
int WordTo[] = {22,23,-1};

int WordPast[] = {24,25,26,-1};
int WordOne[] = {27,28,-1};
int WordThree[] = {29,30,31,-1};

int WordFour[] = {32,33,34,-1};
int WordTwo[] = {35,36,-1};
int WordFive[] = {37,38,39,-1};

int WordSeven[] = {40,41,42,-1};
int WordSix[] = {43,44,-1};
int WordEight[] = {45,46,47,-1};

int WordNine[] = {48,49,-1};
int WordEleven[] = {50,51,52,53, -1};
int WordTen[] = {54,55,-1};

int WordTwelve[] = {56,57,58,59,-1};
int WordOclock[] = {60,61,62,63,-1};

//int flag = 0;

uint32_t Black = pixels.Color(0, 0, 0);
uint32_t White = pixels.Color(255, 255, 255);
uint32_t Green = pixels.Color(0, 255, 0);
uint32_t Red = pixels.Color(255, 0, 0);
uint32_t Gold = pixels.Color(255, 204, 0);
uint32_t Grey = pixels.Color(50, 50, 50);
uint32_t Blue = pixels.Color(0, 0, 255);
uint32_t Orange = pixels.Color(255, 153, 0);
uint32_t Magenta = pixels.Color(255, 0, 128);
uint32_t Yellow = pixels.Color(255, 255, 0);
uint32_t Pink = pixels.Color(255, 153, 153);
uint32_t Cyan = pixels.Color(102, 204, 255);
uint32_t DkBlue = pixels.Color(0, 50, 80);

uint32_t* colorList[] = {&White, &Green, &Red, &Gold, &Grey, &Blue, &Orange, &Magenta, &Yellow, &Pink, &Cyan, &DkBlue};
#define colorsSize (sizeof(colorList)/sizeof(uint32_t*))


int dayBrightness = 90;
int nightBrightness = 10;

uint32_t characterColor = Cyan;
uint32_t blinkColor = Green;

uint32_t morningColor = Orange;
uint32_t middayColor = Yellow;
uint32_t afternoonColor = Green;
uint32_t eveningColor = Cyan;
uint32_t latenightColor = DkBlue;
uint32_t earlybirdColor = Grey;


  //flag for saving data
  bool shouldSaveConfig = true;

  //callback notifying us of the need to save config
  void saveConfigCallback () {
   Serial.println("Should save config");
   shouldSaveConfig = true;
  }

void setup() {
  pixels.begin();
  blank();
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booted");
  Serial.println("Connecting to Wi-Fi");

    //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //save settings
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(120);
  // wifiManager.autoConnect("ClockBot");

if (MDNS.begin("clockbot")) {  //Start mDNS
Serial.println("mDNS started");
}

  if(!wifiManager.autoConnect("ClockBot")) {
    Serial.println("failed to connect and hit timeout");
    blinkLed();
    //reset and try again, or maybe put it to deep sleep
    // ESP.restart();
  WiFi.hostname("clockbot"); 
  IPAddress ip(192,168,4,9);
  IPAddress gateway(192,168,4,1); 
  IPAddress subnet(255,255,255,0); 
  WiFi.softAPConfig(ip, gateway, subnet);
  
  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");

  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());         // Send the IP address of the ESP8266 to the computer

  }

  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);


  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  
  delay(1000);

 // WiFi.mode(WIFI_STA);
  //WiFi.begin (ssid, password);
  //while (WiFi.status() != WL_CONNECTED) {
 //   Serial.print(".");
 //   delay(500);
//  }
  Serial.println("WiFi connected");
  
  pixels.setBrightness(dayBrightness);
  test(); //run basic screen tests
}

void loop() {

  server.handleClient();
  
  TimeOfDay(); //set brightness dependant on time of day
  displayTime(); // display the real-time clock data on the Serial Monitor  and the LEDS,
  
lightup(WordIts, characterColor);

if (clockMinute == 0) {
    //blinkLed();
    flash();
}

if ((clockMinute >= 0) && (clockMinute < 5)) {
    lightup(WordOclock, characterColor);
  }
  else {
    lightup(WordOclock, Black);
  }

  if (clockMinute < 35) {
    //Set hour if minutes are less than 35
    switch (clockHour) {
      case 1:
      case 13:
        lightup(WordOne, characterColor);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 2:
      case 14:
        lightup(WordOne, Black);
        lightup(WordTwo, characterColor);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 3:
      case 15:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, characterColor);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 4:
      case 16:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, characterColor);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 5:
      case 17:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, characterColor);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 6:
      case 18:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, characterColor);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 7:
      case 19:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, characterColor);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 8:
      case 20:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, characterColor);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 9:
      case 21:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, characterColor);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 10:
      case 22:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, characterColor);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 11:
      case 23:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, characterColor);
        lightup(WordTwelve, Black);
        break;
      case 00:
      case 12:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, characterColor);
        break;
    }// end of case statement
    if ((clockMinute >= 0) && (clockMinute < 5)) {
      lightup(WordPast, Black);
      lightup(WordTo,   Black);
    }
    else {
      lightup(WordPast, characterColor);
      lightup(WordTo,   Black);
    }

  }//end of if statement

  else if (clockMinute > 34) {
    //Set hour if minutes are greater than 34
    switch (clockHour) {
      case 1:
      case 13:
        lightup(WordOne, Black);
        lightup(WordTwo, characterColor);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 2:
      case 14:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, characterColor);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 3:
      case 15:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, characterColor);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 4:
      case 16:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, characterColor);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 5:
      case 17:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, characterColor);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 6:
      case 18:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, characterColor);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 7:
      case 19:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, characterColor);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 8:
      case 20:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, characterColor);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 9:
      case 21:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, characterColor);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
      case 10:
      case 22:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, characterColor);
        lightup(WordTwelve, Black);
        break;
      case 11:
      case 23:
        lightup(WordOne, Black);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, characterColor);
        break;
      case 00:
      case 12:
        lightup(WordOne, characterColor);
        lightup(WordTwo, Black);
        lightup(WordThree, Black);
        lightup(WordFour, Black);
        lightup(WordFive, Black);
        lightup(WordSix, Black);
        lightup(WordSeven, Black);
        lightup(WordEight, Black);
        lightup(WordNine, Black);
        lightup(WordTen, Black);
        lightup(WordEleven, Black);
        lightup(WordTwelve, Black);
        break;
    }// end of case statement
    lightup(WordPast, Black);
    lightup(WordTo,   characterColor);
  } // end of if statement to test for greater than 34

  if ((clockMinute >= 5) && (clockMinute < 10)) {
    lightup(WordMinFive, characterColor);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, Black);
    lightup(WordMinutes, characterColor);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 10) && (clockMinute < 15)) {
    lightup(WordMinFive, Black);
    lightup(WordMinTen, characterColor);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, Black);
    lightup(WordMinutes, characterColor);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 15) && (clockMinute < 20)) {
    lightup(WordMinFive, Black);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, characterColor);
    lightup(WordTwenty, Black);
    lightup(WordMinutes, Black);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 20) && (clockMinute < 25)) {
    lightup(WordMinFive, Black);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, characterColor);
    lightup(WordMinutes, characterColor);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 25) && (clockMinute < 30)) {
    lightup(WordMinFive, characterColor);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, characterColor);
    lightup(WordMinutes, characterColor);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 30) && (clockMinute < 35)) {
    lightup(WordMinFive, Black);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, Black);
    lightup(WordMinutes, Black);
    lightup(WordHalf, characterColor);
  }
  else if ((clockMinute >= 35) && (clockMinute < 40)) {
    lightup(WordMinFive, characterColor);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, characterColor);
    lightup(WordMinutes, characterColor);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 40) && (clockMinute < 45)) {
    lightup(WordMinFive, Black);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, characterColor);
    lightup(WordMinutes, characterColor);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 45) && (clockMinute < 50)) {
    lightup(WordMinFive, Black);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, characterColor);
    lightup(WordTwenty, Black);
    lightup(WordMinutes, Black);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 50) && (clockMinute < 55)) {
    lightup(WordMinFive, Black);
    lightup(WordMinTen, characterColor);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, Black);
    lightup(WordMinutes, characterColor);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 55) && (clockMinute <= 59)) {
    lightup(WordMinFive, characterColor);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, Black);
    lightup(WordMinutes, characterColor);
    lightup(WordHalf, Black);
  }
  else if ((clockMinute >= 0) && (clockMinute < 5)) {
    lightup(WordMinFive, Black);
    lightup(WordMinTen, Black);
    lightup(WordQuarter, Black);
    lightup(WordTwenty, Black);
    lightup(WordMinutes, Black);
    lightup(WordHalf, Black);

  }


}

void TimeOfDay() {
  //Used to set brightness dependant of time of day - lights dimmed at night

  //monday to thursday and sunday

  if ((clockDayOfWeek == 6) | (clockDayOfWeek == 7)) {
    if ((clockHour > 0) && (clockHour < 8)) {
      pixels.setBrightness(nightBrightness);
    }
    else {
      pixels.setBrightness(dayBrightness);
    }
  }
  else {
    if ((clockHour < 6) | (clockHour >= 22)) {
      pixels.setBrightness(nightBrightness);
    }
    else {
      pixels.setBrightness(dayBrightness);
    }
  }
}

void displayTime()
{
  readtime(&clockSecond, &clockMinute, &clockHour, &clockDayOfWeek, &clockMonth, &clockYear);

  if (clockHour < 10) {
    Serial.print("0");
  }
  Serial.print(clockHour);
  Serial.print(":");

  if (clockMinute < 10) {
    Serial.print("0");
  }
  Serial.println(clockMinute);
  delay(200);
}

void readtime(byte *clockSecond, byte *clockMinute, byte *clockHour, byte *clockDayOfWeek, byte *clockMonth, byte *clockYear) {
    *clockSecond = second();
    *clockMinute = minute();
    *clockHour = hour();
    *clockDayOfWeek = weekday();
    *clockMonth = month();
    *clockYear = year();
}

void lightup(int Word[], uint32_t ColorName) {
  for (int x = 0; x < pixels.numPixels() + 1; x++) {
    if (Word[x] == -1) {
      pixels.show();
      break;
    } //end of if loop
    else {
      pixels.setPixelColor(Word[x], ColorName);
      pixels.show();
    } // end of else loop
  } //end of for loop
}

void blinkLed() {
  
    pixels.setPixelColor(0, blinkColor);
    pixels.setPixelColor(1, blinkColor);
    pixels.show();

    delay(100);
    pixels.setPixelColor(0, Black);
    pixels.setPixelColor(1, Black);
    pixels.show();
    delay(50);

}

void blank() {
  //clear the decks
  for (int x = 0; x < NUMPIXELS; ++x) {
    pixels.setPixelColor(x, Black);
  }
  pixels.show();

}

void wipe() {

  for (int x = 0; x < NUMPIXELS; ++x) {
    pixels.setPixelColor(x, Blue);
    delay(10);
    pixels.show();
  }
  delay(50);
  for (int x = NUMPIXELS; x > -1; --x) {
    pixels.setPixelColor(x, Black);
    delay(10);
    pixels.show();
  }

  for (int x = 0; x < NUMPIXELS; ++x) {
    pixels.setPixelColor(x, Green);
    delay(10);
    pixels.show();
  }
  delay(50);
  for (int x = NUMPIXELS; x > -1; --x) {
    pixels.setPixelColor(x, Black);
    delay(10);
    pixels.show();
  }

  for (int x = 0; x < NUMPIXELS; ++x) {
    pixels.setPixelColor(x, Red);
    delay(10);
    pixels.show();
  }
  delay(50);
  for (int x = NUMPIXELS; x > -1; --x) {
    pixels.setPixelColor(x, Black);
    delay(10);
    pixels.show();
  }


  delay(100);
  blank();

}

void test() {
  blank();
  wipe();
  blank();
  //flash();
}

void flash() {

  blank();
  for (int y = 0; y < 10; ++y) {
    for (int x = 0; x < NUMPIXELS; x = x + 2) {
      pixels.setPixelColor(x, blinkColor);
    }
    //pixels.setBrightness(dayBrightness);
    pixels.show();
    delay(50);
    blank();
    delay(50);

    for (int x = 1; x < NUMPIXELS; x = x + 2) {
      pixels.setPixelColor(x, blinkColor);
    }
    //pixels.setBrightness(dayBrightness);
    pixels.show();
    delay(50);
    blank();
    delay(50);
  }
  blank();
}

void handleRoot()
{
  if (server.hasArg("clockUpdate")) {
    handleSubmit();
  }
  else {
    server.send(200, "text/html", INDEX_HTML);
  }
}

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleSubmit()
{
  String LEDvalue;

  if (!server.hasArg("clockUpdate")) return returnFail("BAD ARGS");
  setColor = server.arg("color").toInt();
  setHour = server.arg("hour").toInt();
  setMinute = server.arg("minute").toInt();
  
  if (setColor!='\0')
    {
      characterColor = *colorList[setColor];
    }

 if (setHour!='\0')
    {
      clockHour = setHour;
    }
 if (setMinute!='\0')
    {
      clockMinute = setMinute;
    }

          server.send(200, "text/html", INDEX_HTML);
            setTime(sync()); 
}

void returnOK()
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK\r\n");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  setHour = server.arg("hour").toInt();
  setMinute = server.arg("minute").toInt();
  setColor = server.arg("color").toInt();

 characterColor = *colorList[setColor];
  if (setColor!='\0')
    {
      characterColor = *colorList[setColor];
    }
 if (setHour!='\0')
    {
      clockHour = setHour;
    }
 if (setMinute!='\0')
    {
      clockMinute = setMinute;
    }

  setTime(sync()); 
}

time_t sync() {
  tmElements_t tm;
  tm.Hour = clockHour;
  tm.Minute = clockMinute;
  tm.Second = clockSecond;
  tm.Day = 6;
  tm.Month = 4;
  tm.Year = 2019-1970;
  return makeTime(tm);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
