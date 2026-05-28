#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "HunterRoam.h"
#include "secrets.h"

// ── Configuration ──────────────────────────────────────
const int           HUNTER_PIN          = 4;
const char*         LOG_TOPIC           = "hunter/log";
const unsigned long HEARTBEAT_INTERVAL  = 5UL * 60 * 1000;

// ── Globals ────────────────────────────────────────────
WiFiClientSecure net;
PubSubClient     client(net);
HunterRoam       hunter(HUNTER_PIN);
unsigned long    lastHeartbeat = 0;

// ── Logging ────────────────────────────────────────────
void mqttLog(const String& msg) {
  Serial.println(msg);
  if (client.connected()) client.publish(LOG_TOPIC, msg.c_str());
}

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
  if (zone < 1 || zone > 8) { mqttLog("Invalid zone in topic"); return; }

  bool isStart = jsonContains(msg, "\"start\"");
  int  dur     = jsonInt(msg, "duration");

  mqttLog("[MQTT] zone=" + String(zone) + " action=" + (isStart ? "start" : "stop") + " dur=" + String(dur));

  byte err;
  if (isStart && dur > 0) {
    err = hunter.startZone((byte)zone, (byte)dur);
  } else {
    err = hunter.stopZone((byte)zone);
  }

  if (err) mqttLog("[Hunter] error: " + hunter.errorHint(err));
  else     mqttLog("[Hunter] zone=" + String(zone) + " " + (isStart ? "start" : "stop") + " OK");
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
      client.subscribe(MQTT_TOPIC, 1);
      mqttLog("[Boot] connected ip=" + WiFi.localIP().toString() + " rssi=" + String(WiFi.RSSI()) + "dBm");
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

  unsigned long now = millis();
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = now;
    mqttLog("[Health] uptime=" + String(now / 1000) + "s rssi=" + String(WiFi.RSSI()) + "dBm heap=" + String(ESP.getFreeHeap()));
  }
}
