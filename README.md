# Send OBD Car Data Collected from an Arduino to InfluxDB via MQTT

Want to put that OBD port in your car to work? Here's how to do it using a simple Arduino DIY project!

```
OBD2 ELM327 Reader --Bluetooth--> Arduino --MQTT--> HiveMQ MQTT broker --Native-MQTT--> InfluxDB Cloud
```

This DIY project will allow you to track your car's GPS position, MPH, RPMs, torque, fault codes, and more. 

This repo contains the Arduino sketches to read OBD2 data from a car via bluetooth and send it to InfluxDB Cloud via MQTT.

## Parts List

These are the components I used, but there are many similar ones that will get the job done.

- [Arduino MKR 1010 (WiFi)](https://www.amazon.com/dp/B07FYFF5YZ?psc=1&ref=ppx_yo2ov_dt_b_product_details)
- [HC-05 Bluetooth Module](https://www.amazon.com/dp/B07C1PXM44?ref=ppx_yo2ov_dt_b_product_details&th=1)
- [NEO-6M GPS Module](https://www.amazon.com/dp/B07P8YMVNT?ref=ppx_yo2ov_dt_b_product_details&th=1)
- [Breadboard + Accessories Kit](https://www.amazon.com/dp/B01HRR7EBG?psc=1&ref=ppx_yo2ov_dt_b_product_details)
- [OBD2 / ELM327 Bluetooth Reader](https://www.amazon.com/gp/product/B005NLQAHS/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&th=1)

## Building the Rig

I loosely followed [this guide](https://create.arduino.cc/projecthub/jassak/mqtt-obd-vehicle-telemetry-f120c4) to build the circuit.

|HC-05|MKR1010|
|---|---|
|VCC|5v| 
|Rx|Tx|
|Tx|Rx|
|GND|GND|

|NEO-6M|MKR1010|
|---|---|
|VCC|5v| 
|Rx|Pin 0|
|Tx|Pin 1|
|GND|GND|

## Pair to ODB/ELM327 Reader

There are a few ways to do this but the most reliable one I found was to manually pair the HC-05 module to the ELM327 reader using the reader's MAC address.

1. Find the MAC Address of the ELM327 Reader. I was able to do this by pairing the reader to my Android phone. At the bottom of the screen it shows the address. (e.g. `00:1D:A5:68:C9:EF`)
2. Convert the MAC Address into the following format: XXXX,XX,XXXXXX (e.g. `001D,A5,68C9EF`)
3. Plug in the Arduino to your computer and upload the `pairing_hc05` sketch to the Arduino. Unplug the Arduino and plug it in again while pressing the small button on the HC-05 module. This puts the module into AT mode. It should flash once every couple seconds.
4. From Arduino IDE, run the following commands. You should receive an `OK` response after each one. Replace the X's with the formatted MAC address. [More info.](https://howtomechatronics.com/tutorials/arduino/how-to-configure-pair-two-hc-05-bluetooth-module-master-slave-commands/)

```
AT
AT+RESET
AT+ROLE=1
AT+CMODE=0
AT+BIND=XXXX,XX,XXXXXX
```

5. Unplug and re-plug the Arduino. The HC-05 should blink faster and connect to your ELM327 device quickly.

## Modify the Sketch

Update the `odb_to_mqtt_via_hc05.ino` sketch for your build:

- WiFi name
- WiFi password
- Topic

Upload the sketch to the MKR1010 using the Arduino IDE.

## Test it out

Plug the ODB2 Reader into your car and turn your car on. Bring the Arduino plugged into your car plugged into your laptop (ideally) so you can see the logs.

The Arduino should light up RED when it's attempting to connect to WiFi network. It will light up BLUE once its connected to the WiFi and is attempting to connect to HiveMQ broker. It will light up GREEN for 2 seconds before it begins reading the ODB2 data.

Once the main loop is running, the Arduino will query 14 or so different attributes from your car's ODB2 data. It will query these one at a time. The BLUE light means it's waiting on a response.

If it flashes GREEN then the message was received from the ODB2 reader and was sent to the MQTT broker successfully.

If it flashes RED then there was an error reading the response from the ODB2 reader. Sometimes the reader will return error "NODATA" for some queries depending on what type of car you have.

## HiveMQ MQTT Broker

Now that you're collecting data, you can connect to the HiveMQ MQTT broker from any MQTT client (e.g. http://www.hivemq.com/demos/websocket-client/). Subscribe to your topic and see your data flowing in.

## InfluxDB Cloud Native MQTT

The last step is to configure InfluxDB to subscribe to your topics and write the data to your InfluxDB bucket so you can monitor it over time.

- Create or login to your InfluxDB Cloud account
- Click Load Data -> Native Subscriptions
- Create a new Subscription
- Fill out the HiveMQ broker connection details (e.g. `broker.hivemq.com:1883`) and provide your topic (e.g. `my/odb/rpm`).
- Use the "String" data format and set the measurement to a name of your liking (e.g. `obd`).
- Set the field name to `rpm` and the value to `(.*)` so it captures the whole message
- Click Save Subscription
- Repeat for any other data points you want to capture.

Now you can query your data by running this flux query:

```flux
from(bucket: "my_bucket")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "obd")
  |> filter(fn: (r) => r["_field"] == "rpm")
  |> toFloat()
  ```

## More data?

My example only collects 14 or so data points from the ODB device. There are many more you can capture! Simply extend the `switch` block and invoke any other functions in the [elmduino](https://github.com/PowerBroker2/ELMduino) library. And make sure to add the corresponding sub topic to the `subTopics` array.

## Bluetooth Low Energy (BLE)?

The MKR1010 natively has support for BLE. The HC-05 module does not. And the ELM327 device I linked above does not. However, there are ELM327 devices (such as [this one](https://www.amazon.com/dp/B073XKQQQW?psc=1&ref=ppx_yo2ov_dt_b_product_details)) that do support BLE. It should be possible to use the ArduinoBLE library to pair the MKR1010 directly to the ELM327 BLE reader, but I have not tried this. Perhaps a future iteration of this project will attempt it.