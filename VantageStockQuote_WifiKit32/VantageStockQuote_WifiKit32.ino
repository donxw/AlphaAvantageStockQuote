// AlphaVantage Key Signup:
// https://www.alphavantage.co/support/#api-key
// https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=HPQ&apikey=86GGFRIQSM2UVJAC
// to use https - dowload and format the root certificate from the website:  https://techtutorialsx.com/2017/11/18/esp32-arduino-https-get-request/
// this will only work on ESP32 due to the secure client library
// board devkit 32 or Heltec wifikit 32
// CPU freq reduced to 80MHz to reduce power

#include <ArduinoJson.h>
#include <esp_wifi.h>

// WiFi and AP
#define WIFI_SSID       "xxxxx"
#define WIFI_PASS       "xxxxxxx"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
WebServer server(80);
#include <HTTPClient.h>

//OLED Display
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
// U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C       u8g2(U8G2_R0, 16, 15, 4);         // OLED graphics (rotation, clock, data, reset)

//AlphaVantage API Variables
String myticker = "HPQ";
String myAPIKEY = "xxxxxxxxxxxxxx";

char char_price[20], char_change[20], char_date[20];
String month[12] = {"Jan", "Feb", "Mch", "Apr", "May", "Jun", "Jly", "Aug", "Sep", "Oct", "Nov", "Dec"};
char mon[15];

const char* root_ca = \
                      "-----BEGIN CERTIFICATE-----\n" \
                      "MIIFXzCCBEegAwIBAgISBFI8aC5lnovKspMwhMhDxH4BMA0GCSqGSIb3DQEBCwUA\n" \
                      "MEoxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MSMwIQYDVQQD\n" \
                      "ExpMZXQncyBFbmNyeXB0IEF1dGhvcml0eSBYMzAeFw0yMDExMTYwNTI3NTBaFw0y\n" \
                      "MTAyMTQwNTI3NTBaMB4xHDAaBgNVBAMTE3d3dy5hbHBoYXZhbnRhZ2UuY28wggEi\n" \
                      "MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC81HBqgLQqHqZkpBPbmBpGmQsX\n" \
                      "ZPimoVLJwh4tQLI+VKTX6R401fvrV2uQ0CntvrKxcGMoYkE2z+VEVZHeE/7mXoaz\n" \
                      "+aLxnuxPVG1V1FOuF1EHWc+gRTKLdUEiNH+zHV3AT5tNk3GYpSJ1DPMP+bu0nBnk\n" \
                      "BkXUGknYm/VHRPWDoNEuTg8N9sz8HnREPtze4hlikUqMfzb1nYJk7sOJYBY4jhfB\n" \
                      "p6GzET59rm0NDjrG3A/by7Ks/HC5npY+DoO+5YZ6Nrov9DrK9cQEAD/CnZG7zJkm\n" \
                      "p3DOPcO9DM3Yecf7SVuKQdxLUVdC2rIcLNO4PEsoRspveTu03HREVHPwrtA9AgMB\n" \
                      "AAGjggJpMIICZTAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYBBQUHAwEG\n" \
                      "CCsGAQUFBwMCMAwGA1UdEwEB/wQCMAAwHQYDVR0OBBYEFFSlQ1MURpGRAVNxAuZw\n" \
                      "PaqLkWhuMB8GA1UdIwQYMBaAFKhKamMEfd265tE5t6ZFZe/zqOyhMG8GCCsGAQUF\n" \
                      "BwEBBGMwYTAuBggrBgEFBQcwAYYiaHR0cDovL29jc3AuaW50LXgzLmxldHNlbmNy\n" \
                      "eXB0Lm9yZzAvBggrBgEFBQcwAoYjaHR0cDovL2NlcnQuaW50LXgzLmxldHNlbmNy\n" \
                      "eXB0Lm9yZy8wHgYDVR0RBBcwFYITd3d3LmFscGhhdmFudGFnZS5jbzBMBgNVHSAE\n" \
                      "RTBDMAgGBmeBDAECATA3BgsrBgEEAYLfEwEBATAoMCYGCCsGAQUFBwIBFhpodHRw\n" \
                      "Oi8vY3BzLmxldHNlbmNyeXB0Lm9yZzCCAQUGCisGAQQB1nkCBAIEgfYEgfMA8QB3\n" \
                      "AESUZS6w7s6vxEAH2Kj+KMDa5oK+2MsxtT/TM5a1toGoAAABdc+8+xMAAAQDAEgw\n" \
                      "RgIhAJwiB9OUFrhreADbDZT6J9yS0k2cThLv41A+F9D83Q0CAiEAhKeGGA3UUx/l\n" \
                      "xe9fvGXeU55ae3v2QCefjH9+/T2BWyUAdgD2XJQv0XcwIhRUGAgwlFaO400TGTO/\n" \
                      "3wwvIAvMTvFk4wAAAXXPvPsSAAAEAwBHMEUCIQDx1iAae1+j/sZzgOZwVN0ZmPqY\n" \
                      "Y9uXe0h/Fy4bdfATTQIgaFBjfHvvGz/uHVnNbFE07rIut27+/0qgSRlq01/3MCEw\n" \
                      "DQYJKoZIhvcNAQELBQADggEBAFLbp5Zewfrt+ZV6wnC0Aikv7qZhdl5ylvxSVvBG\n" \
                      "clXJZN2XrA6W1uSXQQGItWR3UT1ZQ9wAyGPmRTT8DTwtVkm7qYxOeqNPsztsEQxq\n" \
                      "rnzBdC/VFbJkJCCKY9TrNJIYybVljT603XaHl4sn6vG5B3HAFxtFM7L55Gsr2wLX\n" \
                      "75VQXzh+3blg7tNGKjbmj6aOElSGpvDqbH+TVo0Gt9DhtlBywuOF7QSMWino2Rrq\n" \
                      "6u63Ad4zuKmzFafMjs69iFanu95UneoUwdJsA7PQo4Tbk4Rkd5x4aUzOb72tS+tW\n" \
                      "IWOfB+YoXEVF1wkzreF7vgr3IlIqc+ftaeZUnJ/a3ThpnHU=\n" \
                      "-----END CERTIFICATE-----\n";

