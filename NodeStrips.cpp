#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include "RGB.h"
#include "SecureCred.h"

#define TIMEOUT 1500
String device = "ESP8266";

WiFiClientSecure client;
RGB led = RGB(D5, D6, D7);

void setup() {
  Serial.begin(115200);
  setupWiFi();
  setupSSL();
  led.begin();
  led.tick();
  //set(dump());
}

void set(String value) {//pushes data to the server
  while ((!client.connect(host, 443))) delay(100);
  String json  = "{\"state\":\"" + value + "\"}";
  client.println( String("PATCH ") + link + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: " + device + "\r\n" + "Content-Type: application/json" + "\r\n" + "Connection: close\r\n" + "x-apikey: " + key + "\r\n" +"Content-Length: " + String(json.length()) + "\r\n\r\n" +json + "\r\n");
  client.stop();
}

String fetch() { //gets data from the server
  while ((!client.connect(host, 443))) delay(100);
  client.println( String("GET ") + link + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n" + "x-apikey: " + key + "\r\n");
  while (client.connected()) if (String(client.readStringUntil('\n')) == "\r") break;
  String ret = "";
  while (client.available()) ret += client.readStringUntil('\n');
  client.stop();
  return valueForKey((char*)ret.c_str(), "state");
}

String valueForKey(char * d, char * k) {//super low level data manipulation, and i did it in one line too 😎
  char ret[20];
  for(int i=0;i<strlen(d);i++)for(int h=0;h<strlen(k);h++)if(k[h]!=d[i+h])break;else if(h=strlen(k)-1)for(int j=i+3+strlen(k);j<strlen(d);j++)if(d[j]=='\"')memcpy(&ret,&d[i+strlen(k)+3],j-i-3-strlen(k));
  ret[19] = '\0';
  return String(ret);
}


IPAddress setupWiFi(){//setup
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(NETWORK,PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  return WiFi.localIP();
}

void setupSSL(){//setup
  client.setInsecure();
  client.setTimeout(TIMEOUT);
}

String dump(){ //Red:Green:Blue:Enabled:Fade mode:speed = RR:GG:BB:EE:FF:SS
  return fixLength(String(led.r,HEX)) + ":" +fixLength(String(led.g,HEX)) + ":" +fixLength(String(led.b,HEX)) + ":" + (led.on == true ? "FF" : "00") + ":" + (led.tick() ? "FF" : "00")  + ":" + fixLength(String(led.speed,HEX));
}
void act(String state){//decides what to do with the data in the response
  unsigned char data[20];
  memcpy(&data,&state.c_str()[0],state.length());
  if (hex2bin(data,9) == 0) led.stop();
  if (hex2bin(data,12) != 0) led.start(); else led.setColor(hex2bin(data, 0), hex2bin(data, 3), hex2bin(data, 6));
  led.speed = hex2bin(data,15);
  if(led.tick() == false){
    led.setColor(hex2bin(data, 0), hex2bin(data, 3), hex2bin(data, 6));
  }
}

unsigned char hex2bin(unsigned char * str, unsigned char offset) {//low level data manipulation
  unsigned char ret = 0;
  unsigned char buf[2];
  memcpy(buf, &str[0] + offset, 2);
  ret += buf[0] <= '9' ? (buf[0] - 48) * 16 : (buf[0] - 55) * 16;
  ret += buf[1] <= '9' ? buf[1] - 48 : buf[1] - 55;
  return ret;
}

unsigned char dec2bin(unsigned char * str, unsigned char offset) {//low level data manipulation
  unsigned char buf[2];
  memcpy(buf, &str[0] + offset, 2);
  return (buf[0] - 48) * 10 + (buf[1] - 48);
}

String fixLength(String in){//adds zeros where needed
  return in.length() < 2 ? "0"+in : in;
}

void loop() {
  for(int i = 0; i < 32;i++) delay(led.tick() ? led.speed : 100);
  String newData = fetch();
  Serial.println(newData);
  act(newData);
}
