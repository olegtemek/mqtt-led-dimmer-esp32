#include <Arduino.h>
#include <config.h>
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

uint8_t currentBrightness = DEFAULT_BRIGHTNESS;

void publishState()
{
  char brightnessStr[4];
  snprintf(brightnessStr, sizeof(brightnessStr), "%d", currentBrightness);
  client.publish(MQTT_TOPIC_STATE, brightnessStr);
}

void reconnect()
{
  while (!client.connected())
  {
    if (client.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
    {
      client.subscribe(MQTT_TOPIC_SET);
      analogWrite(MOS_PIN, currentBrightness);
      publishState();
    }
    else
    {
      Serial.print("Cannot connect to MQTT: ");
      Serial.println(client.state());
      delay(5000);
    }
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
    brightness = constrain(brightness, 0, 255);

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
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}
