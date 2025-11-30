#import <WiFiClientSecure.h>
#include <WiFi.h>
#include "time.h"
#include <stdlib.h>

#include <MD_Parola.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "config.h"
#include "mqtt.h"
#include "Score.h"
#include "fonts.h"

#define HOME_BOARD 0
#define AWAY_BOARD 1
#define HOME_NAME 2
#define HOME_SCORE 3
#define AWAY_NAME 4
#define AWAY_SCORE 5

#define ARR_SIZE(arr) ( sizeof((arr)) / sizeof((arr[0])) )
char *welcomes[6][2] = {
  {"CIAO", "BENVENUTI"},
  {"HELLO", "WELCOME"},
  {"SALUT", "BIENVENUE"},
  {"HOLA", "BIENVENIDOS"},
  {"OLA", "BEM-VINDOS"},
  {"ZDRAVO", "DOBRODOsLI"}
};

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

WiFiClientSecure espClient;
PubSubClient client(espClient);

Score* s = new Score();

void reconnect_to_wifi(String ssid, String passphrase, uint8_t channel) {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Reconnecting to WiFi");
  WiFi.begin(ssid, passphrase, channel);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void reconnect_to_mqtt(PubSubClient* client) {
  // Loop until we're reconnected
  while (!client->connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client->connect(MQTT_CLIENT_NAME, "scoreboard-subscriber", "zujqqG9s4E#B$FvM")) {
      Serial.println(" Connected!");
      client->subscribe(MQTT_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client->state());
      Serial.println(" try again in 1 second");
      delay(1000);
    }
  }
}

size_t displayMode = 0;

String homeName, awayName;
String homeScore, awayScore;

void mqtt_callback(char *topic, byte* payload, unsigned int length) {
  displayMode = 1;

  String message_in = "";
  for (int i = 0; i < length; i++) {
    message_in += (char)payload[i];
  }
  Serial.print("Got message: ");
  Serial.println(message_in);

  JsonDocument doc;
  deserializeJson(doc, message_in);

  homeName = doc["homeName"].as<String>();
  awayName = doc["awayName"].as<String>();
  homeScore = doc["homeScore"].as<String>();
  awayScore = doc["awayScore"].as<String>();
}

void update(int displayMode) {
  if (display.displayAnimate()) {
    if (displayMode == 0) {
      return;
    }

    display.displayReset();

    display.displayZoneText(HOME_NAME, homeName.c_str(), PA_LEFT, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
    display.displayZoneText(AWAY_NAME, awayName.c_str(), PA_RIGHT, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
    display.displayZoneText(HOME_SCORE, homeScore.c_str(), PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
    display.displayZoneText(AWAY_SCORE, awayScore.c_str(), PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);

    // Vertical bar
    // display.getGraphicObject()->setColumn(32, 255);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome to ESP32-Scoreboard!");

  // TODO: use constants
  display.begin(6);
  display.setZone(HOME_BOARD, 8, 15);
  display.setZone(AWAY_BOARD, 0, 7);
  display.setZone(HOME_NAME, 10, 15);
  display.setZone(HOME_SCORE, 8, 9);
  display.setZone(AWAY_NAME, 0, 5);
  display.setZone(AWAY_SCORE, 6, 7);
  display.setFont(apoteke);

  WiFi.mode(WIFI_STA);

  espClient.setCACert(root_ca);
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqtt_callback);
}

void loop() {
  reconnect_to_wifi(WIFI_SSID, WIFI_PASSPHRASE, WIFI_CHANNEL);
  if (!client.connected()) {
    reconnect_to_mqtt(&client);
  }
  client.loop();

  // Update the display
  update(displayMode);

  delay(100);
}
