#include <Wire.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "ESPDateTime.h"
#include <Separador.h>

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_PUBLISH_TOPIC2   "esp32/tab"
#define RXD2 16
#define TXD2 17

Separador s;

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

String inputString = "";  // a String to hold incoming data
String TOTAL="";
String DATA=""; 
String MNGMT="";
String PPROM_M="";
String PPROM_D="";
String MAC="";
String TYPE="";
String PRSSI=""; 
String CHANN="";
bool stringComplete = false; 

void wifiInit(){
   
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
}

void connectAWS()
{
  

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  Serial.print("\n Connecting to AWS IOT \n");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("\n AWS IoT Timeout!");
    return;
  }

}

void reconnect() {
  while (!client.connect(THINGNAME)) {
    Serial.print("Intentando conectarse MQTT...");

    if (client.connect(THINGNAME)) {
      Serial.println("Conectado");
      
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" intentar de nuevo en 5 segundos \n");
      if(WiFi.status() != WL_CONNECTED){
      Serial.println("Se presento desconexión de internet reintentando conexión \n");
      wifiInit();
      };
      delay(5000);// Wait 5 seconds before retrying
    }
  }
}

void publishMessage(String TOTAL,String DATA, String MNGMT,String PPROM_M,String PPROM_D)
{
  StaticJsonDocument<200> doc;
 // doc["device_id"] = "ESP32_sensor";
  doc["time"] = DateTime.format(DateFormatter::SIMPLE).c_str();
  //doc["Total"]=TOTAL;
  doc["Data"] = DATA;
  doc["Management"] = MNGMT;
  doc["PPM"]=PPROM_M;
  doc["PPD"]=PPROM_D;
  doc["Personas"]="";
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void publishMessage2(String TYPE,String MAC, String PRSSI, String CHANN)
{
  StaticJsonDocument<200> doc;
 // doc["device_id"] = "ESP32_sensor";
  doc["time"] = DateTime.format(DateFormatter::SIMPLE).c_str();
  doc["T"]=TYPE;
  doc["MAC"] = MAC;
  doc["RSSI"] = PRSSI;
  doc["CHANN"]=CHANN;
  doc["Personas"]="12";
   
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC2, jsonBuffer);
}

void setupDateTime() {
  // setup this after wifi connected
  // you can use custom timeZone,server and timeout
  //DateTime.setServer("time.pool.aliyun.com");
  DateTime.setTimeZone("CST+5");
  DateTime.begin();
 }

void serialEvent() {
  while (Serial2.available()) {
    // get the new byte:
    char inChar = (char)Serial2.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  } client.loop();
}

void divide_mensaje(String inputString){
    TOTAL=s.separa(inputString,',',0);
    DATA=s.separa(inputString,',',1);
    MNGMT=s.separa(inputString,',',2);
    PPROM_M=s.separa(inputString,',',3);
    PPROM_D=s.separa(inputString,',',4);
    TYPE=s.separa(inputString,',',5);
    MAC=s.separa(inputString,',',6);
    PRSSI=s.separa(inputString,',',7);
    CHANN=s.separa(inputString,',',8);
    
  }


void restaura(){
  
    inputString = "";
    TOTAL="";
    MNGMT="";
    DATA="";
    PPROM_M="";
    PPROM_D="";
    TYPE="";
    MAC="";
    PRSSI="";
    CHANN="";
    stringComplete = false;
    

}

void setup()
{
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(10);
  wifiInit();
  connectAWS();
  setupDateTime();
  Serial.print("-----Conectado---- \n");
 
}

void loop()
{
     if (!client.connected()) {
    reconnect();
  }
   
if (stringComplete) {
      divide_mensaje(inputString);
      Serial.println("---------PRUEBA TOTAL---------------\n");
      Serial.println(TOTAL);
      Serial.println("---------PRUEBA TOTAL---------------\n");
      Serial.println(TYPE);
      if(TOTAL != ""){
      publishMessage(TOTAL,DATA,MNGMT,PPROM_M,PPROM_D);
      
      Serial.println("---------Cantidad Mensajes Recibidos------------------\n");
      Serial.println(TOTAL);
      Serial.println("---------Cantidad Mensajes Data------------------\n");
      Serial.println(DATA);
      Serial.println("---------Cantidad Mensajes MNGMT------------------\n");
      Serial.println(MNGMT);
      Serial.println("----------Potencia Promedio MNGMT -----------------\n");
      Serial.println(PPROM_M);
      Serial.println("----------Potencia Promedio DATA -----------------\n");
      Serial.println(PPROM_D);}
      else
      {
        publishMessage2(TYPE,MAC,PRSSI,CHANN);
        }
      client.loop();
      Serial.println("enviado");
     
    // clear the string:
    restaura();
  }
      client.loop();
      serialEvent();
  
      
  
}
