#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h"
#include "DHT.h"

#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 400            //mv
#define        SYS_VOLTAGE                     5000           

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
int pinDHT11 = D2;
DHT dht;
const char* host = "10.2.10.5";
const int httpPort = 8086;
const int iled = D0;
const int vout = A0;

float density, voltage;
int   adcvalue;


int Filter(int m) {
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;
  
  if(flag_first == 0) {
    flag_first = 1;

    for(i = 0, sum = 0; i < _buff_max; i++) {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  }
  else {
    sum -= _buff[0];
    for(i = 0; i < (_buff_max - 1); i++) {
      _buff[i] = _buff[i + 1];
    }
    _buff[9] = m;
    sum += _buff[9];
    
    i = sum / 10.0;
    return i;
  }
}

void setup(){

  Serial.begin(9600);
  dht.setup(pinDHT11); 
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  pinMode(iled, OUTPUT);
  digitalWrite(iled, LOW);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.setTimeOffset(3600);
  //timeClient.setTimeOffset(0);
  //GMT +1 3600
}

void sendPost(String url, String payload) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(payload);
  http.writeToStream(&Serial);
  http.end();
}

void loop(){
  timeClient.update();
  Serial.print(timeClient.getEpochTime());

  float temperature  = 0.00;
  float humidity = 0.00;

  humidity = dht.getHumidity();
  temperature = dht.getTemperature();
  //if (strcmp(dht.getStatusString(),"OK") != 0) {
  //Serial.println(" Read DHT11 failed");
  //}

  digitalWrite(iled, HIGH);
  delayMicroseconds(280);
  adcvalue = analogRead(vout);
  digitalWrite(iled, LOW);
  adcvalue = Filter(adcvalue);
  /*
  covert voltage (mv)
  */
  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11;
  
  /*
  voltage to density
  */
  if(voltage >= NO_DUST_VOLTAGE){
    voltage -= NO_DUST_VOLTAGE;
    density = voltage * COV_RATIO;
  }
  else
    density = 0;
  
  Serial.print("The current dust concentration is: ");
  Serial.print(density);
  Serial.print(" ug/m3\n");  


  Serial.print((float)temperature); Serial.print(" *C, ");
  Serial.print((float)humidity); Serial.println(" %");



  String url = String("http://")+ host + String(":")+httpPort + "/write?db=temp_test";
  String payload = "air_monitoring,measuretype=temperature,unit=Celsius value=" + String(temperature);
  sendPost( url, payload);

  payload = "air_monitoring,measuretype=humidity,unit=% value=" + String(humidity);
  sendPost( url, payload);

  payload = "air_monitoring,measuretype=ppm,uint=ug/m^3 value=" + String(density);
  sendPost( url, payload);

  delay( 30 * 1000);
}