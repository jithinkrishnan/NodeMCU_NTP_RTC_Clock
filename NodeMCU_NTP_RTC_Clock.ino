/************************************************************************
*   ESP8266 NTP - DS3231 RTC HW-069 Display Time sync
*   File:   NodeMCU_NTP_RTC_Clock.ino
*   Arduino IDE Ver 2.1.0 
*   Author:  Jithin Krishnan.K
*       Rev. 2.0 : 11/06/2023 :  17:49 PM
* 
* This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Email: jithinkrishnan.k@gmail.com
*   
************************************************************************/

#include <Wire.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <Time.h>
#include <TimeLib.h>
#include <TM1637Display.h>
#include <DS3231.h>

//#define SOFT_SDA  4  // D2    
//#define SOFT_SCL  5 //  D1
#define CLK 2       //  D4
#define DIO 0       //  D3

const uint8_t SEG_SYNC[] = {
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,           // S
  SEG_F | SEG_G | SEG_B | SEG_C | SEG_D,           // y
  SEG_E | SEG_F | SEG_A | SEG_B | SEG_C,           // n
  SEG_A | SEG_F | SEG_E | SEG_D                    // C
  };

DS3231 clk;
RTCDateTime dt, ist_dt;
TM1637Display display(CLK, DIO);

char ssid[] = "Krishna";    // SSID
char pass[] = "12345678";;   // Wifi Password
unsigned int localPort = 2390;

IPAddress timeServerIP; 
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];

char do_once = 0, cnt = 0;
uint8_t data[] = { 0x00, 0x00, 0x00, 0x00 };

WiFiUDP udp;

void setup()
{
  clk.begin(); // In NodeMCU, SDA=4, SCL=5  
  display.setBrightness(0x0f);
}

void loop()
{
    dt = clk.getDateTime();
    display.showNumberDecEx(dt.minute, 0, true,2 ,2);
    display.showNumberDecEx(dt.hour, 0b01000000, true,2 ,0);
   
    if (dt.hour == 12 && dt.minute == 15 && dt.second == 0){
      do_once = 0;
    }

  if (!do_once) {
      display.setSegments(SEG_SYNC); 
      WiFi.begin(ssid, pass);
      while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      cnt++;
      if (cnt > 20) {
         cnt = 0;
         do_once = -1;
         goto X;
      }
    }
  
    udp.begin(localPort);
    WiFi.hostByName(ntpServerName, timeServerIP);
    sendNTPpacket(timeServerIP);
    delay(1000);

    int cb = udp.parsePacket();
  
    if (cb) {
        udp.read(packetBuffer, NTP_PACKET_SIZE);
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        const unsigned long seventyYears = 2208988800UL;
        unsigned long epoch = secsSince1900 - seventyYears;
        epoch += 19800UL; // GMT+5:30
        
        ist_dt.hour   = hour(epoch);
        ist_dt.minute = minute(epoch);
        ist_dt.second = second(epoch);
        ist_dt.year   = year(epoch);
        ist_dt.month  = month(epoch);
        ist_dt.day    = day(epoch);
    
        dt = clk.getDateTime(); 
    
    if((ist_dt.year != dt.year || ist_dt.month != dt.month || ist_dt.day != dt.day ||
        ist_dt.hour != dt.hour) || (ist_dt.minute != dt.minute) || (ist_dt.second != dt.second))  {
        clk.setDateTime(ist_dt.year, ist_dt.month, ist_dt.day, ist_dt.hour, ist_dt.minute, ist_dt.second);
    }
   }  
    WiFi.disconnect();
    do_once = -1;
  }    
  X:
      delay(800);

      return;
}


unsigned long sendNTPpacket(IPAddress& address) {
   memset(packetBuffer, 0, NTP_PACKET_SIZE);
   packetBuffer[0] = 0b11100011;   // LI, Version, Mode
   packetBuffer[1] = 0;     // Stratum, or type of clock
   packetBuffer[2] = 6;     // Polling Interval
   packetBuffer[3] = 0xEC;  // Peer Clock Precision
   packetBuffer[12]  = 49;
   packetBuffer[13]  = 0x4E;
   packetBuffer[14]  = 49;
   packetBuffer[15]  = 52;
   udp.beginPacket(address, 123); //NTP requests are to port 123
   udp.write(packetBuffer, NTP_PACKET_SIZE);
   udp.endPacket();

   return 0;
}
