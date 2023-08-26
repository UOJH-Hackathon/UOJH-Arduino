#include <Arduino.h>
#include <ESP8266WIFI.h>
#include <WebSocketsClient.h>
#include <LiquidCrystal_I2C.h>
#include "DHTesp.h"

/* ################################################################# */

#define WIFI_SSID "iPhone (4)"
#define WIFI_PW   "12345678"

#define WEBSOCKET_HOST  "172.20.10.2" 

#define I2C_ADDRESS  0x27

#define VO_PIN       A0
#define V_PIN        Pin::D2
#define FAN_PIN      Pin::D3
#define DHT_PIN      Pin::D4
#define R_LED_PIN    Pin::D5
#define Y_LED_PIN    Pin::D6
#define G_LED_PIN    Pin::D7

/* ################################################################# */

class Pin {
public:
    static const uint8_t D2   = 16;
    static const uint8_t D3   = 5;
    static const uint8_t D4   = 4;
    static const uint8_t D5   = 14;
    static const uint8_t D6   = 12;
    static const uint8_t D7   = 13;
    static const uint8_t D8   = 0;
    static const uint8_t D9   = 2;
    static const uint8_t D10  = 15;
    static const uint8_t D11  = 13;
    static const uint8_t D12  = 12;
    static const uint8_t D13  = 14;
    static const uint8_t D14  = 4;
    static const uint8_t D15  = 5;
};

class Logger {
public:
    Logger() {
        Serial.begin(115200);
        delay(1000);
        Serial.println();
    }

    void info(String msg, bool ln = true) {
        Serial.print("[Info]  " + String(msg) + (ln ? '\n' : ' '));
    }

    void warn(String msg, bool ln = true) {
        Serial.print("[Warn]  " + String(msg) + (ln ? '\n' : ' '));
    }

    void error(String msg, bool ln = true) {
        Serial.print("[Error] " + String(msg) + (ln ? '\n' : ' '));
    }

    void print(String msg) {
        Serial.print(msg);
    }

    void nextLine() {
        Serial.print('\n');
    }
};

/* ################################################################# */

Logger logger;
WebSocketsClient webSocket;

LiquidCrystal_I2C lcd(I2C_ADDRESS, 16, 2);
DHTesp dht;

/* ################################################################# */

void wifi_init();
void websocket_init();

void on_message(String);

void lcd_init();

void refresh_temperature();
void refresh_humidity();
void refresh_air_quality();

void handler_led();
void handler_lcd();
void handler_fan();

/* ################################################################# */

float humidity    = 50;
float temperature = 24;
int air_quality   = 10;

bool is_fan_auto = true;
bool is_fan_on   = false;

/* ################################################################# */

void setup() {
    // WiFi 연결
    wifi_init();        

    // Socket 연결 및 설정
    websocket_init();
    webSocket.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
        switch (type) {
            case WStype_CONNECTED:
                webSocket.sendTXT("auth:arduino");
                logger.info("WebSocket Connected.");
                break;

            case WStype_DISCONNECTED:
                logger.error("WebSocket is disconnected. Please check your server status.");
                delay(5000);
                break;

            case WStype_TEXT:
                logger.info("[Socket In] " + String((char *) payload));
                on_message(String((char *) payload));
                break;
                
            default:
                break;
        }
    });

    // lcd 초기화
    lcd_init();

    // 핀모드 설정
    pinMode(V_PIN,  OUTPUT);
    pinMode(VO_PIN, INPUT);

    pinMode(FAN_PIN, OUTPUT);

    pinMode(R_LED_PIN, OUTPUT);
    pinMode(Y_LED_PIN, OUTPUT);
    pinMode(G_LED_PIN, OUTPUT);

    // DHT Init
    dht.setup(D8, DHTesp::DHT11);
}

void loop() {
    webSocket.loop();

    refresh_temperature();
    refresh_humidity();
    refresh_air_quality();
    
    handler_led();
    // handler_lcd();
    handler_fan();
}

/* ################################################################# */

