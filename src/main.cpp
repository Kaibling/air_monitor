#include <Arduino.h>
//#include <WiFi.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h"
#include "DHT.h"
//#include <SimpleDHT.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
int pinDHT11 = D2;
DHT dht;
//SimpleDHT11 dht11;
//int pinDHT22 = D1;
//SimpleDHT22 dht22(pinDHT22);
const char* host = "10.2.10.5";

#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 400            //mv
#define        SYS_VOLTAGE                     5000           


/*
I/O define
*/
const int iled = D0;                                            //drive the led of sensor
const int vout = A0;                                            //analog input

/*
variable
*/
float density, voltage;
int   adcvalue;

/*
private function
*/
int Filter(int m)
{
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;
  
  if(flag_first == 0)
  {
    flag_first = 1;

    for(i = 0, sum = 0; i < _buff_max; i++)
    {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  }
  else
  {
    sum -= _buff[0];
    for(i = 0; i < (_buff_max - 1); i++)
    {
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
  digitalWrite(iled, LOW);                                     //iled default closed
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

  timeClient.begin();
  
  timeClient.setTimeOffset(3600);
  //timeClient.setTimeOffset(0);
  //GMT +1 3600

   //server.begin();
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
  

  float temperature  = 0.00;
  float humidity = 0.00;
  //do {
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();
  //if (strcmp(dht.getStatusString(),"OK") != 0) {
    Serial.print(timeClient.getEpochTime());
    //Serial.println(" Read DHT11 failed");
    //delay(1000);
    //return;
  //}
  //} while (humidity != 0.00);
  

 
 
 /*
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  do {
  if ((err = dht22.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT22 failed, err="); Serial.println(err);delay(2000);
    //return;
    delay(1000);
  }
  } while (humidity != 0.00);
  */

  /*
  get adcvalue
  */
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
    
  /*
  display the result
  */
  Serial.print("The current dust concentration is: ");
  Serial.print(density);
  Serial.print(" ug/m3\n");  


  Serial.print((float)temperature); Serial.print(" *C, ");
  Serial.print((float)humidity); Serial.println(" %");


    const int httpPort = 8086;

String url = String("http://")+ host + String(":")+httpPort + "/write?db=temp_test";
String payload = "air_monitoring,measuretype=temperature,unit=Celsius value=" + String(temperature) ;//+ " "+ timeClient.getEpochTime() +  "000000000";
sendPost( url, payload);

payload = "air_monitoring,measuretype=humidity,unit=% value=" + String(humidity) ;// + " "+ timeClient.getEpochTime() +  "000000000";
sendPost( url, payload);

payload = "air_monitoring,measuretype=ppm,uint=ug/m^3 value=" + String(density) ;//+ " "+ timeClient.getEpochTime() +  "000000000";
sendPost( url, payload);
/*
HTTPClient http;
http.begin(url);
http.addHeader("Content-Type", "application/x-www-form-urlencoded");
http.POST(payload);
http.writeToStream(&Serial);
http.end();
*/


  delay( 30 * 1000);
}