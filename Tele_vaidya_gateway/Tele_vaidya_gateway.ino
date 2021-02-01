#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <TimeLib.h>
#include <NtpClientLib.h>
#include "ESP8266Helpers.h"

#ifndef D5
#if defined(ESP8266)
#define D5 (14)
#define D6 (12)
#define D7 (13)
#define D8 (15)
#define TX (1)
#elif defined(ESP32)
#define D5 (18)
#define D6 (19)
#define D7 (23)
#define D8 (5)
#define TX (1)
#endif
#endif

#ifdef ESP32
#define BAUD_RATE 57600
#else
#define BAUD_RATE 9600
#endif

//#define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG    //Macros are usually in all capital letters.
#define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
#define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
#define DPRINT(...)     //now defines a blank line
#define DPRINTLN(...)   //now defines a blank line
#endif


SoftwareSerial mySerial;

int httpResponseCode;

//NTP Settings
int8_t timeZone = 5;
int8_t minutesTimeZone = 30;
const PROGMEM char *ntpServer = "pool.ntp.org";
boolean syncEventTriggered = faprocessSyncEventlse; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event


bool autoReconnect = true;

//Azure IoT Hub Credentials
const char* THUMBPRINT = "xxxxx";
const char* DEVICE_ID = "xxxxx";
const char* MQTT_HOST = "xxxxx.azure-devices.net";
const char* MQTT_USER = "xxxxx.azure-devices.net/<device-name>/?api-version=2018-06-30";
const char* MQTT_PASS = "SharedAccessSignature sr=xxxxx.azure-devices.net%2Fdevices%2F<device-name>&sig=xxxxx&se=xxxxx";
const char* MQTT_SUB_TOPIC = "devices/<device-name>/messages/devicebound/#";
const char* MQTT_PUB_TOPIC = "devices/<device-name>/messages/events/";

BearSSL::WiFiClientSecure tlsClient;
PubSubClient client(tlsClient);

WiFiEventHandler  wifiStationConnectedEvent, wifiStationDisconnectedEvent;

void setup()
{

  Serial.begin(115200);  //Serial connection
  mySerial.begin(BAUD_RATE, SWSERIAL_8N1, D5, D6, false, 95, 11);
  WiFi.mode(WIFI_STA);
  //  WiFi.begin("Dravidians", "1assasuvi37");   //WiFi connection
  WiFiManager wm;

  //reset settings - wipe credentials for testing
  //wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  res = wm.autoConnect("Citriot_Sense"); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  while (!Serial) continue;
}


void loop() {


  if (mySerial.available())
  {
    // Allocate the JSON document
    // This one must be bigger than for the sender because it must store the strings
    StaticJsonDocument<300> doc;

    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(doc, mySerial);

    if (err == DeserializationError::Ok)
    {
      // Print the values
      // (we must use as<T>() to resolve the ambiguity)
      Serial.print("PersonID = ");
      String PersonID = doc["member_id"];
      Serial.println(PersonID);
      Serial.print("Temperature = ");
      String Temp = doc["temperature"];
      Serial.println(member_id);
      Serial.print("Heart rate = ");
      String BPM = doc["Heart_rate"];
      Serial.println(BPM);
      Serial.print("Blood Oxygen Level = ");
      String O2 = doc["SPO2"];
      Serial.println(O2);
      if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

        if (PersonID != "null")
        {

          HTTPClient http;    //Declare object of class HTTPClient
          http.begin("http://enplan.in/apis/staff_info.php");      //Specify request destination
          http.addHeader("Content-Type", "text/plain");  //Specify content-type header
          String cmd = "{\"nfc_device_id\":\"" + nfc_device_id + "\"}";
          Serial.println(cmd);
          int httpCode = http.POST(cmd);   //Send the request
          String payload = http.getString();        //Get the response payload
          delay(100);
          mySerial.print(payload);
          delay(100);
          Serial.println(httpCode);   //Print HTTP return code
          Serial.println(payload);    //Print request response payload

          http.end();  //Close connection

        }
        else if (nfc_device_id == "null")
        {
          HTTPClient http2;
          http2.begin(SEND_DATA);


          String snd = "{\"member_id\":\"" + String(member_id) + "\"" + ",\"temperature\":\"" + temperature + "\"}";
          // If you need an HTTP request with a content type: text/plain
          http2.addHeader("Content-Type", "text/plain");
          int httpResponseCode2 = http2.POST(snd);
          Serial.println(snd);

          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode2);
          // Free resources
          String payload2 = http2.getString();    //Get the response payload
          delay(100);
          mySerial.print(payload2);
          delay(100);
          Serial.print(payload2);
          http2.end();
        }
        else
        {
          Serial.println("No data received");
        }

      }
      else
      {
        Serial.println("Wifi Not connected");
      }
    }
    else
    {
      // Print error to the "debug" serial port
      Serial.print("deserializeJson() returned ");
      Serial.println(err.c_str());

      // Flush all bytes in the "link" serial port buffer
      while (mySerial.available() > 0)
        mySerial.read();
    }

  }
}
