    
//"NORVI X-CPU-ESPS3-X1 -V2-N16R2"

#include <Wire.h>
#include <WiFi.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "PCA9539.h"
#include <PCA9536D.h>
#include "RTClib.h"
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include "TFT_eSPI.h"
#include <CST816S.h>
#include "Free_Fonts.h" // Include the header file attached to this sketch

PCA9539 ioport(0x75);// Base address starts at 0x74 for A0 = H and A1 = L

#define SDA   8     
#define SCL   9     

#define RS485_RXD 16
#define RS485_TXD 15
#define RS485_FC  41

#define ETH_CS 1

#define MISO 13
#define MOSI 11
#define SCLK 12

#define DSP_CS 45

#define CS4      PCA_A5 
#define CS3      PCA_A6 
#define CS2      PCA_A7 
#define CS1      PCA_B0 
#define OUTPUT19 PCA_B1
#define OUTPUT20 PCA_B2
#define OUTPUT33 PCA_B3
#define OUTPUT34 PCA_B4
#define OUTPUT38 PCA_B5

#define PORT_RST 38

#define OUTPUT5  5       // Direct GPIO
#define OUTPUT6  6
#define OUTPUT7  7
#define OUTPUT10 10
#define OUTPUT11 35     //********
#define OUTPUT12 36     // Not alowed with PSRAM
#define OUTPUT13 37     //*******
#define OUTPUT14 14
#define OUTPUT21 21
#define OUTPUT39 39
#define OUTPUT40 40

#define TX1 17
#define RX1 18

#define IO_PB1  0
#define IO_LED1 1
#define IO_LED2 2
#define IO_PB3  3

PCA9536 io;

CST816S touch(8, 9, 47, 48);  // sda, scl, rst, irq

TFT_eSPI tft = TFT_eSPI();

// SD Paramerters
#define SD_chipSelect 42

RTC_DS3231 rtc; 
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

unsigned int localPort = 8888;       // local port to listen for UDP packets

const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

EthernetUDP Udp;// A UDP instance to let us send and receive packets over UDP

unsigned long int timer1 = 0;
unsigned long int millis_start = 0;
unsigned long total = 0;
unsigned long tn = 0;


