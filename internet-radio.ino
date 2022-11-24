/**
 * @file internet-radio.ino
 * @author David Baker
 * @brief Internet Radio appliance
 * @version 0.1
 * @date 2022-11-24
 * 
 */

// install https://github.com/pschatzmann/arduino-libhelix.git

// Audio includes
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include "AudioCodecs/CodecAACHelix.h"
//#include "AudioCodecs/CodecHelix.h"


// Display includes
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SSD1306_NO_SPLASH

#define BUTTON_STANEXT 19
#define BUTTON_STATUNE 18

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Display update timer
unsigned long lastDispUpdTime = millis();

ICYStream url("SafetyPinsSaveLives","Couchpepfluxoustsponge!");
AnalogAudioStream analog; // final output of decoded stream
EncodedAudioStream decMp3(&analog, new MP3DecoderHelix()); // Decoding stream
EncodedAudioStream decAac(&analog, new AACDecoderHelix()); // Decoding stream

StreamCopy copier(decAac, url); // copy url to decoder
StreamCopy copierMp3(decMp3, url); // copy url to decoder


// MP3=1, AAC=0
int isMp3 = 1;

int cursorY = 0;

// flag to update display. Checked during loop.
int updateDisplay = 0;

// state for NEXT STATION button
int btnNextLast = LOW;
int btnNextCurrent = HIGH;

// state for TUNE TO STATION button
int btnTuneLast = LOW;
int btnTuneCurrent = HIGH;

int btnChangeLastState = LOW;
int currentState = HIGH;

int station = 0; // Station Index

const int numStations = 8;
const char* stations[numStations][3] = {
  //{"http://wpwt.streamon.fm:8000/WPWT-24k.aac","audio/aac","WPWT - The Possum"},
  //{"http://wiyd.streamon.fm:8000/WIYD-24k.aac","audio/aac","WIYD Country"},
  //{"http://ih-streaming.cmgdigital.com/san680/san680-iheart.aac","audio/acc","KKYX 680 Country"},
  //{"http://stream.revma.ihrhls.com/zc4435","audio/acc","iHeart Country Classics"},
  //{"http://stream.nightride.fm/nightride.m4a","audio/aac","Nightride.FM Synthwave"},
  {"http://usa17.fastcast4u.com:5464/;","audio/mp3","KGID-FM Classic Country"},
  {"http://s7.reliastream.com:8194/stream","audio/mp3","AM 1630 Classic Country"},
  {"http://s9.streammonster.com:8012/;","audio/mp3","1940s Radio UK"},
  {"http://s2.voscast.com:11688/;","audio/mp3","Cheap Music 20s-50s"},
  {"http://streaming.rubinbroadcasting.com/kcea","audio/mp3","KCEA-FM Big Band"},
  {"http://stream.laut.fm/nightdrive","audio/mp3","Nightdrive - Germany"},
  {"http://cassini.shoutca.st:8574/;","audio/mp3","Grateful Dead - Slovenia"},
  {"http://108.163.245.230:8100/stream","audio/mp3","GDRadio.net"},
  {"http://108.163.245.230:8100/stream","audio/mp3","GDRadio.net"}
};

// Rotary Stuff
const int pinA = 13;
const int pinB = 12;
int lastA = 0;
int lastB = 0;
int readA;
int readB;
hw_timer_t *My_timer = NULL;


void printStationInfo() {
  display.clearDisplay(); 
  cursorY = 30;
  display.setCursor(0, cursorY);
  display.println(stations[station][2]);
  display.display();      
  cursorY = display.getCursorY() + 5;
}

void chkRotary() {
  readA = digitalRead(pinA);
  readB = digitalRead(pinB);
  if (readA != lastA) {
    if (readA == 1 && readB == 0) {
      if (station < numStations-1)
        station++;
      else
       station = 0;
     Serial.println(station);
     updateDisplay = 1;
    }
    /*Serial.print("A: "); Serial.println(readA); Serial.print("B: "); Serial.println(readB); Serial.println("");*/
    lastA = readA;
  }
  if (readB != lastB) {
    if (readA == 0 && readB == 1) {
      if (station > 0)
        station--;
      else
       station = numStations-1;
      Serial.println(station);
      updateDisplay = 1; 
    }
    /*Serial.print("A: "); Serial.println(readA); Serial.print("B: "); Serial.println(readB); Serial.println("");*/
    lastB = readB;
  }
}

// Timer Interrupt routine
void IRAM_ATTR onTimer() {
  chkRotary();
}

void setup(){
  Serial.begin(115200);
  //AudioLogger::instance().begin(Serial, AudioLogger::Info);  

  pinMode(BUTTON_STANEXT, INPUT_PULLUP); // show next station
  pinMode(BUTTON_STATUNE, INPUT_PULLUP); // tune to station

  // setup analog
  auto config = analog.defaultConfig(TX_MODE);
  analog.begin(config);

  // setup I2S based on sampling rate provided by decoder
  decMp3.setNotifyAudioChange(analog);
  decMp3.begin();

   // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay(); 
  cursorY = 0;
  display.setTextSize(1); // Draw 1X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("LOADING..."));
  display.display();      

  // mp3 radio
  url.begin(stations[station][0], stations[station][1]);
  printStationInfo();
  
  // Rotary switch
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);

  // set up interrupt timer to check rotorary switch
  My_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(My_timer, &onTimer, true);
  timerAlarmWrite(My_timer, 7813, true); // how often in microseconds 15625
  timerAlarmEnable(My_timer);
}



void checkBtnNext() {
 btnNextCurrent = digitalRead(BUTTON_STANEXT);
 if (/*btnNextLast == HIGH ) &&*/ btnNextCurrent == LOW) {
    Serial.println("NEXT STATION button is pressed");
    if (station < numStations-1)
      station++;
    else
      station = 0;
    printStationInfo();
  }
  btnNextLast = btnNextCurrent; 
}

void checkBtnTune() {
   btnTuneCurrent = digitalRead(BUTTON_STATUNE);
   if (btnTuneLast == HIGH && btnTuneCurrent == LOW) {
    Serial.println("TUNE TO STATION button is pressed");
    url.end();
    analog.end();
    //decAac.end();
    //decMp3.end();

    if (stations[station][2] == "audio/aac") {
      
      isMp3=0;
    }
    else if (stations[station][2] == "audio/mp3") {
      EncodedAudioStream decMp3(&analog, new MP3DecoderHelix()); // Decoding stream
      copier.end();
      
      
      
      decMp3.setNotifyAudioChange(analog);
      //decAac.end();
      decMp3.begin();
      isMp3=1;
    }
    
    url.begin(stations[station][0], stations[station][1]);
    analog.begin();

  }
  btnTuneLast = btnTuneCurrent; 
}



void loop(){
  if (isMp3)
    copierMp3.copy();
  else
    copier.copy();

  // update display every 350ms                           TODO: update only when screen changes
 /* if (millis() - lastDispUpdTime >= 1000) {
    printStationInfo();
    lastDispUpdTime = millis();
  }*/

  if (updateDisplay) {
    printStationInfo();
    updateDisplay = 0;
  }

  // Check if buttons pressed
 // checkBtnNext();
  checkBtnTune();

 


  
}
