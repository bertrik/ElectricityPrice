#include <Arduino.h>

#include "SPIFFS.h"
#include "WiFiSettings.h"
#include "HTTPClient.h"

#include "ArduinoJson.h"

#include "SPI.h"
#include "TFT_eSPI.h"

static const int pin_backlight = 4;
static TFT_eSPI display;
static TFT_eSprite sprite(&display);
static char espid[32];

static void display_big(const String & text, int fg = TFT_WHITE, int bg = TFT_BLACK, int border =
                        TFT_BLUE)
{
    Serial.println(text);

    sprite.fillSprite(bg);
    sprite.drawRect(0, 0, display.width(), display.height(), border);
    sprite.setTextFont(4);
    sprite.setTextSize(2);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(fg, bg);
    sprite.drawString(text, display.width() / 2, display.height() / 2);

    sprite.pushSprite(0, 0);
}

static bool fetch_url(String host, int port, String path, String & response)
{
    HTTPClient httpClient;
    httpClient.begin(host, port, path);
    httpClient.setTimeout(20000);
    httpClient.setUserAgent(espid);

    printf("> GET http://%s:%d%s\n", host.c_str(), port, path.c_str());
    int res = httpClient.GET();

    // evaluate result
    bool result = (res == HTTP_CODE_OK);
    response = result ? httpClient.getString() : httpClient.errorToString(res);
    httpClient.end();
    printf("< %d: %s\n", res, response.c_str());
    return result;
}

static int compare_float(const void *v1, const void *v2)
{
    float *f1 = (float *) v1;
    float *f2 = (float *) v2;
    return (*f1 < *f2) ? -1 : (*f1 > *f2) ? 1 : 0;
}

static bool calculate_quartiles(JsonDocument & doc, float &q1, float &q2, float &q3)
{
    float prices[24];
    int index = 0;
    for (JsonObject item:doc["day-ahead"].as < JsonArray > ()) {
        float price = item["price"];
        if (index < 24) {
            prices[index++] = price;
        }
    }

    qsort(prices, 24, sizeof(float), compare_float);
    q1 = prices[5];
    q2 = prices[11];
    q3 = prices[17];
    return true;
}

void setup(void)
{
    Serial.begin(115200);

    snprintf(espid, sizeof(espid), "esp32-nlelecprice");
    Serial.println(espid);

    SPIFFS.begin(true);

    digitalWrite(pin_backlight, HIGH);
    display.init();
    display.fillScreen(TFT_BLACK);
    display.setRotation(1);

    sprite.createSprite(display.width(), display.height());

    WiFiSettings.begin();
    WiFiSettings.connect();
}

void loop(void)
{
    unsigned long ms = millis();

    static unsigned long last_period = -1;
    int period = ms / 300000;
    if (period != last_period) {
        DynamicJsonDocument doc(2048);
        String response;
        last_period = period;
        fetch_url("stofradar.nl", 9001, "/electricity/price", response);

        // decode
        if (deserializeJson(doc, response) == DeserializationError::Ok) {
            float price = doc["current"]["price"];
            float q1, q2, q3;
            calculate_quartiles(doc, q1, q2, q3);
            printf("q1=%f,q2=%f,q3=%f\n", q1, q2, q3);

            int border = (price <= q1) ? TFT_GREEN : (price >= q3) ? TFT_RED : TFT_BLUE;

            char buffer[16];
            sprintf(buffer, "E%.2f", price / 1000.0);
            display_big(String(buffer), TFT_WHITE, TFT_BLACK, border);
        }
    }
}