char oledbuf[80];

//Define touch sensitivity. Greater the value, more the sensitivity.
#define Threshold 40
touch_pad_t touchPin;

#define uS_TO_S_FACTOR 1000000UL  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  60*60UL        //Time ESP32 will go to sleep (in seconds) - 60m sleep

void callback() {
  //placeholder callback function
}


void setup() {

  pinMode(25, OUTPUT);  //GPIO 25 is LED pin on Heltec Wifi kit 32
  digitalWrite(25, HIGH);   // turn the LED on (HIGH is the voltage level)

  Serial.begin(115200);


  //Setup interrupt on Touch Pad 7 (GPIO27)
  touchAttachInterrupt(T7, callback, Threshold);

  // Start display
  u8g2.begin();
  Wire.begin(4, 15);  // map correct SCL SDA pins for Heltec WifiKit 32

  // Start Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println(F("Connecting..."));
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // get stock quote
  GetVantageQuote(myticker, myAPIKEY);
  
  // display results to screen
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_cupcakemetoyourleader_tr); // choose a suitable font

  // display last price
  u8g2.setCursor(0, 20);
  String str_price = char_price;
  u8g2.printf("%s: $%s", myticker, str_price.substring(0, 5));

  // display % change
  float f_change = atof(char_change);
  u8g2.setCursor(0, 40);
  u8g2.printf("%s %*.1f%%", f_change >= 0 ? "UP: " : "DN: ", f_change);

  // display date
  u8g2.setCursor(0, 60);
  String str_date = char_date;

  // Extract Year
  String str_year = str_date.substring(0, 4);
  Serial.println(str_year);

  // Extract Month
  String str_mon = str_date.substring(5, 7);
  int i_mon = atoi(str_mon.c_str());
  String str_month = month[i_mon - 1];
  Serial.println(str_month);

  // Extract Day
  String str_day = str_date.substring(8, 10);
  Serial.println(str_day);
  u8g2.printf("%s %s %s", str_day, str_month, str_year);
  
  u8g2.sendBuffer();          // transfer internal memory to the display

  delay(60 * 1000);

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();
  //Configure Timer to wake
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  //Go to sleep now
  esp_wifi_stop();
  esp_deep_sleep_start();

}

void loop() {
  // put your main code here, to run repeatedly:

}

void GetVantageQuote(String ticker, String APIKEY) {

  //*************************************  Get Quote from API ***********************************************************
 

  HTTPClient http;
  String payload;

  http.begin("https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=" + ticker + "&apikey=" + APIKEY, root_ca); //Specify the URL and certificate
  int httpCode = http.GET();                                                  //Make the request

  if (httpCode > 0) { //Check for the returning code

    payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  }

  else {
    Serial.println("Error on HTTP request");
  }

  http.end(); //Free the resources


  // Parse JSON to Variables

  const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(10) + 750;
  DynamicJsonDocument doc(capacity);

  deserializeJson(doc, payload);

  JsonObject Global_Quote = doc["Global Quote"];
  const char* Global_Quote_01_symbol = Global_Quote["01. symbol"]; // "HPQ"
  Serial.print(Global_Quote_01_symbol);  Serial.print(": $");

  const char* Global_Quote_02_open = Global_Quote["02. open"]; // "22.8000"
  const char* Global_Quote_03_high = Global_Quote["03. high"]; // "23.4300"
  const char* Global_Quote_04_low = Global_Quote["04. low"]; // "22.7600"

  const char* Global_Quote_05_price = Global_Quote["05. price"]; // "23.2400"
  Serial.println(Global_Quote_05_price);
  strcpy(char_price, Global_Quote_05_price);

  const char* Global_Quote_06_volume = Global_Quote["06. volume"]; // "8730582"

  const char* Global_Quote_07_latest_trading_day = Global_Quote["07. latest trading day"]; // "2020-12-03"
  strcpy(char_date, Global_Quote_07_latest_trading_day);

  const char* Global_Quote_08_previous_close = Global_Quote["08. previous close"]; // "22.8200"
  const char* Global_Quote_09_change = Global_Quote["09. change"]; // "0.4200"

  const char* Global_Quote_10_change_percent = Global_Quote["10. change percent"]; // "1.8405%"
  strcpy(char_change, Global_Quote_10_change_percent);

}
