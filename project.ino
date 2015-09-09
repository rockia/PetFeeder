/*************************************************** 

Introduction: 
This Arduino sketch is for Pet Feeder project for ENSC 100/100 
for Fall 2013 at Simon Fraser University.

This project uses:
1. Arduino Uno R3
2. Adafruit CC3000 Wifi Shield
3. Belkin Wemo Smart controller
4. Self-hosted API server to handle signal
5. IFTTT to handle SMS


**************************************************/
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin! default 3
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5  //Default: 5
#define ADAFRUIT_CC3000_CS    10

// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, 
                                        ADAFRUIT_CC3000_IRQ, 
                                        ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2); 

#define WLAN_SSID       "SSID_name"           // The network SSID Arduino connects to
#define WLAN_PASS       "WLAN_Password"       // The password for wireless lan
#define WLAN_SECURITY   WLAN_SEC_WPA2         // Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  1000      // Default value: 3000 
                                   //Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "www.server.com"           //  API Server
#define WEBPAGE      "/index.php?token=D125"    //  API page and receive token


uint32_t ip;

void setup(void)
{
 // Sensor
 
 Serial.begin(9600);
 pinMode(8, OUTPUT);

 while (! Serial);

 Serial.println("Speed 0 to 255");

}

void loop(void)
{
   int sensor, inches, x;
  
  // read the analog output of the EZ1 from analog input 0
  sensor = analogRead(0);
  
  // convert the sensor reading to inches
  inches = sensor / 2;
  
  // print out the decimal result
  Serial.print("Distance in inches: ");
  Serial.print(inches,DEC);
  
  Serial.print("\r\n");
  
  if (inches >15) {
    notifyUser();
  }

  // pause before taking the next reading
  delay(1000);                  
}


void notifyUser(){
  Serial.println(F("Hello, D125 Group San!\n")); 
   /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  // Optional SSID scan
   //listSSIDResults();
  
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  /* Display the IP address DNS, Gateway, etc. */  
  while (! displayConnectionDetails()) {
    delay(1000);
  }

  ip = 0;
  // Try looking up the website's IP address
  Serial.print(WEBSITE); Serial.print(F(" -> "));
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }

  cc3000.printIPdotsRev(ip);
  
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
     www.fastrprint(F("GET "));
    www.fastrprint(WEBPAGE);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));    
    return;
  }

  Serial.println(F("\r\n-------------------------------------\r\nResult from HTTP Server:"));
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
  Serial.println(F("\r\n-------------------------------------"));
  
  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
  cc3000.disconnect();
}

/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.println();
    return true;
  }
}
