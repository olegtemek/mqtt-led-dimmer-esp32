#include <Arduino.h>
#include <config.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);

uint8_t currentBrightness = DEFAULT_BRIGHTNESS;

void publishState()
{
  char brightnessStr[4];
  snprintf(brightnessStr, sizeof(brightnessStr), "%d", currentBrightness);
  client.publish(MQTT_TOPIC_STATE, brightnessStr);
}

void publishDiscovery()
{
  String topic = String(HA_DISCOVERY_PREFIX) + "/light/" + String(DEVICE_ID) + "/config";

  JsonDocument doc;
  doc["name"] = DEVICE_NAME;
  doc["unique_id"] = DEVICE_ID;
  doc["command_topic"] = MQTT_TOPIC_SET;
  doc["state_topic"] = MQTT_TOPIC_STATE;
  doc["brightness_command_topic"] = MQTT_TOPIC_SET;
  doc["brightness_state_topic"] = MQTT_TOPIC_STATE;
  doc["brightness_scale"] = 100;
  doc["on_command_type"] = "brightness";
  doc["payload_on"] = "100";
  doc["payload_off"] = "0";

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(DEVICE_ID);
  device["name"] = DEVICE_NAME;
  device["manufacturer"] = "DIY";
  device["model"] = "ESP32 LED Dimmer";

  char payload[640];
  serializeJson(doc, payload);

  client.publish(topic.c_str(), payload, true);
  Serial.println("HA Discovery config published:");
}

void reconnectMqtt()
{
  if (client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
  {
    Serial.println("MQTT connected");
    publishDiscovery();
    client.subscribe(MQTT_TOPIC_SET);
    analogWrite(MOS_PIN, currentBrightness);
    publishState();
  }
  else
  {
    Serial.print("Cannot connect to MQTT: ");
    Serial.println(client.state());
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String message;
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  if (String(topic) == MQTT_TOPIC_SET)
  {
    Serial.print("Received brightness: ");
    Serial.println(message);

    int brightness = message.toInt();
    brightness = constrain(brightness, 0, 100);

    currentBrightness = (uint8_t)brightness;
    analogWrite(MOS_PIN, currentBrightness);
    publishState();
  }
}

void setup()
{
  Serial.begin(9600);

  pinMode(MOS_PIN, OUTPUT);
  analogWrite(MOS_PIN, currentBrightness);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setHostname(HOSTNAME);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("cannot connect to wifi, retry..");
  }

  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setBufferSize(640);
  client.setCallback(callback);
}

unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 5000;

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(5000);
    return;
  }

  if (!client.connected())
  {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= RECONNECT_INTERVAL)
    {
      lastReconnectAttempt = now;
      reconnectMqtt();
    }
  }
  else
  {
    client.loop();
  }
}
