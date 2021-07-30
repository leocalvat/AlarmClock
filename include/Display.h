
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <NeoPixelBus.h>
#include <RtcDS3231.h>

void initDisplay();
void fillDisplay(RgbColor color);
uint8_t *clockToDigits(const RtcDateTime &t);
uint8_t *nbToDigits(uint8_t hh, uint8_t mm);
uint8_t *dateToDigits(const RtcDateTime &t, bool year);
uint8_t *tempToDigits(const float &t);
void displayDigits(uint8_t *d, bool dotL, bool dotH, RgbColor colorL, RgbColor colorR);
void displayMisc(RgbColor color);

#define TOTAL_LED_COUNT 62 // has to be the number of LEDs (56 + 2 + 4)
#define DIGIT_LED_COUNT 14 // 7 seg * 2 LED
#define CLOCK_LED_COUNT 58 // 4 digit + 2 dot
#define MISC_LED_COUNT 4
#define DIGIT_COUNT 4 // HH:MM

#define WHITE RgbColor(150, 150, 150)
#define OFF RgbColor(0)

uint8_t d[DIGIT_COUNT];

enum DisplayMods
{
    CLOCK,
    DATE,
    YEAR,
    ALARM1,
    ALARM2,
    TEMP,
    SET_HOUR,
    SET_MIN,
    SET_DAY,
    SET_MONTH,
    SET_YEAR,
    SET_ALARM1_HOUR,
    SET_ALARM1_MIN,
    SET_ALARM2_HOUR,
    SET_ALARM2_MIN,
    SET_VOLUME
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

uint8_t *clockToDigits(const RtcDateTime &t)
{
    d[0] = uint8_t(t.Hour() / 10);
    d[1] = uint8_t(t.Hour() % 10);
    d[2] = uint8_t(t.Minute() / 10);
    d[3] = uint8_t(t.Minute() % 10);
    return d;
}

uint8_t *nbToDigits(uint8_t nbL, uint8_t nbR)
{
    d[0] = uint8_t(nbL / 10);
    d[1] = uint8_t(nbL % 10);
    d[2] = uint8_t(nbR / 10);
    d[3] = uint8_t(nbR % 10);
    return d;
}

uint8_t *dateToDigits(const RtcDateTime &t, bool year = false)
{
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
    return d;
}

uint8_t *tempToDigits(const float &t)
{
    d[0] = uint8_t((int)t / 10);
    d[1] = uint8_t((int)t % 10);
    d[2] = 11; // °C
    d[3] = 10; // off
    return d;
}

void displayDigits(uint8_t *d, bool dotL, bool dotH, RgbColor colorL, RgbColor colorR)
{
    RgbColor color = colorL;
    uint8_t index = 0;
    for (size_t i = 0; i < DIGIT_COUNT; i++)
    {
        for (size_t j = 0; j < DIGIT_LED_COUNT; j++)
        {
            if (digits[d[i]][j])
                Strip.SetPixelColor(index, color);
            else
                Strip.SetPixelColor(index, OFF);
            index++;
        }
        if (i == 1)
            color = colorR;
    }
    if (dotL)
        Strip.SetPixelColor(index, colorL);
    else
        Strip.SetPixelColor(index, OFF);
    if (dotH)
        Strip.SetPixelColor(index + 1, colorR);
    else
        Strip.SetPixelColor(index + 1, OFF);
    Strip.Show();
    Serial.print("Display digits " + d[0] + d[1] + ':' + d[2] + d[3]);
}

void displayMisc(RgbColor color)
{
    uint8_t index = CLOCK_LED_COUNT;
    for (size_t i = 0; i < MISC_LED_COUNT; i++)
        Strip.SetPixelColor(index + i, color);
    Strip.Show();
    Serial.print("Display misc");
}

void fillDisplay(RgbColor color)
{
    Strip.ClearTo(color);
    Strip.Show();
}

#endif // __DISPLAY_H__