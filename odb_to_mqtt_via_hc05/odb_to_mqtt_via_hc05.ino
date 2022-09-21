#include <ELMduino.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include <TinyGPS++.h>
#include <utility/wifi_drv.h>
#include "wiring_private.h"

// elm327
ELM327 elm;
bool waiting = false;
int state = 0;
float value = -1;

// gps
TinyGPSPlus gps;
const long interval = 200;
unsigned long previousMillis = 0;
#define PIN_GPSSERIAL_RX       (1ul)
#define PIN_GPSSERIAL_TX       (0ul)
#define PAD_GPSSERIAL_TX       (UART_TX_PAD_0)
#define PAD_GPSSERIAL_RX       (SERCOM_RX_PAD_1)

Uart gpsSerial(&sercom3, PIN_GPSSERIAL_RX, PIN_GPSSERIAL_TX, PAD_GPSSERIAL_RX, PAD_GPSSERIAL_TX);

void SERCOM3_Handler()
{
  gpsSerial.IrqHandler();
}

// wifi
char ssid[] = "wifi";
char pass[] = "pass";
int status = WL_IDLE_STATUS;
WiFiClient wifiClient;

// mqtt broker
char host[] = "broker.hivemq.com";
int port = 1883;
MqttClient mqttClient(wifiClient);

// mqtt message
String topic = "gene/obd";
String subTopics[20] = {"/rpm","/mph","/engineLoad","/engineCoolantTemp","/intakeAirTemp","/throttle","/fuelLevel","/mafRate","/hybridBatLife","/oilTemp","/emissionRqmts","/demandedTorque","/torque","/batteryVoltage"};

void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);
  pinPeripheral(1, PIO_SERCOM);
  pinPeripheral(0, PIO_SERCOM);
  gpsSerial.begin(9600);

  // LEDs
  WiFiDrv::pinMode(25, OUTPUT);
  WiFiDrv::pinMode(26, OUTPUT);
  WiFiDrv::pinMode(27, OUTPUT);
  off();

  // hc-05 connect to elm327
  elm.begin(Serial1, true, 5000);
  
  // wifi connect
  while (WiFi.status() != WL_CONNECTED) {
    off();
    delay(100);
    red();
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    delay(10000);
  }
  Serial.println("Connected to the network");
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  blue();

  // mqtt connect
  if (!mqttClient.connect(host, port)) {
    Serial.print("MQTT broker connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.print("Connected to MQTT broker: ");
  Serial.println(host);

  green();
  delay(2000);
}

void loop()
{
  // the elm library is non-blocking and we can only make one query at a time
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
      // reset state and query rpms
      state = 0;
      value = elm.rpm();
      break;
  }

  // process the msg once recieved
  processOBD(value, subTopics[state]);

  // process latest gps coords
  processGPS();
}

void processOBD(float value, String subTopic) {
  if (elm.nb_rx_state == ELM_SUCCESS) {
    mqttClient.beginMessage(topic + subTopic);
    mqttClient.print(value);
    mqttClient.endMessage();
    green();
    delay(100);
    state += 1;
  } else if (elm.nb_rx_state == ELM_GETTING_MSG) {
    blue();
  } else {
    red();
    delay(100);
    elm.printError();
    state += 1;
  }
}

void processGPS() {
  double latitude = 0;
  double longitude = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval && gpsSerial.available()) {
    previousMillis = currentMillis;
    gps.encode(gpsSerial.read());
    if (gps.location.isUpdated()) {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
      String json = "{ \"lat\": " + (String)latitude + ", \"long\": " + longitude + " }";
      Serial.println(json);
      mqttClient.beginMessage(topic + "/gps");
      mqttClient.print(json);
      mqttClient.endMessage();
    }
  }
}

void green() {
  WiFiDrv::analogWrite(25, 0);
  WiFiDrv::analogWrite(26, 255);
  WiFiDrv::analogWrite(27, 0);
}

void blue() {
  WiFiDrv::analogWrite(25, 0);
  WiFiDrv::analogWrite(26, 0);
  WiFiDrv::analogWrite(27, 255);
}

void red() {
  WiFiDrv::analogWrite(25, 255);
  WiFiDrv::analogWrite(26, 0);
  WiFiDrv::analogWrite(27, 0);
}

void off() {
  WiFiDrv::analogWrite(25, 0);
  WiFiDrv::analogWrite(26, 0);
  WiFiDrv::analogWrite(27, 0);
}