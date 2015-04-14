/*
Copyright (c) 2015 PTC Inc. 
Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software 
is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies 
or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <Dhcp.h>
#include <Dns.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <twApi.h>
#include <twLogger.h>
#include <twOSPort.h>
#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include "HTU21D.h"
HTU21D myHumidity;

/* Mac address of the network adapter */
//byte mac[] =   { 
//  0x98, 0x4F, 0xEE, 0x01, 0x85, 0x71 };

/* Name of your thing */
char * thingName = "HTU21DThing";
/* IP/hostname of your TWX server */
char * serverName = "thingworx-academic-staff.ptcmscloud.com";
/* port */
int port = 80;
/* API key */
char * apiKey = "fac4baad-bee8-466d-bae2-6569ee4938cd";
/* refresh rate */
int timeBetweenRefresh = 1000;

/* Hold all the properties */
struct  {
  double Temperature;
  double Humidity;
} 
properties;



void sendPropertyUpdate() {
  /* Create the property list */
 propertyList * proplist = twApi_CreatePropertyList("Temperature",twPrimitive_CreateFromNumber(properties.Temperature), 0);
	if (!proplist) {
		TW_LOG(TW_ERROR,"sendPropertyUpdate: Error allocating property list");
		return;
	}
  twApi_AddPropertyToList(proplist,"Humidity",twPrimitive_CreateFromNumber(properties.Humidity), 0);
  twApi_PushProperties(TW_THING, thingName, proplist, -1, FALSE);
  twApi_DeletePropertyList(proplist);
}

void dataCollectionTask() {
   properties.Humidity = myHumidity.readHumidity();
   properties.Temperature = myHumidity.readTemperature();

  Serial.print("Time:");
  Serial.print(millis());
  Serial.print(" Temperature:");
  Serial.print(properties.Temperature, 1);
  Serial.print("C");
  Serial.print(" Humidity:");
  Serial.print(properties.Humidity, 1);
  Serial.print("%");
  Serial.println();
  /* Update the properties on the server */
  sendPropertyUpdate();
}



/*****************
 * Property Handler Callbacks 
 ******************/
enum msgCodeEnum propertyHandler(const char * entityName, const char * propertyName,  twInfoTable ** value, char isWrite, void * userdata) {
  char * asterisk = "*";
  if (!propertyName) propertyName = asterisk;
  TW_LOG(TW_TRACE,"propertyHandler - Function called for Entity %s, Property %s", entityName, propertyName);
  if (value) {
    
      /* Property Reads */
      if (strcmp(propertyName, "Temperature") == 0) *value = twInfoTable_CreateFromNumber(propertyName, properties.Temperature);
      else if (strcmp(propertyName, "Humidity") == 0) *value = twInfoTable_CreateFromNumber(propertyName, properties.Humidity);      
      else return TWX_NOT_FOUND;
    return TWX_SUCCESS;
  } 
  else {
    TW_LOG(TW_ERROR,"propertyHandler - NULL pointer for value");
    return TWX_BAD_REQUEST;
  }
}

void setup() {
  int err=0;
  /* Open serial connection */
  Serial.begin(9600);
  /* Wait for someone to launch the console */
  delay(3000);

  /* Setup the ethernet connection */
  Serial.println("Attempting to start Wireless0");
  //if (Ethernet.begin(mac) == 0) {
 //   Serial.println("Failed to configure Ethernet using DHCP");
//  } 
//  else {
//    Serial.println("Sucess getting dhcp address");
//  }
  /* start adapter */
  system("ifup wlan0");
  /* wait for it to start */
  delay(1000);
// system("ifconfig > /dev/ttyGS0");
  system("telnetd -l /bin/sh");
  Serial.println("HTU21D Example!");
  myHumidity.begin();

  Serial.print("Initialize TWX: ");
  err = twApi_Initialize(serverName, port, TW_URI, apiKey, NULL, MESSAGE_CHUNK_SIZE, MESSAGE_CHUNK_SIZE, TRUE);
  if (err) {
    Serial.println("Error initializing the API");
  }

  /* Allow self signed certs */
  twApi_SetSelfSignedOk();

  /* Regsiter our properties */
   twApi_RegisterProperty(TW_THING, thingName, "Temperature", TW_NUMBER, NULL, "ALWAYS", 0, propertyHandler,NULL);
   twApi_RegisterProperty(TW_THING, thingName, "Humidity", TW_NUMBER, NULL, "ALWAYS", 0, propertyHandler,NULL);
	
  /* Bind our thing */
  twApi_BindThing(thingName);

  /* Connect to server */
  if (!twApi_Connect(CONNECT_TIMEOUT, CONNECT_RETRIES)) {
    Serial.println("sucessefully connected to TWX!");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  dataCollectionTask();	
  delay(timeBetweenRefresh);
  Serial.print(".");
}



