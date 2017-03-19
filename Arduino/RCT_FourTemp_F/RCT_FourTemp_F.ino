/*
  -----------------------------------------------------------
                Jeti FourTemp Sensor v 1.1
  -----------------------------------------------------------

   Tero Salminen RC-Thoughts.com (c) 2017 www.rc-thoughts.com

  -----------------------------------------------------------
  
                    Fahrenheit version

        Temperature sensor for 4 separate sensors
      Uses Dallas DS18S20 and Arduino Pro Mini 3.3V 8Mhz

  -----------------------------------------------------------
      Shared under MIT-license by Tero Salminen (c) 2017
  -----------------------------------------------------------
*/

#include <EEPROM.h>
#include <stdlib.h>
#include <SoftwareSerialJeti.h>
#include <JETI_EX_SENSOR.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Wire.h"

#define ONE_WIRE_BUS_1 6
#define ONE_WIRE_BUS_2 7
#define ONE_WIRE_BUS_3 8
#define ONE_WIRE_BUS_4 9
OneWire oneWire_1(ONE_WIRE_BUS_1);
OneWire oneWire_2(ONE_WIRE_BUS_2);
OneWire oneWire_3(ONE_WIRE_BUS_3);
OneWire oneWire_4(ONE_WIRE_BUS_4);
DallasTemperature sensor_1(&oneWire_1);
DallasTemperature sensor_2(&oneWire_2);
DallasTemperature sensor_3(&oneWire_3);
DallasTemperature sensor_4(&oneWire_4);

#define prog_char  char PROGMEM
#define GETCHAR_TIMEOUT_ms 20

#ifndef JETI_RX
#define JETI_RX 3
#endif

#ifndef JETI_TX
#define JETI_TX 4
#endif

#define ITEMNAME_1 F("Temp 1")
#define ITEMTYPE_1 F("\xB0\x46")
#define ITEMVAL_1 (volatile float*)&uTemp1

#define ITEMNAME_2 F("Temp 2")
#define ITEMTYPE_2 F("\xB0\x46")
#define ITEMVAL_2 (volatile float*)&uTemp2

#define ITEMNAME_3 F("Temp 3")
#define ITEMTYPE_3 F("\xB0\x46")
#define ITEMVAL_3 (volatile float*)&uTemp3

#define ITEMNAME_4 F("Temp 4")
#define ITEMTYPE_4 F("\xB0\x46")
#define ITEMVAL_4 (volatile float*)&uTemp4

#define ABOUT_1 F(" RCT Jeti Tools")
#define ABOUT_2 F("  RCT FourTemp")

SoftwareSerial JetiSerial(JETI_RX, JETI_TX);

void JetiUartInit()
{
  JetiSerial.begin(9700);
}

void JetiTransmitByte(unsigned char data, boolean setBit9)
{
  JetiSerial.set9bit = setBit9;
  JetiSerial.write(data);
  JetiSerial.set9bit = 0;
}

unsigned char JetiGetChar(void)
{
  unsigned long time = millis();
  while ( JetiSerial.available()  == 0 )
  {
    if (millis() - time >  GETCHAR_TIMEOUT_ms)
      return 0;
  }
  int read = -1;
  if (JetiSerial.available() > 0 )
  { read = JetiSerial.read();
  }
  long wait = (millis() - time) - GETCHAR_TIMEOUT_ms;
  if (wait > 0)
    delay(wait);
  return read;
}

