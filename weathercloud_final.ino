//Web client
#include <SPI.h>
#include <Ethernet.h>
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char server[] = "http://api.weathercloud.net";
IPAddress ip(192, 168, 0, 130);
EthernetClient client;
//-------------------------------------------------------------------------------------
//DS18B20
#include "cactus_io_DS18B20.h" 
int DS18B20_Pin = 9;
DS18B20 ds(DS18B20_Pin);
//-------------------------------------------------------------------------------------
//DHT21
#include "DHT.h"
#define DHTPIN 3
#define DHTTYPE DHT21 
DHT dht(DHTPIN, DHTTYPE);
//-------------------------------------------------------------------------------------
//RTC
#include <DS3231.h>
DS3231  rtc(SDA, SCL);
Time  time;
// rtc.setDOW(WEDNESDAY);     // Set Day-of-Week to SUNDAY
//rtc.setTime(15, 55, 0);     // Set the time to 12:00:00 (24hr format)
//rtc.setDate(17, 10, 2018);   // Set the date to January 1st, 2014
//-------------------------------------------------------------------------------------
//rain
#include <Wire.h>
int rpd, rph, input;
//-------------------------------------------------------------------------------------

void setup() {
  //Web Server
  Serial.begin(9600);
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    Ethernet.begin(mac, ip);
  } else {
    Serial.print("DHCP assigned IP: ");
    Serial.println(Ethernet.localIP());
  }
  delay(1000);
  Serial.print("connecting to ");
  Serial.print(server);
  Serial.println("...");

  if (client.connect(server, 80)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
  } else {
    Serial.println("connection failed");
  }
  //-------------------------------------------------------------------------------------
  //sensors
  Serial.begin(9600);
  ds.readSensor();
  dht.begin();
  rtc.begin();
  Wire.begin();
  //-------------------------------------------------------------------------------------   
}

void loop() {
  //temperature
  ds.readSensor();
  float temperature = ds.getTemperature_C()*10;
  int temp = int(temperature);
  //-------------------------------------------------------------------------------------
  //inside temperature
  int tempin = rtc.getTemp()*10;
  //-------------------------------------------------------------------------------------
  //humidity
  int hum = int(dht.readHumidity());
  //-------------------------------------------------------------------------------------
  //heat index
  float hic = dht.computeHeatIndex(ds.getTemperature_C(), dht.readHumidity(), false)*10;
  int heat = int(hic);
  //-------------------------------------------------------------------------------------
  //dew point
  double gamma = log(dht.readHumidity() / 100) + ((17.62 * ds.getTemperature_C()) / (243.5 + ds.getTemperature_C()));
  double dp = 243.5 * gamma / (17.62 - gamma);
  double dewpoint = dp*10;
  int dew = int(dewpoint);
  //-------------------------------------------------------------------------------------
  //UV index
  int UVIndex = 0;
  int sensorValue = 0;      
  sensorValue = analogRead(1);                        //connect UV sensor to Analog 0   
  int voltage = (sensorValue * (5.0 / 1023.0))*1000;  
  int a = voltage - 227;
  int b = a / 9.43;
  UVIndex = b + 10;
  //-------------------------------------------------------------------------------------
  //rainfall
  Wire.requestFrom(8, 4);
  while (Wire.available()){ 
  input = Wire.read();
      if(input == 1){
      rph = rph + 3;
      rpd = rpd + 3;
      }
  }
  time = rtc.getTime();   
  if(time.min  == 0){
    rph = 0;
  }
  if(time.hour  == 0){
    if(time.min  == 0){
    rpd = 0;
  }
  }
  //-------------------------------------------------------------------------------------
  //sending data
  client.print("GET /set/wid/e21ab07989c2a72d/key/3f18a9090e493b4b051c0d4d7dbe6102/temp/");
  client.print(temp);
  client.print("/tempin/");
  client.print(tempin);
  client.print("/hum/");
  client.print(hum);
  client.print("/heat/");
  client.print(heat);
  client.print("/dew/");
  client.print(dew);
  //client.print("/uvi/");
  //client.print(UVIndex);
  client.print("/rain/");
  if(rpd < 10){
    client.print("0");
    client.print(rpd);
  }
  else{
    client.print(rpd);
  }
  client.print("/rainrate/");
  if(rph < 10){
    client.print("0");
    client.print(rph);
  }
  else{
    client.print(rph);
  } 
  client.println("/ HTTP/1.1");
  client.println("Host: api.weathercloud.net");
  client.println();
  delay(1000);

  if (!client.connected()) {
    Serial.println("client disconected.");
      if (client.connect(server, 80)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
  } else {
    Serial.println("connection failed");
  }
  }
}
