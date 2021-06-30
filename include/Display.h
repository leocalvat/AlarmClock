
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <NeoPixelBus.h>
#include <RtcDS3231.h>

void initDisplay();
void fillDisplay(RgbColor color);
void displayClock(RtcDateTime &t, RgbColor color);
void displayDate(RtcDateTime &t, RgbColor color, bool year);
void displayTemp(RtcTemperature &t, RgbColor color);
void displayMisc(RgbColor color);

#define TOTAL_LED_COUNT 62 // has to be the number of LEDs (56 + 2 + 4)
#define DIGIT_LED_COUNT 14 // 7 seg * 2 LED
#define CLOCK_LED_COUNT 58 // 4 digit + 2 dot
#define MISC_LED_COUNT 4

#define WHITE RgbColor(50, 50, 50)

enum DisplayMods
{
    CLOCK,
    DATE,
    YEAR,
    ALARM1,
    ALARM2,
    VOLUME,
    TEMP
} displayMod;

NeoPixelBus<NeoRgbFeature, NeoWs2813Method> Strip(TOTAL_LED_COUNT); // All the available methods here https://github.com/Makuna/NeoPixelBus/wiki/ESP8266-NeoMethods

bool digits[12][14] = {{true, true, true, true, true, true, true, true, true, true, true, true, false, false},             // zero
                       {false, false, false, false, true, true, true, true, false, false, false, false, false, false},     // one
                       {false, false, true, true, true, true, false, false, true, true, true, true, true, true},           // two
                       {false, false, true, true, true, true, true, true, true, true, false, false, true, true},           // tree
                       {true, true, false, false, true, true, true, true, false, false, false, false, true, true},         // four
                       {true, true, true, true, false, false, true, true, true, true, false, false, true, true},           // five
                       {true, true, true, true, false, false, true, true, true, true, true, true, true, true},             // six
                       {false, false, true, true, true, true, true, true, false, false, false, false, false, false},       // seven
                       {true, true, true, true, true, true, true, true, true, true, true, true, true, true},               // eight
                       {true, true, true, true, true, true, true, true, true, true, false, false, true, true},             // nine
                       {false, false, false, false, false, false, false, false, false, false, false, false, false, false}, // off
                       {true, true, true, true, false, false, false, false, true, true, true, true, false, false}};        // °C

void initDisplay()
{
    Strip.Begin();
    Strip.Show();
    fillDisplay(RgbColor(50, 0, 0));
    delay(500);
    fillDisplay(RgbColor(0, 50, 0));
    delay(500);
    fillDisplay(RgbColor(0, 0, 50));
    delay(500);
    fillDisplay(RgbColor(50, 50, 50));
    delay(500);
}

void displayClock(const RtcDateTime &t, RgbColor color, bool stopDots = false)
{
    uint8_t index = 0;
    uint8_t d[4];
    d[0] = uint8_t(t.Hour() / 10);
    d[1] = uint8_t(t.Hour() % 10);
    d[2] = uint8_t(t.Minute() / 10);
    d[3] = uint8_t(t.Minute() % 10);
    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < DIGIT_LED_COUNT; j++)
        {
            if (digits[d[i]][j])
                Strip.SetPixelColor(index, color);
            else
                Strip.SetPixelColor(index, RgbColor(0));
            index++;
        }
    }
    if (t.Second() % 2 == 0)
    {
        Strip.SetPixelColor(index, color);
        Strip.SetPixelColor(index + 1, color);
    }
    else
    {
        Strip.SetPixelColor(index, RgbColor(0));
        Strip.SetPixelColor(index + 1, RgbColor(0));
    }
    Strip.Show();
}

void displayDate(const RtcDateTime &t, RgbColor color, bool year = false)
{
    uint8_t index = 0;
    uint8_t d[4];
    if (!year)
    {
        d[0] = uint8_t(t.Day() / 10);
        d[1] = uint8_t(t.Day() % 10);
        d[2] = uint8_t(t.Month() / 10);
        d[3] = uint8_t(t.Month() % 10);
    }
    else
    {
        d[0] = uint8_t((t.Year() / 1000) % 10);
        d[1] = uint8_t((t.Year() / 100) % 10);
        d[2] = uint8_t((t.Year() / 10) % 10);
        d[3] = uint8_t(t.Year() % 10);
    }

    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < DIGIT_LED_COUNT; j++)
        {
            if (digits[d[i]][j])
                Strip.SetPixelColor(index, color);
            else
                Strip.SetPixelColor(index, RgbColor(0));
            index++;
        }
    }
    Strip.SetPixelColor(index, RgbColor(0));
    Strip.SetPixelColor(index + 1, RgbColor(0));
    Strip.Show();
}

void displayTemp(RtcTemperature t, RgbColor color)
{
    uint8_t index = 0;
    uint8_t d[4];
    d[0] = uint8_t((int)t.AsFloatDegC() / 10);
    d[1] = uint8_t((int)t.AsFloatDegC() % 10);
    d[2] = 11; // °C
    d[3] = 10; // off
    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < DIGIT_LED_COUNT; j++)
        {
            if (digits[d[i]][j])
                Strip.SetPixelColor(index, color);
            else
                Strip.SetPixelColor(index, RgbColor(0));
            index++;
        }
    }
    Strip.SetPixelColor(index, color);
    Strip.SetPixelColor(index + 1, RgbColor(0));
    Strip.Show();
}

void displayMisc(RgbColor color)
{
    uint8_t index = CLOCK_LED_COUNT;
    for (size_t i = 0; i < MISC_LED_COUNT; i++)
        Strip.SetPixelColor(index + i, color);
    Strip.Show();
}

void fillDisplay(RgbColor color)
{
    Strip.ClearTo(color);
    Strip.Show();
}

#endif // __DISPLAY_H__