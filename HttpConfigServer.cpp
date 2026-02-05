#include "HttpConfigServer.h"

#include <ctype.h>

#include "Log.h"
#include "RuntimeConfig.h"

static bool parseFloatField(const String& body, const String& key, float& outVal) {
  String lower = body;
  lower.toLowerCase();
  String k = key;
  k.toLowerCase();

  int idx = lower.indexOf(k);
  if (idx < 0) return false;
  int colon = lower.indexOf(':', idx);
  if (colon < 0) return false;

  int i = colon + 1;
  while (i < lower.length() && (lower[i] == ' ' || lower[i] == '\"')) i++;

  int start = i;
  while (i < lower.length() && (isdigit(lower[i]) || lower[i] == '.' || lower[i] == '-')) i++;
  if (start == i) return false;

  outVal = body.substring(start, i).toFloat();
  return true;
}

static bool parseTimeField(const String& body, const String& key, int& minutesOut) {
  String lower = body;
  lower.toLowerCase();
  String k = key;
  k.toLowerCase();

  int idx = lower.indexOf(k);
  if (idx < 0) return false;
  int colon = lower.indexOf(':', idx);
  if (colon < 0) return false;

  int i = colon + 1;
  while (i < lower.length() && (lower[i] == ' ' || lower[i] == '\"')) i++;

  int start = i;
  while (i < lower.length() && body[i] != '\"' && body[i] != ',' && body[i] != '}') i++;

  String timeStr = body.substring(start, i);
  int sep = timeStr.indexOf(':');
  if (sep < 0) return false;

  int hh = timeStr.substring(0, sep).toInt();
  int mm = timeStr.substring(sep + 1).toInt();
  if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return false;

  minutesOut = hh * 60 + mm;
  return true;
}

static void sendHttpResponse(WiFiClient& client, int code, const char* status, const String& body) {
  client.print("HTTP/1.1 ");
  client.print(code);
  client.print(" ");
  client.println(status);
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(body.length());
  client.println("Connection: close");
  client.println();
  client.print(body);
}

HttpConfigServer::HttpConfigServer(RuntimeConfig& cfg, uint16_t port)
  : _cfg(cfg), _server(port) {}

void HttpConfigServer::begin() {
  _server.begin();
  logInfo("HTTP", "Config server ready");
}

void HttpConfigServer::tick() {
  WiFiClient client = _server.available();
  if (!client) return;
  handleClient_(client);
  delay(10);
  client.stop();
}

void HttpConfigServer::handleClient_(WiFiClient& client) {
  unsigned long startWait = millis();
  while (client.connected() && !client.available() && millis() - startWait < 2000) {
    delay(1);
  }
  if (!client.available()) return;

  String reqLine = client.readStringUntil('\n');
  reqLine.trim();

  int sp1 = reqLine.indexOf(' ');
  int sp2 = reqLine.indexOf(' ', sp1 + 1);
  if (sp1 < 0 || sp2 < 0) {
    sendHttpResponse(client, 400, "Bad Request", "{\"error\":\"bad request line\"}");
    return;
  }

  String method = reqLine.substring(0, sp1);
  String path = reqLine.substring(sp1 + 1, sp2);

  int contentLength = 0;
  while (true) {
    String line = client.readStringUntil('\n');
    if (line == "\r" || line.length() == 0) break;
    line.trim();
    if (line.startsWith("Content-Length:")) {
      contentLength = line.substring(15).toInt();
    }
  }

  String body = "";
  if (contentLength > 0) {
    while ((int)body.length() < contentLength && client.connected()) {
      if (client.available()) body += (char)client.read();
    }
  }

  if (method == "PATCH" && path == "/temp") {
    handlePatchTemp_(body, client);
  } else if (method == "PATCH" && path == "/hours") {
    handlePatchHours_(body, client);
  } else {
    sendHttpResponse(client, 404, "Not Found", "{\"error\":\"unknown route\"}");
  }
}

void HttpConfigServer::handlePatchTemp_(const String& body, WiFiClient& client) {
  float minv = _cfg.tempMinC();
  float maxv = _cfg.tempMaxC();

  bool okMin = parseFloatField(body, "mintemp", minv);
  bool okMax = parseFloatField(body, "maxtemp", maxv);
  if (!okMin && !okMax) {
    sendHttpResponse(client, 400, "Bad Request", "{\"error\":\"mintemp/maxtemp missing\"}");
    return;
  }

  _cfg.setTempRangeC(minv, maxv);

  String resp = "{\"tempMin\":";
  resp += _cfg.tempMinC();
  resp += ",\"tempMax\":";
  resp += _cfg.tempMaxC();
  resp += "}";

  sendHttpResponse(client, 200, "OK", resp);
  logInfo("HTTP", String("Temp updated min=") + _cfg.tempMinC() + " max=" + _cfg.tempMaxC());
}

static bool updateDaySchedule(RuntimeConfig& cfg, int dayIndex, int startMin, int endMin) {
  if (startMin < 0 || endMin <= startMin) return false;
  cfg.setScheduleForDow(dayIndex, DaySchedule{startMin, endMin});
  return true;
}

void HttpConfigServer::handlePatchHours_(const String& body, WiFiClient& client) {
  struct DayMap { const char* key; int idx; };
  const DayMap map[] = {
    {"sunday", 0}, {"monday", 1}, {"tuesday", 2}, {"wednesday", 3},
    {"thursday", 4}, {"friday", 5}, {"saturday", 6}
  };

  bool updated = false;
  for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
    String startKey = String(map[i].key) + "start";
    String endKey = String(map[i].key) + "end";

    int s, e;
    bool hasStart = parseTimeField(body, startKey, s);
    bool hasEnd = parseTimeField(body, endKey, e);

    if (hasStart && hasEnd) {
      if (updateDaySchedule(_cfg, map[i].idx, s, e)) {
        updated = true;
      } else {
        logInfo("HTTP", String("Invalid hours for ") + map[i].key);
      }
    } else if (hasStart || hasEnd) {
      logInfo("HTTP", String("Incomplete hours for ") + map[i].key);
    }
  }

  if (!updated) {
    sendHttpResponse(client, 400, "Bad Request", "{\"error\":\"no valid start/end pairs\"}");
    return;
  }

  const char* names[] = {"sunday","monday","tuesday","wednesday","thursday","friday","saturday"};
  String resp = "{";
  for (int i = 0; i < 7; i++) {
    DaySchedule d = _cfg.scheduleForDow(i);
    int sh = d.startMin / 60;
    int sm = d.startMin % 60;
    int eh = d.endMin / 60;
    int em = d.endMin % 60;

    resp += "\"";
    resp += names[i];
    resp += "Start\":\"";
    if (sh < 10) resp += "0";
    resp += sh;
    resp += ":";
    if (sm < 10) resp += "0";
    resp += sm;
    resp += "\"";

    resp += ",\"";
    resp += names[i];
    resp += "End\":\"";
    if (eh < 10) resp += "0";
    resp += eh;
    resp += ":";
    if (em < 10) resp += "0";
    resp += em;
    resp += "\"";

    if (i != 6) resp += ",";
  }
  resp += "}";

  sendHttpResponse(client, 200, "OK", resp);
  logInfo("HTTP", "Hours updated");
}

