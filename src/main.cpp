#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Display.h"
#include "Clock.h"
#include "Player.h"

void IRAM_ATTR ISR();
void turnOffAlarm();

#define ANALOG_IN A0 // NC
#define GPIO0 0      // NC
#define GPIO1 1      // TX Serial
#define RTC_ALARM 6
#define BTN_SNOOZE 7
#define BTN_MODE 8
#define BTN_ALARMS 9
#define BTN_SET 10
#define BTN_UP 11
#define BTN_DOWN 12
#define BTN_PLAY 13 // ??
#define LED_ALARM1 15
#define LED_ALARM2 16

#define INTERVAL_SNOOZE 600000 // 10min
#define INTERVAL_DISPLAY 5000  // 5sec
#define INTERVAL_CLOCK 1000    // 1sec

volatile bool flags[8] = {false, false, false, false, false, false, false, false};
unsigned long tsSnooze;
unsigned long tsMod;
bool snoozeEnable = false;
bool alarming = false;
bool displayed = false;
bool blink = false;
uint8_t vol = 20;
RgbColor dispColor = WHITE;
RtcDateTime tempDateTime;

void setup()
{
    // Init Serial
    Serial.begin(9600);
    while (!Serial)
        ; // wait for serial attach
    //Serial.setTimeout(50);
    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    // Turn off WiFi
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);

    // Init LED strip
    initDisplay();
    displayMod = CLOCK;

    // Init RTC Module
    Clock.Begin(2, 14);
    //Wire.begin(2, 14);
    //Clock.Begin();

    // Init MP3 Player
    Player.init(6, 5, 4);
    Player.setVolume(vol);

    // Init IOs
    for (size_t pin = RTC_ALARM; pin <= BTN_PLAY; pin++)
    {
        pinMode(pin, INPUT_PULLUP);
        attachInterrupt(pin, ISR, FALLING);
    }
    pinMode(LED_ALARM1, OUTPUT);
    pinMode(LED_ALARM2, OUTPUT);
    // ETS_GPIO_INTR_DISABLE();
    // for (size_t pin = RTC_ALARM; pin <= BTN_PLAY; pin++)
    // {
    //     pinMode(pin, INPUT_PULLUP);
    //     GPC(pin) &= ~(0xF << GPCI);            // INT mode disabled
    //     GPIEC = (1 << pin);                    // Clear Interrupt for this pin
    //     GPC(pin) |= ((FALLING & 0xF) << GPCI); // INT mode "FALLING"
    //     GPIE = (1 << pin);                     // Interrupt Enable
    // }
    // ETS_GPIO_INTR_ENABLE();
    // attachInterrupt(digitalPinToInterrupt(RTC_ALARM), alarmISR, FALLING);
    // attachInterrupt(digitalPinToInterrupt(BTN_SNOOZE), alarmISR, FALLING);
    // attachInterrupt(digitalPinToInterrupt(BTN_MODE), alarmISR, FALLING);
    // attachInterrupt(digitalPinToInterrupt(BTN_ALARMS), alarmISR, FALLING);
    // attachInterrupt(digitalPinToInterrupt(BTN_SET), alarmISR, FALLING);
    // attachInterrupt(digitalPinToInterrupt(BTN_UP), alarmISR, FALLING);
    // attachInterrupt(digitalPinToInterrupt(BTN_DOWN), alarmISR, FALLING);
    // attachInterrupt(digitalPinToInterrupt(BTN_PLAY), alarmISR, FALLING);

    Serial.println("Running...");
}

