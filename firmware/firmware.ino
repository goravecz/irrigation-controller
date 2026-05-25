#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "HunterRoam.h"
#include "secrets.h"

// ── Configuration ──────────────────────────────────────
const int HUNTER_PIN = 4;  // GPIO connected to Hunter bus via transistor

// ── Globals ────────────────────────────────────────────
WiFiClientSecure net;
PubSubClient     client(net);
HunterRoam       hunter(HUNTER_PIN);

// ── JSON helpers (avoids ArduinoJson dependency) ───────
int jsonInt(const String& json, const char* key) {
  String search = String("\"") + key + "\":";
  int idx = json.indexOf(search);
  if (idx < 0) return -1;
  return json.substring(idx + search.length()).toInt();
}

bool jsonContains(const String& json, const char* val) {
  return json.indexOf(val) >= 0;
}

// ── MQTT message handler ───────────────────────────────
void onMessage(char* topic, byte* payload, unsigned int len) {
  String msg = String((char*)payload, len);
  String t   = String(topic);

  int zone = t.substring(t.lastIndexOf('/') + 1).toInt();
  if (zone < 1 || zone > 8) { Serial.println("Invalid zone in topic"); return; }

  bool isStart = jsonContains(msg, "\"start\"");
  int  dur     = jsonInt(msg, "duration");

  Serial.printf("[MQTT] zone=%d action=%s dur=%d\n", zone, isStart ? "start" : "stop", dur);

  byte err;
  if (isStart && dur > 0) {
    err = hunter.startZone((byte)zone, (byte)dur);
  } else {
    err = hunter.stopZone((byte)zone);
  }

  if (err) Serial.println("Hunter error: " + hunter.errorHint(err));
}

// ── WiFi ───────────────────────────────────────────────
void connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
}

// ── MQTT ───────────────────────────────────────────────
void connectMqtt() {
  while (!client.connected()) {
    Serial.print("MQTT connecting...");
    String clientId = "esp32-hunter-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println(" OK");
      client.subscribe(MQTT_TOPIC);
    } else {
      Serial.printf(" failed rc=%d, retrying in 5s\n", client.state());
      delay(5000);
    }
  }
}

// ── Setup / Loop ───────────────────────────────────────
void setup() {
  Serial.begin(115200);
  net.setInsecure();  // TLS without cert pinning — connection is still encrypted
  connectWifi();
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(onMessage);
  connectMqtt();
}

void loop() {
  if (!client.connected()) connectMqtt();
  client.loop();
}