void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial2.begin(115200, SERIAL_8N1,RS485_RXD,RS485_TXD); 

  Serial.println("NPD-NORVI EXPEDITE CPU");
  delay(1000);

  Wire.begin(SDA, SCL);
  delay(100);

  I2C_SCAN();
  delay(1000);

  touch.begin();

  Serial.print(touch.data.version);
  Serial.print("\t");
  Serial.print(touch.data.versionInfo[0]);
  Serial.print("-");
  Serial.print(touch.data.versionInfo[1]);
  Serial.print("-");
  Serial.println(touch.data.versionInfo[2]);

  pinMode(PORT_RST,OUTPUT);
  pinMode(RS485_FC, OUTPUT); 
  digitalWrite(PORT_RST, HIGH);   // Enable PCA9539
  delay(100);

  
  ioport.pinMode(CS4, OUTPUT);
  ioport.pinMode(CS3, OUTPUT);
  ioport.pinMode(CS2, OUTPUT);
  ioport.pinMode(CS1, OUTPUT);
  ioport.pinMode(OUTPUT19, OUTPUT);
  ioport.pinMode(OUTPUT20, OUTPUT);
  ioport.pinMode(OUTPUT33, OUTPUT);
  ioport.pinMode(OUTPUT34, OUTPUT);
  ioport.pinMode(OUTPUT38, OUTPUT);

  pinMode(OUTPUT5,  OUTPUT);
  pinMode(OUTPUT6,  OUTPUT);
  pinMode(OUTPUT7,  OUTPUT);
  pinMode(OUTPUT10,  OUTPUT);
  pinMode(OUTPUT14,  OUTPUT);
  pinMode(OUTPUT21,  OUTPUT);
  pinMode(OUTPUT39,  OUTPUT);
  pinMode(OUTPUT40,  OUTPUT);

  if (io.begin() == false)
  {
    Serial.println("PCA9536 not detected. Please check wiring. Freezing...");
    while (1);
  }

  io.pinMode(IO_PB1, INPUT);
  io.pinMode(IO_LED1, OUTPUT);
  io.pinMode(IO_LED2, OUTPUT);
  io.pinMode(IO_PB3, INPUT);

  SPI.begin(SCLK, MISO, MOSI); // Ensure these pin numbers are correct 
  delay(1000);

  RTC_Check();
  delay(1000);

  SD_CHECK();
  delay(1000);

  ETHERNET_CHECK();     

  WiFi.mode(WIFI_MODE_APSTA);
  delay(300);
  wifi_test();

  Serial.println("TFT_eSPI library test!");
  tft.init();

  tn = micros();
  tft.fillScreen(TFT_BLACK);

  yield(); Serial.println(F("Benchmark                Time (microseconds)"));

  yield(); Serial.print(F("Screen fill              "));
  yield(); Serial.println(testFillScreen());
  //total+=testFillScreen();
  //delay(500);

  yield(); Serial.print(F("Text                     "));
  yield(); Serial.println(testText());
  //total+=testText();
  //delay(3000);

  yield(); Serial.print(F("Lines                    "));
  yield(); Serial.println(testLines(TFT_CYAN));
  //total+=testLines(TFT_CYAN);
  //delay(500);

  yield(); Serial.print(F("Horiz/Vert Lines         "));
  yield(); Serial.println(testFastLines(TFT_RED, TFT_BLUE));
  //total+=testFastLines(TFT_RED, TFT_BLUE);
  //delay(500);

  yield(); Serial.print(F("Rectangles (outline)     "));
  yield(); Serial.println(testRects(TFT_GREEN));
  //total+=testRects(TFT_GREEN);
  //delay(500);

  yield(); Serial.print(F("Rectangles (filled)      "));
  yield(); Serial.println(testFilledRects(TFT_YELLOW, TFT_MAGENTA));
  //total+=testFilledRects(TFT_YELLOW, TFT_MAGENTA);
  //delay(500);

  yield(); Serial.print(F("Circles (filled)         "));
  yield(); Serial.println(testFilledCircles(10, TFT_MAGENTA));
  //total+= testFilledCircles(10, TFT_MAGENTA);

  yield(); Serial.print(F("Circles (outline)        "));
  yield(); Serial.println(testCircles(10, TFT_WHITE));
  //total+=testCircles(10, TFT_WHITE);
  //delay(500);

  yield(); Serial.print(F("Triangles (outline)      "));
  yield(); Serial.println(testTriangles());
  //total+=testTriangles();
  //delay(500);

  yield(); Serial.print(F("Triangles (filled)       "));
  yield(); Serial.println(testFilledTriangles());
  //total += testFilledTriangles();
  //delay(500);

  yield(); Serial.print(F("Rounded rects (outline)  "));
  yield(); Serial.println(testRoundRects());
  //total+=testRoundRects();
  //delay(500);

  yield(); Serial.print(F("Rounded rects (filled)   "));
  yield(); Serial.println(testFilledRoundRects());
  //total+=testFilledRoundRects();
  //delay(500);

  yield(); Serial.println(F("Done!")); yield();

  tft.begin();
  tft.setRotation(0);

  tft.fillScreen(TFT_BLACK);            // Clear screen
  tft.setTextColor(TFT_YELLOW);
  tft.setFreeFont(FSB12);
  
  tft.setCursor(60, 20);      //xpos, ypos
  tft.print("NORVI");

  tft.setCursor(20, 40);      //xpos, ypos
  tft.print("CONTROLLERS");
}

void loop() {

  tft.fillScreen(TFT_BLACK);            // Clear screen
  tft.setTextColor(TFT_YELLOW);
  tft.setFreeFont(FSB12);
  
  tft.setCursor(60, 20);      //xpos, ypos
  tft.print("NORVI");

  tft.setCursor(20, 40);      //xpos, ypos
  tft.print("CONTROLLERS");

  tft.setTextColor(TFT_RED);
  tft.setCursor(10, 70);      //xpos, ypos
  tft.setFreeFont(FSB9);
  tft.print("Touch Test");
  

  if (touch.available()) {
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 90);      //xpos, ypos
    tft.print(touch.gesture());
    tft.print(" | ");
    Serial.print(touch.gesture());
    Serial.print("\t");
    
    tft.print(touch.data.points);
    tft.print(" | ");
    Serial.print(touch.data.points);
    Serial.print("\t");
    
    tft.print(touch.data.event);
    tft.print(" | ");
    Serial.print(touch.data.event);
    Serial.print("\t");
    
    tft.print(touch.data.x);
    tft.print(" | ");
    Serial.print(touch.data.x);
    Serial.print("\t");
    
    tft.print(touch.data.y);
    Serial.println(touch.data.y);

  }
  
  digitalWrite(RS485_FC, HIGH);                  // Make FLOW CONTROL pin HIGH       
  Serial2.println("RS485 01 SUCCESS");     // Send RS485 SUCCESS serially
  delay(500);                              // Wait for transmission of data
  digitalWrite(RS485_FC, LOW);                   // Receiving mode ON                                                 
  
  while (Serial2.available()) {  // Check if data is available
    char c = Serial2.read();     // Read data from RS485
    Serial.write(c);             // Print data on serial monitor
  }
  delay(1000);

  tft.setTextColor(TFT_RED);
  tft.setCursor(10, 130);      //xpos, ypos
  tft.print("IO TEST");
  
  Serial.println("IO TEST");
  io.digitalWrite(IO_LED1, LOW); io.digitalWrite(IO_LED2, HIGH); 
  delay(500);
  io.digitalWrite(IO_LED1, HIGH); io.digitalWrite(IO_LED2, LOW);
  delay(500);
  io.digitalWrite(IO_LED1, HIGH); io.digitalWrite(IO_LED2, HIGH);
  Serial.print("PB1 Read: "); Serial.println(io.digitalRead(IO_PB1));
  Serial.print("PB2 Read: "); Serial.println(io.digitalRead(IO_PB3));

  // Turn off all the IO ports