void wifi_init() {
    logger.info("############### WiFi ###############");
    logger.info("Connecting to " + String(WIFI_SSID), false);

    WiFi.begin(WIFI_SSID, WIFI_PW);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        logger.print(".");
    }
    logger.nextLine();
    
    logger.info("Wi-Fi connected. (IP: " + WiFi.localIP().toString() + ")");
    logger.info("####################################");
    logger.nextLine();
}

void websocket_init() {
    logger.info("############### WebSocket ###############");
    logger.info("Connecting to " + String(WEBSOCKET_HOST), false);

    webSocket.begin(WEBSOCKET_HOST, 80, "/ws");

    while (!webSocket.isConnected()) {
        delay(500);
        logger.print(".");
        webSocket.loop();
    }
    logger.nextLine();

    webSocket.sendTXT("auth:arduino");
    
    logger.info("Websocket connected.");
    logger.info("####################################");
    logger.nextLine();
}



void on_message(String msg) {
    if (msg == "request") {
        String response = "";
        
        response = "from_arduino:temperature:" + String(temperature);
        webSocket.sendTXT(response);

        response = "from_arduino:humidity:" + String(humidity);
        webSocket.sendTXT(response);

        response = "from_arduino:air_quality:" + String(air_quality);
        webSocket.sendTXT(response);

        response = "from_arduino:is_fan_auto:" + String((is_fan_auto ? "true" : "false"));
        webSocket.sendTXT(response);

        response = "from_arduino:is_fan_on:" + String((is_fan_on ? "true" : "false"));
        webSocket.sendTXT(response);
    } else if (msg == "auto_on") {
        is_fan_auto = true;
    } else if (msg == "auto_off") {
        is_fan_auto = false;
    } else if (msg == "fan_on") {
        is_fan_on = true;
    } else if (msg == "fan_off") {
        is_fan_on = false;
    }
}



void lcd_init() {
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Remote Controlled");
    lcd.setCursor(0, 1);
    lcd.print("Air Purifier");
}



void refresh_temperature() {
    float new_temperature = dht.getTemperature();
    if (new_temperature != temperature) {
        temperature = new_temperature;

        String response = "from_arduino:humidity:" + String(temperature);
        webSocket.sendTXT(response);
    }
}

void refresh_humidity() {
    float new_humidity = dht.getHumidity();
    if (new_humidity != humidity) {
        humidity = new_humidity;

        String response = "from_arduino:humidity:" + String(humidity);
        webSocket.sendTXT(response);
    }
}

void refresh_air_quality() {
    static unsigned long previousMillis_air = 0;

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis_air >= 1000) {
        previousMillis_air = currentMillis;

        digitalWrite(V_PIN, LOW);
        delayMicroseconds(280);
        float vo_value = analogRead(VO_PIN);
        delayMicroseconds(40);
        digitalWrite(V_PIN, HIGH);
        delayMicroseconds(9680);

        float voltage = vo_value * 5.0 / 1024.0;
        float new_air_quality = 0;

        if (voltage >= 0.6) {
            new_air_quality = 172 * voltage - 103.2;
        }

        if (new_air_quality != air_quality) {
            air_quality = new_air_quality;

            String response = "from_arduino:air_quality:" + String(air_quality);
            webSocket.sendTXT(response);
        }
    }
}



void handler_led() {
    if (air_quality < 50) {
        digitalWrite(R_LED_PIN, LOW);
        digitalWrite(Y_LED_PIN, LOW);
        digitalWrite(G_LED_PIN, HIGH);
    } else if (air_quality < 80) {
        digitalWrite(R_LED_PIN, LOW);
        digitalWrite(Y_LED_PIN, HIGH);
        digitalWrite(G_LED_PIN, LOW);
    } else {
        digitalWrite(R_LED_PIN, HIGH);
        digitalWrite(Y_LED_PIN, LOW);
        digitalWrite(G_LED_PIN, LOW);
    }
}

void handler_lcd() {
    // lcd.setCursor(6, 0);
    // lcd.print(temperature);
    // lcd.setCursor(6, 1);
    // lcd.print(humidity);
}

void handler_fan() {
    if (is_fan_auto) {
        digitalWrite(FAN_PIN, air_quality >= 50);
    } else {
        digitalWrite(FAN_PIN, is_fan_on);
    }
}