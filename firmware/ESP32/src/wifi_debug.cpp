#include "wifi_debug.h"

#include <WiFi.h>
#include <WebServer.h>

const char* WIFI_SSID = "FiberHGW_HULCH1";
const char* WIFI_PASSWORD = "xuHLUuKcXp3w";
WebServer server(80);

String logs[100];
int logIndex = 0;

void debugLog(const String &message)
{
    String line = "[" + String(millis()) + " ms] " + message;

    Serial.println(line);

    logs[logIndex] = line;
    logIndex = (logIndex + 1) % 100;
}

void handleRoot()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta http-equiv="refresh" content="2">
<title>ESP32 Debug</title>
<style>
body {
    background: #111;
    color: #0f0;
    font-family: monospace;
    padding: 20px;
}
pre {
    white-space: pre-wrap;
    font-size: 16px;
}
</style>
</head>
<body>
<h2>ESP32 UAV DEBUG</h2>
<pre>
)rawliteral";

    for (int i = 0; i < 100; i++)
    {
        int index = (logIndex + i) % 100;

        if (logs[index].length() > 0)
            html += logs[index] + "\n";
    }

    html += R"rawliteral(
</pre>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

void initWiFi()
{
    debugLog("Wi-Fi baslatiliyor...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 15000)
    {
        delay(250);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        debugLog("Wi-Fi BAGLANDI");
        debugLog("SSID: " + WiFi.SSID());
        debugLog("IP: " + WiFi.localIP().toString());
        debugLog("RSSI: " + String(WiFi.RSSI()) + " dBm");

        server.on("/", handleRoot);

        server.begin();

        debugLog("HTTP SERVER BASLADI");
        debugLog("Tarayicidan ac: http://" + WiFi.localIP().toString());
    }
    else
    {
        debugLog("Wi-Fi BAGLANAMADI");
    }
}

void updateWiFi()
{
    server.handleClient();
}