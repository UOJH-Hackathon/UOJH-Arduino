#include <Arduino.h>
#include <ESP8266WiFi.h>

void setup() {
    Serial.begin(115200);

    Serial.println();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
}

void loop() {
    int networksFound = WiFi.scanNetworks();

    if (networksFound == 0) {
        Serial.println("No Wi-Fi networks found.");
    } else {
        Serial.println(String(networksFound) + " Wi-Fi networks found:");

        for (int i = 0; i < networksFound; i++) {
            Serial.println("  - " + String(i+1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)");
        }

        Serial.println();
    }

    delay(10000);
}
