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
#include "fonts.h"

typedef struct {
  textEffect_t left;
  textEffect_t right;
} WelcomeTransition;

#define ARR_SIZE(arr) ( sizeof((arr)) / sizeof((arr[0])) )
char *welcome_messages[] = {"BENVENUTI", "BEM-VINDOS", "BIENVENIDOS", "BIENVENUE", // "BONVENON",
                            "BUN VENItI", "DOBRODOsLI",
                            // "dObPE dOhln", "LASKAVO PROSIMO",
                            "TERVETULOA", "uDVoZoLJuK", "VaLKOMNA",
                            "VELKOMMEN", "ViTEJTE", "WELCOME", "WELKOM", "WILLKOMMEN", "WITAMY"};
WelcomeTransition welcome_transitions[] = {
  {PA_SCROLL_DOWN, PA_SCROLL_UP},
  {PA_DISSOLVE, PA_DISSOLVE},
  {PA_GROW_DOWN, PA_GROW_UP},
  {PA_BLINDS, PA_BLINDS},
};

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

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

void reconnect_to_mqtt(PubSubClient* mqttClient) {
  // Loop until we're reconnected
  while (!mqttClient->connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient->connect(MQTT_CLIENT_NAME, "scoreboard-subscriber", "zujqqG9s4E#B$FvM")) {
      Serial.println(" Connected!");
      mqttClient->subscribe(MQTT_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient->state());
      Serial.println(" try again in 1 second");
      delay(1000);
    }
  }
}

size_t displayMode = 0;

String homeName, awayName;
String homeScore, awayScore;

void mqttCallback(char *topic, byte* payload, unsigned int length) {
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

  String event = doc["event"].as<String>();
  if (event == "GOAL_HOME" || event == "GOAL_AWAY") {
    displayMode = 2;
  }
}

size_t display_until = 0;
size_t welcome_message_index = 0;
size_t welcome_transition_index = 0;

void update_display() {
  if (display.displayAnimate()) {
    if (displayMode == 0) {
      if (display_until == 0 || millis() > display_until) {
        display_until = millis() + 2000;
        // display.displayClear();
        display.displayZoneText(HOME_BOARD, welcome_messages[welcome_message_index], PA_CENTER, 100, 3000, welcome_transitions[welcome_transition_index].left, PA_NO_EFFECT);
        display.displayZoneText(AWAY_BOARD, welcome_messages[welcome_message_index], PA_CENTER, 100, 3000, welcome_transitions[welcome_transition_index].right, PA_NO_EFFECT);
        welcome_message_index++;
        if (welcome_message_index >= ARR_SIZE(welcome_messages)) {
          welcome_message_index = 0;
        }
        welcome_transition_index++;
        if (welcome_transition_index >= ARR_SIZE(welcome_transitions)) {
          welcome_transition_index = 0;
        }
      }
    } else if (displayMode == 1 && (display_until == 0 || millis() > display_until)) {
      // Serial.println("displayMode = 1");
      display_until = 0;
      display.displayZoneText(HOME_NAME, homeName.c_str(), PA_LEFT, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
      display.displayZoneText(AWAY_NAME, awayName.c_str(), PA_RIGHT, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
      display.displayZoneText(HOME_SCORE, homeScore.c_str(), PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
      display.displayZoneText(AWAY_SCORE, awayScore.c_str(), PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);

      // Vertical bar
      // display.getGraphicObject()->setColumn(32, 255);
    } else if (displayMode == 2) {
      if (display_until == 0) {
        display_until = millis() + 8000;
        display.setInvert(1);
        display.displayClear();
        display.displayZoneText(HOME_BOARD, "GOOOOOOOOO", PA_RIGHT, 100, 3000, PA_SCROLL_LEFT, PA_NO_EFFECT);
        display.displayZoneText(AWAY_BOARD, "OOOOOOOOOL", PA_LEFT, 100, 3000, PA_SCROLL_RIGHT, PA_NO_EFFECT);
      } else if (millis() > display_until) {
        display_until = 0;
        display.setInvert(0);
        displayMode = 1;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome to ESP32-Scoreboard!");

  display.begin(ZONES);
  display.setZone(HOME_BOARD, 8, 15);
  display.setZone(AWAY_BOARD, 0, 7);
  display.setZone(HOME_NAME, 10, 15);
  display.setZone(HOME_SCORE, 8, 9);
  display.setZone(AWAY_NAME, 0, 5);
  display.setZone(AWAY_SCORE, 6, 7);
  display.setFont(apoteke);

  WiFi.mode(WIFI_STA);

  espClient.setCACert(rootCA);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  reconnect_to_wifi(WIFI_SSID, WIFI_PASSPHRASE, WIFI_CHANNEL);
  reconnect_to_mqtt(&mqttClient);

  mqttClient.loop();

  update_display();

  delay(100);
}