//  digitalWrite(OUTPUT1,LOW); digitalWrite(OUTPUT2,LOW); digitalWrite(OUTPUT4,LOW); 
  digitalWrite(OUTPUT5,LOW); digitalWrite(OUTPUT6,LOW);
  digitalWrite(OUTPUT7,LOW); digitalWrite(OUTPUT10,LOW); digitalWrite(OUTPUT14,LOW);
  ioport.digitalWrite(OUTPUT19,LOW); ioport.digitalWrite(OUTPUT20,LOW); digitalWrite(OUTPUT21,LOW);
  ioport.digitalWrite(OUTPUT33,LOW); ioport.digitalWrite(OUTPUT34,LOW); ioport.digitalWrite(OUTPUT38,LOW);
  digitalWrite(OUTPUT39,LOW); digitalWrite(OUTPUT40,LOW);

  ioport.digitalWrite(CS1,LOW); ioport.digitalWrite(CS2,LOW); ioport.digitalWrite(CS3,LOW); ioport.digitalWrite(CS4,LOW);
  delay(1000);
  
  digitalWrite(OUTPUT5,HIGH);
  delay(100);
  digitalWrite(OUTPUT6,HIGH);
  delay(100);
  digitalWrite(OUTPUT7,HIGH);
  delay(100);
  digitalWrite(OUTPUT10,HIGH);
  delay(100);
  digitalWrite(OUTPUT14,HIGH);
  delay(100);
  ioport.digitalWrite(OUTPUT19,HIGH);
  delay(100);
  ioport.digitalWrite(OUTPUT20,HIGH);
  delay(100);
  digitalWrite(OUTPUT21,HIGH);
  delay(100);
  ioport.digitalWrite(OUTPUT33,HIGH);
  delay(100);
  ioport.digitalWrite(OUTPUT34,HIGH);
  delay(100);
  ioport.digitalWrite(OUTPUT38,HIGH);
  delay(100);
  digitalWrite(OUTPUT39,HIGH);
  delay(100);
  digitalWrite(OUTPUT40,HIGH);
  delay(100);

  ioport.digitalWrite(CS1,HIGH);
  delay(100);
  ioport.digitalWrite(CS2,HIGH);
  delay(100);
  ioport.digitalWrite(CS3,HIGH);
  delay(100);
  ioport.digitalWrite(CS4,HIGH);
  delay(100);
  
  
}

void I2C_SCAN() {
  byte error, address;
  int deviceCount = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);
      Serial.println("  !");

      deviceCount++;
      delay(1);  // Wait for a moment to avoid overloading the I2C bus
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }

  if (deviceCount == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("Scanning complete\n");
  }
}

void SD_CHECK(){
  Serial.print("\nInitializing SD card...");
  uint8_t cardType = SD.cardType();

  if(SD.begin(SD_chipSelect))
 {
  Serial.println("Card Mount: success");
  Serial.print("Card Type: ");

    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("Unknown");
    }

  int cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("Card Size: %lluMB\n", cardSize);
  }

  if(!SD.begin(SD_chipSelect))
  {
  Serial.println("NO SD card");            
  }
}

void displayTime(void) {
  DateTime now = rtc.now();
     
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);

  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  delay(1000);

}

void RTC_Check(){
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
 else{
 if (rtc.lostPower()) {
  
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
  }

   
  int a=1;
  while(a<6)
  {
  displayTime();   // printing time function for oled
  a=a+1;
  }
 }
}