char * floatToString(char * outstr, float value, int places, int minwidth = 0) {
  int digit;
  float tens = 0.1;
  int tenscount = 0;
  int i;
  float tempfloat = value;
  int c = 0;
  int charcount = 1;
  int extra = 0;
  float d = 0.5;
  if (value < 0)
    d *= -1.0;
  for (i = 0; i < places; i++)
    d /= 10.0;
  tempfloat +=  d;
  if (value < 0)
    tempfloat *= -1.0;
  while ((tens * 10.0) <= tempfloat) {
    tens *= 10.0;
    tenscount += 1;
  }
  if (tenscount > 0)
    charcount += tenscount;
  else
    charcount += 1;
  if (value < 0)
    charcount += 1;
  charcount += 1 + places;
  minwidth += 1;
  if (minwidth > charcount) {
    extra = minwidth - charcount;
    charcount = minwidth;
  }
  if (value < 0)
    outstr[c++] = '-';
  if (tenscount == 0)
    outstr[c++] = '0';
  for (i = 0; i < tenscount; i++) {
    digit = (int) (tempfloat / tens);
    itoa(digit, &outstr[c++], 10);
    tempfloat = tempfloat - ((float)digit * tens);
    tens /= 10.0;
  }
  if (places > 0)
    outstr[c++] = '.';
  for (i = 0; i < places; i++) {
    tempfloat *= 10.0;
    digit = (int) tempfloat;
    itoa(digit, &outstr[c++], 10);
    tempfloat = tempfloat - (float) digit;
  }
  if (extra > 0 ) {
    for (int i = 0; i < extra; i++) {
      outstr[c++] = ' ';
    }
  }
  outstr[c++] = '\0';
  return outstr;
}

JETI_Box_class JB;

unsigned char SendFrame()
{
  boolean bit9 = false;
  for (int i = 0 ; i < JB.frameSize ; i++ )
  {
    if (i == 0)
      bit9 = false;
    else if (i == JB.frameSize - 1)
      bit9 = false;
    else if (i == JB.middle_bit9)
      bit9 = false;
    else
      bit9 = true;
    JetiTransmitByte(JB.frame[i], bit9);
  }
}

unsigned char DisplayFrame()
{
  for (int i = 0 ; i < JB.frameSize ; i++ )
  {
  }
}

uint8_t frame[10];
short value = 27;

float uTemp1 = 0;
float uTemp2 = 0;
float uTemp3 = 0;
float uTemp4 = 0;


#define MAX_SCREEN 3
#define MAX_CONFIG 1
#define COND_LES_EQUAL 1
#define COND_MORE_EQUAL 2

void setup()
{
  Serial.begin(9600);
  analogReference(EXTERNAL);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  sensor_1.begin();
  sensor_2.begin();
  sensor_3.begin();
  sensor_4.begin();

  pinMode(JETI_RX, OUTPUT);
  JetiUartInit();

  JB.JetiBox(ABOUT_1, ABOUT_2);
  JB.Init(F("4-Temp"));
  JB.addData(ITEMNAME_1, ITEMTYPE_1);
  JB.addData(ITEMNAME_2, ITEMTYPE_2);
  JB.addData(ITEMNAME_3, ITEMTYPE_3);
  JB.addData(ITEMNAME_4, ITEMTYPE_4);
  JB.setValue(1, ITEMVAL_1, 1);
  JB.setValue(2, ITEMVAL_2, 1);
  JB.setValue(3, ITEMVAL_3, 1);
  JB.setValue(4, ITEMVAL_4, 1);
  
  do {
    JB.createFrame(1);
    SendFrame();
    delay(GETCHAR_TIMEOUT_ms);
  }
  while (sensorFrameName != 0);
  digitalWrite(13, LOW);

}

float alarm_current = 0;

int header = 0;
int lastbtn = 240;
int current_screen = 0;
int current_config = 0;
char temp[LCDMaxPos / 2];
char msg_line1[LCDMaxPos / 2];
char msg_line2[LCDMaxPos / 2];

