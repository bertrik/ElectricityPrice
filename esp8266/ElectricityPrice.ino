#include <Arduino.h>
#include <ESP8266HTTPClient.h>

#include "WiFiManager.h"
#include "TM1637Display.h"
#include "ArduinoJson.h"

#define PIN_TM1637_CLK D3
#define PIN_TM1637_DIO D4

static TM1637Display display(PIN_TM1637_CLK, PIN_TM1637_DIO);
static WiFiManager wifiManager;
static WiFiClient wifiClient;
static char espid[64];

static bool fetch_url(const char *host, int port, const char *path, String & response)
{
    HTTPClient httpClient;
    httpClient.begin(wifiClient, host, port, path, false);
    httpClient.setTimeout(20000);
    httpClient.setUserAgent(espid);

    printf("> GET http://%s:%d%s\n", host, port, path);
    int res = httpClient.GET();

    // evaluate result
    bool result = (res == HTTP_CODE_OK);
    response = result ? httpClient.getString() : httpClient.errorToString(res);
    httpClient.end();
    printf("< %d: %s\n", res, response.c_str());
    return result;
}

static bool fetch_price(void)
{
    DynamicJsonDocument doc(2048);
    String response;

    // fetch
    if (fetch_url("stofradar.nl", 9001, "/electricity/price", response)) {
        // decode
        if (deserializeJson(doc, response) == DeserializationError::Ok) {
            float current_price = doc["current"]["price"];

            int number = (int) current_price;
            display.showNumberDec(number);
            return true;
        }
    }
    return false;
}

void setup(void)
{
    snprintf(espid, sizeof(espid), "esp8266-elecprice-%06x", ESP.getChipId());

    Serial.begin(115200);
    Serial.println("\nElectricityPrice");

    // init display
    display.setBrightness(15);

    // connect to wifi
    printf("Starting WIFI manager (%s)...\n", WiFi.SSID().c_str());
    wifiManager.setConfigPortalTimeout(120);
    wifiManager.autoConnect("ESP-ELECPRICE");
}

void loop(void)
{
    static int last_period = -1;

    int period = millis() / 300000;
    if (period != last_period) {
        last_period = period;
        fetch_price();
    }
}
