#include <Arduino.h>

#include <SPIFFS.h>
#include <WiFiSettings.h>

#include <SPI.h>
#include <TFT_eSPI.h>

const int pin_backlight = 4;


static TFT_eSPI display;
static TFT_eSprite sprite(&display);

static void clear_sprite(int bg = TFT_BLACK)
{
    sprite.fillSprite(bg);
    sprite.drawRect(0, 0, display.width(), display.height(), TFT_BLUE);
}

static void display_big(const String & text, int fg = TFT_WHITE, int bg = TFT_BLACK)
{
    clear_sprite(bg);
    sprite.setTextSize(1);
    bool nondigits = false;
    for (int i = 0; i < text.length(); i++) {
        char c = text.charAt(i);
        if (c < '0' || c > '9')
            nondigits = true;
    }
    sprite.setTextFont(nondigits ? 4 : 8);
    sprite.setTextSize(nondigits && text.length() < 10 ? 2 : 1);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(fg, bg);
    sprite.drawString(text, display.width() / 2, display.height() / 2);

    sprite.pushSprite(0, 0);
}

void setup(void)
{
    Serial.begin(115200);

    digitalWrite(pin_backlight, HIGH);
    display.init();
    display.fillScreen(TFT_BLACK);
    display.setRotation(1);

    sprite.createSprite(display.width(), display.height());

    display_big("Hello world!");
}

void loop(void)
{
}
