#include <string.h>

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

static void display_big(const String & text, int fg = TFT_WHITE, int bg = TFT_BLACK)
{
    sprite.fillSprite(bg);
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

static void calculate_quartiles(JsonDocument & doc, float &q1, float &q2, float &q3)
{
    float prices[24];

    // convert prices into array
    int index = 0;
    int num_elems = sizeof(prices) / sizeof(*prices);
    for (JsonObject item:doc["day-ahead"].as < JsonArray > ()) {
        float price = item["price"];
        if (index < num_elems) {
            prices[index++] = price;
        }
    }
    // pad with 0 if fewer than 24
    while (index < num_elems) {
        prices[index++] = 0.0;
    }

    qsort(prices, num_elems, sizeof(float), compare_float);
    q1 = (prices[5] + prices[6]) / 2;
    q2 = (prices[11] + prices[12]) / 2;
    q3 = (prices[17] + prices[18]) / 2;
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
            printf("q1=%.2f,q2=%.2f,q3=%.2f\n", q1, q2, q3);

            int bg, fg;
            if (price < q1) {
                bg = TFT_GREEN;
                fg = TFT_BLACK;
            } else if (price > q3) {
                bg = TFT_RED;
                fg = TFT_WHITE;
            } else {
                bg = TFT_BLACK;
                fg = TFT_WHITE;
            }

            char buffer[16];
            sprintf(buffer, "E%.2f", price / 1000.0);
            display_big(String(buffer), fg, bg);
        }
    }
}
