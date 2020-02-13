/*
  ESP8266 ESP12 - 1
 
  Comando: AT+GMR para saber a versão do SDK e do AT.

  AT version:0.51.0.0(Nov 27 2015 13:37:21)
  SDK version:1.5.0
  compile time:Nov 27 2015 13:58:02
  OK  
*/

/*
C:\Firmware - ESP8266\esp_iot_sdk_v1.5.0_15_11_27\esp_iot_sdk_v1.5.0\bin\esp_init_data_default.bin  -  0xfc000
C:\Firmware - ESP8266\esp_iot_sdk_v1.5.0_15_11_27\esp_iot_sdk_v1.5.0\bin\blank.bin  -  0xfe000
C:\Firmware - ESP8266\esp_iot_sdk_v1.5.0_15_11_27\esp_iot_sdk_v1.5.0\bin\boot_v1.4(b1).bin  -  0x00000
C:\Firmware - ESP8266\esp_iot_sdk_v1.5.0_15_11_27\esp_iot_sdk_v1.5.0\bin\at\512+512\user1.1024.new.2.bin  -  0x01000
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Arduino.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

#define OTA_PASSDW "admin"

//alterei para false
#define DEBUG true // false: OTA_PASSDW enabled; true: OTA_PASSDW disabled

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 2
#define TELNET_PORT 23

#define MAX_LENGTH_TELNET_PW 50

WiFiServer server(TELNET_PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];
bool clientsLogged[MAX_SRV_CLIENTS];

const int led = 2;

String inputString[MAX_SRV_CLIENTS];

byte cntlogin[MAX_SRV_CLIENTS];

String TelnetClientPW[MAX_SRV_CLIENTS] = {"121091"}; // Note: Initially this code was written for only one client

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void telnetInit()
{
  int i;

  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    clientsLogged[i] = false;
    inputString[i].reserve(MAX_LENGTH_TELNET_PW);
    cntlogin[i] = 0;
  }

  server.begin();
  server.setNoDelay(true);

  Serial.print("Telnet server initialized!");
  Serial.print(" (Port: ");
  Serial.print(TELNET_PORT, DEC);
  Serial.println(")");
  Serial.println("You can use the PuTTY.exe or Hercules SETUP utility!");
  Serial.println("Get here: https://www.putty.org/");
  Serial.println("Or here: https://www.hw-group.com/software/hercules-setup-utility");
  Serial.println("Hercules SETUP utility is good for debug");
  Serial.println("(You can use TCP client for Telnet)");
}

void telnet()
{
  uint8_t i, j;
  char c;

  //check if there are any new clients
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        Serial.print("New client: "); Serial.println(i);
        clientsLogged[i] = false;
        cntlogin[i] = 0;
        if (serverClients[i] && serverClients[i].connected()) {
          serverClients[i].println("ESP8266 ESP12 - 1");
          serverClients[i].print("Login: ");
          while (serverClients[i].available()) {
            c = (char)serverClients[i].read();
          }
          inputString[i] = "";
          delay(1);
        }
        break;
      }
    }
    //no free/disconnected spot so reject
    if ( i == MAX_SRV_CLIENTS) {
      WiFiClient serverClients = server.available();
      serverClients.stop();
      Serial.println("Connection rejected ");
    }
  }
  //check clients for password and data
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      if (serverClients[i].available()) {
        //get data from the telnet client and push it to the UART
        while (serverClients[i].available()) {
          c = (char)serverClients[i].read();
          if (clientsLogged[i] == false) {
            if (inputString[i].length() >= MAX_LENGTH_TELNET_PW)
            {
              inputString[i] = "";
            } else {
              if ((c != '\r') & (c != '\n')) inputString[i] += c;
            }

            if (c == '\n') {
              if (inputString[i] == TelnetClientPW[i]) {
                serverClients[i].println("Bem vindo!");
                serverClients[i].print("$");
                clientsLogged[i] = true;
              } else {
                if (cntlogin[i] < 3) {
                  cntlogin[i]++;
                  serverClients[i].print("Senha incorreta.\r\nLogin: ");
                } else {
                  serverClients[i].println("Access denied. Connection rejected.");
                  serverClients[i].stop();
                  Serial.println("\r\nAccess denied. Connection rejected.");
                }
              }
              inputString[i] = "";
            }
          } else {
            if (inputString[i].length() >= MAX_LENGTH_TELNET_PW)
            {
              inputString[i] = "";
            } else {
              if ((c != '\r') & (c != '\n')) inputString[i] += c;
            }
            //if ((c != '\r') & (c != '\n')) serverClients[i].write(c); //echo
            if (c == '\n') {
              if (inputString[i] == (String)("led")) {
                serverClients[i].print("Toggle LED, ");
                digitalWrite(led, !digitalRead(led));
                serverClients[i].print("Pin ");
                serverClients[i].print(led, DEC);
                serverClients[i].print(": ");
                if (digitalRead(led)) {
                  serverClients[i].print("HIGH");
                }
                if (!digitalRead(led)) {
                  serverClients[i].print("LOW");
                }
              }
              else if (inputString[i] == (String)("reset")) {
                serverClients[i].print("Rebooting");
                for (j = 0; j < 10; j++) {
                  delay(250);
                  serverClients[i].print(".");
                }
                serverClients[i].stop();
                delay(250);
                ESP.reset();
              }
              serverClients[i].print("\r\n$");
              inputString[i] = "";
            }
          }
          Serial.write(c);
        }
      }
    }
    //check UART for data
    if (Serial.available()) {
      size_t len = Serial.available();
      uint8_t sbuf[len];
      Serial.readBytes(sbuf, len);
      for (i = 0; i < len; i++) {
        Serial.write(sbuf[i]);
      }
      if (sbuf[len - 1] == 13) Serial.println();
      //push UART data to all connected telnet clients
      for (i = 0; i < MAX_SRV_CLIENTS; i++) {
        if (serverClients[i] && serverClients[i].connected()) {
          serverClients[i].write(sbuf, len);
          if (sbuf[len - 1] == 13) serverClients[i].println();
          delay(1);
        }
      }
    }
  }
}

void wifiManagerInit()
{
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  Serial.println(WiFi.localIP());

  //if you get here you have connected to the WiFi
  Serial.println("conexão ok! :)");
}

void OTAinit()
{
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  if (DEBUG == false) {
    // Comment to: No authentication by default
    ArduinoOTA.setPassword(OTA_PASSDW);
    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  }

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(led, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(led, LOW);

  Serial.begin(115200);

  wifiManagerInit();

  Serial.println("Booting");

  OTAinit();

  telnetInit();

  digitalWrite(led, LOW);
  delay(5000);

  // Rotina para liberar Gpio's para uso.
  pinMode(04, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(15, OUTPUT);
}

void loop() {
  ArduinoOTA.handle();

  telnet();
  //Rotina do Led 2.
  digitalWrite(led, HIGH);
  delay(1000);
  digitalWrite(led, LOW);
  delay(1000);
  digitalWrite(led, HIGH);
  delay(1000);

  //Rotina para colocar pinos HIGH / LOW.
  //

  //Rotina do Relé
  digitalWrite(4, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(5000);                       // wait for a second
  digitalWrite(4, LOW);    // turn the LED off by making the voltage LOW
  delay(10000);                     // wait for a second

  /*
  //Rotina do Led Green
  digitalWrite(12, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(2000);                       // wait for a second
  digitalWrite(12, LOW);    // turn the LED off by making the voltage LOW
  delay(5000);                       // wait for a second

  //Rotina do Led Blue
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(4000);                       // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  delay(8000);                       // wait for a second

  //Rotina do Led Red
  digitalWrite(15, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(2000);                       // wait for a second
  digitalWrite(15, LOW);    // turn the LED off by making the voltage LOW
  delay(10000);                       // wait for a second
 */
}