void process_screens()  
{
  switch (current_screen)
  {
  case 0 : {
        JB.JetiBox(ABOUT_1, ABOUT_2);
        break;
      }
  case 1 : {
        msg_line1[0] = 0; msg_line2[0] = 0;

        strcat_P((char*)&msg_line1, (prog_char*)F("T1:"));
        temp[0] = 0;
        floatToString((char*)&temp, uTemp1, 0);
        strcat((char*)&msg_line1, (char*)&temp);        
        strcat_P((char*)&msg_line1, (prog_char*)F("\xB0\x46 "));
        
        strcat_P((char*)&msg_line1, (prog_char*)F("T2:"));
        temp[0] = 0;
        floatToString((char*)&temp, uTemp2, 0);
        strcat((char*)&msg_line1, (char*)&temp);        
        strcat_P((char*)&msg_line1, (prog_char*)F("\xB0\x46"));

        strcat_P((char*)&msg_line2, (prog_char*)F("T3:"));
        temp[0] = 0;
        floatToString((char*)&temp, uTemp3, 0);
        strcat((char*)&msg_line2, (char*)&temp);
        strcat_P((char*)&msg_line2, (prog_char*)F("\xB0\x46 "));
        
        strcat_P((char*)&msg_line2, (prog_char*)F("T4:"));
        temp[0] = 0;
        floatToString((char*)&temp, uTemp4, 0);
        strcat((char*)&msg_line2, (char*)&temp);
        strcat_P((char*)&msg_line2, (prog_char*)F("\xB0\x46"));

        JB.JetiBox((char*)&msg_line1, (char*)&msg_line2);
        break;
      }
  case MAX_SCREEN : {
        JB.JetiBox(ABOUT_1, ABOUT_2);
        break;
      }
  }
}

void loop()
{   
    sensor_1.requestTemperaturesByIndex(0);
    sensor_2.requestTemperaturesByIndex(0);
    sensor_3.requestTemperaturesByIndex(0);
    sensor_4.requestTemperaturesByIndex(0);
    uTemp1 = sensor_1.getTempFByIndex(0);
    uTemp2 = sensor_2.getTempFByIndex(0);
    uTemp3 = sensor_3.getTempFByIndex(0);
    uTemp4 = sensor_4.getTempFByIndex(0);
  
    if (uTemp1 < -190) {
        uTemp1 = 0;
    }
    
    if (uTemp2 < -190) {
        uTemp2 = 0;
    }
    
    if (uTemp3 < -190) {
        uTemp3 = 0;
    }
    
    if (uTemp4 < -190) {
        uTemp4 = 0;
    }

  /*Serial.print("T1: ");Serial.print(uTemp1); // Uncomment these for PC debug
  Serial.print(" T2: ");Serial.print(uTemp2);
  Serial.print(" T3: ");Serial.print(uTemp3);
  Serial.print(" T4: ");Serial.println(uTemp4);
  Serial.println();*/

  unsigned long time = millis();
  SendFrame();
  time = millis();
  int read = 0;
  pinMode(JETI_RX, INPUT);
  pinMode(JETI_TX, INPUT_PULLUP);

  JetiSerial.listen();
  JetiSerial.flush();

  while ( JetiSerial.available()  == 0 )
  {

    if (millis() - time >  5)
      break;
  }

  if (JetiSerial.available() > 0 )
  { read = JetiSerial.read();

    if (lastbtn != read)
    {
      lastbtn = read;
      switch (read)
      {
        case 224 : // RIGHT
          if (current_screen != MAX_SCREEN)
          {
            current_screen++;
            if (current_screen == 2) current_screen = 0;
          }
          break;
        case 112 : // LEFT
          if (current_screen != MAX_SCREEN)
          if (current_screen == 2) current_screen = 1;
          else
          {
            current_screen--;
            if (current_screen > MAX_SCREEN) current_screen = 0;
          }
          break;
        case 208 : // UP
          break;
        case 176 : // DOWN
          break;
        case 144 : // UP+DOWN
          break;
        case 96 : // LEFT+RIGHT
          break;
      }
    }
  }

  if (current_screen != MAX_SCREEN)
    current_config = 0;
  process_screens();
  header++;
  if (header >= 5)
  {
    JB.createFrame(1);
    header = 0;
  }
  else
  {
    JB.createFrame(0);
  }

  long wait = GETCHAR_TIMEOUT_ms;
  long milli = millis() - time;
  if (milli > wait)
    wait = 0;
  else
    wait = wait - milli;
  pinMode(JETI_TX, OUTPUT);
}