void loop()
{
    /////   Handle Display   /////
    switch (displayMod)
    {
    case CLOCK:
        if ((millis() - tsMod) > INTERVAL_CLOCK)
        {
            blink = !blink;
            displayDigits(clockToDigits(getClockTime()), blink, blink, dispColor, dispColor);
            tsMod = millis();
        }
        break;
    case DATE:
        if (!displayed)
        {
            displayDigits(dateToDigits(getClockTime()), false, false, dispColor, dispColor);
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case YEAR:
        if (!displayed)
        {
            displayDigits(dateToDigits(getClockTime(), true), false, false, dispColor, dispColor);
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case TEMP:
        if (!displayed)
        {
            displayDigits(tempToDigits(Clock.GetTemperature().AsFloatDegC()), false, true, dispColor, dispColor);
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case ALARM1:
        if (!displayed)
        {
            DS3231AlarmOne al = Clock.GetAlarmOne();
            displayDigits(nbToDigits(al.Hour(), al.Minute()), true, true, dispColor, dispColor);
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case ALARM2:
        if (!displayed)
        {
            DS3231AlarmTwo al = Clock.GetAlarmTwo();
            displayDigits(nbToDigits(al.Hour(), al.Minute()), true, true, dispColor, dispColor);
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case SET_VOLUME:
        if (!displayed)
        {
            displayDigits(nbToDigits(0, vol), false, true, OFF, dispColor);
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    default:
        displayMod = CLOCK;
        break;
    }

    /////   Handle Interrupts   /////
    if (flags[0]) // RTC Alarm
    {
        Player.playFile(0);
        displayMisc(WHITE);
        flags[0] = false;
        alarming = true;
        Clock.LatchAlarmsTriggeredFlags();
    }
    if (flags[1]) // Snooze
    {
        turnOffAlarm();
        tsSnooze = millis();
        snoozeEnable = true;
    }
    if (flags[2]) // Mode
    {
        if (alarming)
            turnOffAlarm();
        else if (displayMod == CLOCK)
            displayMod = DATE;
        else if (displayMod == DATE)
            displayMod = YEAR;
        else if (displayMod == YEAR)
            displayMod = TEMP;
        else if (displayMod == TEMP || displayMod == ALARM1 || displayMod == ALARM2)
            displayMod = CLOCK;
        displayed = false;
        tsMod = millis();
    }
    if (flags[3]) // Alarms
    {
        if (alarming)
            turnOffAlarm();
        else if (displayMod == CLOCK || displayMod == DATE || displayMod == YEAR || displayMod == TEMP)
            displayMod = ALARM1;
        else if (displayMod == ALARM1)
            displayMod = ALARM2;
        else if (displayMod == ALARM2)
            displayMod = CLOCK;
        displayed = false;
        tsMod = millis();
    }
    if (flags[4]) // SET
    {
        if (alarming)
            turnOffAlarm();
        else if (displayMod == CLOCK)
            displayMod = SET_CLOCK;
        else if (displayMod == DATE)
            displayMod = SET_DATE;
        else if (displayMod == YEAR)
            displayMod = SET_YEAR;
        else if (displayMod == ALARM1)
            displayMod = SET_ALARM1;
        else if (displayMod == ALARM2)
            displayMod = SET_ALARM2;
    }
    if (flags[5]) // UP
    {
        if (alarming)
            turnOffAlarm();
        else if (displayMod == CLOCK)
            displayMod = SET_VOLUME;
        else if (displayMod == SET_VOLUME && vol != VOL_MAX)
        {
            vol++;
            displayed = false;
            tsMod = millis();
        }
        else if (displayMod == SET_ALARM1)
            ;
        else if (displayMod == SET_ALARM2)
            ;
        else if (displayMod == SET_CLOCK)
            ;
        else if (displayMod == SET_DATE)
            ;
        else if (displayMod == SET_YEAR)
            ;
    }
    if (flags[6]) // DOWN
    {
        if (alarming)
            turnOffAlarm();
    }
    if (flags[7]) // PLAY
    {
        if (alarming)
            turnOffAlarm();
    }

    if (snoozeEnable && ((millis() - tsSnooze) > INTERVAL_SNOOZE))
    {
        flags[0] = 1;
        snoozeEnable = false;
    }
}

void turnOffAlarm()
{
    if (Player.isBusy())
        Player.stop();
    displayMisc(OFF);
}

void IRAM_ATTR ISR()
{
    uint16_t GPIFs = GPIEC;
    for (size_t i = RTC_ALARM; i <= BTN_PLAY; i++)
    {
        if ((GPIFs >> i) & 0x01)
            flags[i] = 1;
    }
}