void ETHERNET_CHECK(){
  Ethernet.init(ETH_CS);  // CS PIN OF THE ETHERNET

  // start Ethernet and UDP
  if (Ethernet.begin(mac) == 0) {
  Serial.println("Failed to configure Ethernet using DHCP");

    // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
  Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
  }
  
  if (Ethernet.linkStatus() == LinkOFF) {
   Serial.println("Ethernet cable is not connected.");
  }
  }
  
 else{
  Udp.begin(localPort);
 
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = ");
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
   Serial.print("The UTC time is ");
   Serial.print((epoch  % 86400L) / 3600);
    // print the hour (86400 equals secs per day)
   Serial.print(':');
   if (((epoch % 3600) / 60) < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
   }
    
   Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
   Serial.print(':');
   if ((epoch % 60) < 10) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
   }
   
   Serial.println(epoch % 60); // print the second
   }
  // wait ten seconds before asking for the time again
  delay(3000);
  Ethernet.maintain();
 }
}


void sendNTPpacket(const char * address) 
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
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket(); 
}

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(TFT_BLACK);
  tft.fillScreen(TFT_RED);
  tft.fillScreen(TFT_GREEN);
  tft.fillScreen(TFT_BLUE);
  tft.fillScreen(TFT_BLACK);
  return micros() - start;
}

unsigned long testText() {
  tft.fillScreen(TFT_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(TFT_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  //tft.setTextColor(TFT_GREEN,TFT_BLACK);
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}

unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();

  tft.fillScreen(TFT_BLACK);

  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing

  tft.fillScreen(TFT_BLACK);

  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(TFT_BLACK);

  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;

  tft.fillScreen(TFT_BLACK);

  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for (y2 = 0; y2 < h; y2 += 6) tft.drawLine(x1, y1, x2, y2, color);

  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = tft.width(), h = tft.height();

  tft.fillScreen(TFT_BLACK);
  start = micros();
  for (y = 0; y < h; y += 5) tft.drawFastHLine(0, y, w, color1);
  for (x = 0; x < w; x += 5) tft.drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = tft.width()  / 2,
                cy = tft.height() / 2;

  tft.fillScreen(TFT_BLACK);
  n     = min(tft.width(), tft.height());
  start = micros();
  for (i = 2; i < n; i += 6) {
    i2 = i / 2;
    tft.drawRect(cx - i2, cy - i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  n = min(tft.width(), tft.height());
  for (i = n - 1; i > 0; i -= 6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx - i2, cy - i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx - i2, cy - i2, i, i, color2);
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(TFT_BLACK);
  start = micros();
  for (x = radius; x < w; x += r2) {
    for (y = radius; y < h; y += r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                      w = tft.width()  + radius,
                      h = tft.height() + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for (x = 0; x < w; x += r2) {
    for (y = 0; y < h; y += r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = tft.width()  / 2 - 1,
                      cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  n     = min(cx, cy);
  start = micros();
  for (i = 0; i < n; i += 5) {
    tft.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      tft.color565(0, 0, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = tft.width()  / 2 - 1,
                   cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  start = micros();
  for (i = min(cx, cy); i > 10; i -= 5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     tft.color565(0, i, i));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                     tft.color565(i, i, 0));
  }

  return t;
}

unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  w     = min(tft.width(), tft.height());
  start = micros();
  for (i = 0; i < w; i += 6) {
    i2 = i / 2;
    tft.drawRoundRect(cx - i2, cy - i2, i, i, i / 8, tft.color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(TFT_BLACK);
  start = micros();
  for (i = min(tft.width(), tft.height()); i > 20; i -= 6) {
    i2 = i / 2;
    tft.fillRoundRect(cx - i2, cy - i2, i, i, i / 8, tft.color565(0, i, 0));
  }

  return micros() - start;
}

void wifi_test(){
  Serial.println("");
  String str_macAddress;
  byte mac[6];

  WiFi.macAddress(mac);

   str_macAddress = (String(mac[0] >> 4,  HEX) + String(mac[0] & 0x0F, HEX)) + (":") +
                   (String(mac[1] >> 4,  HEX) + String(mac[1] & 0x0F, HEX)) + (":") +
                   (String(mac[2] >> 4,  HEX) + String(mac[2] & 0x0F, HEX)) + (":") +
                   (String(mac[3] >> 4,  HEX) + String(mac[3] & 0x0F, HEX)) + (":") +
                   (String(mac[4] >> 4,  HEX) + String(mac[4] & 0x0F, HEX)) + (":") +
                   (String(mac[5] >> 4,  HEX) + String(mac[5] & 0x0F, HEX));
  str_macAddress.toUpperCase();
  
  Serial.println("MAC Address: " + str_macAddress);

  String ssid =  str_macAddress; 
  const char* password = "12345678";   

  Serial.println("Setting up the Wi-Fi Access Point...");
  WiFi.softAP(ssid.c_str(), password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP address: ");
  Serial.println(IP);

  Serial.print("Access Point SSID: ");
  Serial.println(ssid);

  Serial.println("Wi-Fi Access Point is active!");
  Serial.println("");
}