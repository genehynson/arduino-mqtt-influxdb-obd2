#include "ELMduino.h"
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <TinyGPS++.h>

// elm327
ELM327 elm;
bool waiting = false;
int state = 0;
float value = -1;

// gps
TinyGPSPlus gps;
unsigned long cas = 0;

// wifi
char ssid[] = "GenePixel";
char pass[] = "geneisthebest";
int status = WL_IDLE_STATUS;
WiFiClient wifiClient;

// mqtt broker
char host[] = "broker.hivemq.com";
int port = 1883;
MqttClient mqttClient(wifiClient);

// mqtt message
String topic = "gene/obd";
String subTopics[20] = {"/rpm","/mph","/engineLoad","/engineCoolantTemp","/intakeAirTemp","/throttle","/fuelLevel","/mafRate","/hybridBatLife","/oilTemp","/emissionRqmts","/demandedTorque","/torque","/batteryVoltage"};
const long interval = 100;
unsigned long previousMillis = 0;

void setup()
{
  // hc-05 connect to elm327
  Serial1.begin(9600);
  elm.begin(Serial1, true, 5000);
  
  // wifi connect
  Serial.begin(9600);
  while (!Serial);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    delay(10000);
  }
  Serial.println("Connected to the network");
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // mqtt connect
  if (!mqttClient.connect(host, port)) {
    Serial.print("MQTT broker connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.print("Connected to MQTT broker: ");
  Serial.println(host);
}

void loop()
{
  unsigned long currentMillis = millis();
  // the elm library is non-blocking and we can only make one query at a time
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    mqttClient.poll();

    // query a msg
    switch (state) {
      case 0:
        value = elm.rpm();
        break;
      case 1:
        value = elm.mph();
        break;
      case 2:
        value = elm.engineLoad();
        break;
      case 3:
        value = elm.engineCoolantTemp();
        break;
      case 4:
        value = elm.intakeAirTemp();
        break;
      case 5:
        value = elm.throttle();
        break;
      case 6:
        value = elm.fuelLevel();
        break;
      case 7:
        value = elm.mafRate();
        break;
      case 8:
        value = elm.hybridBatLife();
        break;
      case 9:
        value = elm.oilTemp();
        break;
      case 10:
        value = (float)elm.emissionRqmts();
        break;
      case 11:
        value = elm.demandedTorque();
        break;
      case 12:
        value = elm.torque();
        break;
      case 13:
        value = elm.batteryVoltage();
        break;
      default:
        state = 0;
        break;
    }

    // process the msg recieved
    processOBD(value, subTopics[state]);

    // processGPS();
  }
}

void processOBD(float value, String subTopic) {
  if (elm.nb_rx_state == ELM_SUCCESS) {
    mqttClient.beginMessage(topic + subTopic);
    mqttClient.print(value);
    mqttClient.endMessage();
    state += 1;
  } else if (elm.nb_rx_state != ELM_GETTING_MSG) {
    elm.printError();
    state += 1;
  }
}

void processGPS() {
  double latitude = 0;
  double longitude = 0;
  while (Serial2.available() > 0){
      gps.encode(Serial2.read());
      if (gps.location.isUpdated()){
        for(millis(); (millis() - cas) > 5000;){
          cas = millis();
          latitude = (gps.location.lat());
          longitude = (gps.location.lng());
      }
      String json = "{ \"lat\": \"" + (String)latitude + "\", \"long\": \"" + longitude + "\" }";
      mqttClient.beginMessage(topic + "/gps");
      mqttClient.print(json);
      mqttClient.endMessage();
    }
  }
}