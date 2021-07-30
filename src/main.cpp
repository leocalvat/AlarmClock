#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Display.h"
#include "Clock.h"
#include "Player.h"

void IRAM_ATTR ISR();
RgbColor getDimmedColor();
void autoDim();
void turnOnAudio(int file_index);
void turnOffAudio();
void turnOffAlarm();

#define DIM_SENSOR A0
#define PWR_DIM_SENSOR 1 // TX Serial
#define RTC_ALARM 6
#define BTN_SNOOZE 7
#define BTN_MODE 8
#define BTN_ALARMS 9
#define BTN_SET 10
#define BTN_UP 11
#define BTN_DOWN 12
#define BTN_PLAY 13

#define INTERVAL_STOP_ALARM 300000 // 5min
#define INTERVAL_SNOOZE 600000     // 10min
#define INTERVAL_DISPLAY 5000      // 5sec
#define INTERVAL_BLINK 500         // 500ms
#define INTERVAL_UPDATE_CLK 1000   // 1sec
#define INTERVAL_AUTO_DIM 1000     // 1sec

enum DimMods
{
    DIM_LOW,
    DIM_MED,
    DIM_HIGH,
    DIM_AUTO
} dimMod;

volatile bool flags[8] = {false, false, false, false, false, false, false, false};
unsigned long tsAlarmBeggin = 0;
unsigned long tsSnooze = 0;
unsigned long tsMod = 0;
unsigned long tsAutoDim = 0;
bool snoozeEnable = false;
bool alarming = false;
bool displayed = false;
bool blink = false;
uint8_t vol = 20;
uint8_t dim = 255;
RgbColor dispColor = WHITE;

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
    dimMod = DIM_AUTO;

    // Init RTC Module
    initRTC();

    // Init IOs
    for (size_t pin = RTC_ALARM; pin <= BTN_PLAY; pin++)
    {
        pinMode(pin, INPUT_PULLUP);
        attachInterrupt(pin, ISR, FALLING);
    }
    pinMode(PWR_DIM_SENSOR, OUTPUT);
    pinMode(PWR_AUDIO, OUTPUT);
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

    // Init MP3 Player
    initPlayer();

    Serial.println("Running...");
}

void loop()
{
    /////   Handle Display   /////
    switch (displayMod)
    {
    case CLOCK:
        if ((millis() - tsMod) > INTERVAL_UPDATE_CLK)
        {
            blink = !blink;
            RgbColor color = getDimmedColor();
            displayDigits(clockToDigits(getClockTime()), blink, blink, color, color);
            tsMod = millis();
        }
        break;
    case DATE:
        if (!displayed)
        {
            displayDigits(dateToDigits(getClockTime()), false, false, getDimmedColor(), getDimmedColor());
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case YEAR:
        if (!displayed)
        {
            displayDigits(dateToDigits(getClockTime(), true), false, false, getDimmedColor(), getDimmedColor());
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case TEMP:
        if (!displayed)
        {
            displayDigits(tempToDigits(Clock.GetTemperature().AsFloatDegC()), false, true, getDimmedColor(), getDimmedColor());
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case ALARM1:
        if (!displayed)
        {
            DS3231AlarmOne al = Clock.GetAlarmOne();
            displayDigits(nbToDigits(al.Hour(), al.Minute()), true, true, getDimmedColor(), getDimmedColor());
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case ALARM2:
        if (!displayed)
        {
            DS3231AlarmTwo al = Clock.GetAlarmTwo();
            displayDigits(nbToDigits(al.Hour(), al.Minute()), true, true, getDimmedColor(), getDimmedColor());
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case SET_VOLUME:
        if (!displayed)
        {
            displayDigits(nbToDigits(0, vol), false, true, OFF, getDimmedColor());
            displayed = true;
        }
        if ((millis() - tsMod) > INTERVAL_DISPLAY)
            displayMod = CLOCK;
        break;
    case SET_HOUR:
    case SET_ALARM1_HOUR:
    case SET_ALARM2_HOUR:
        if ((millis() - tsMod) > INTERVAL_BLINK)
        {
            blink = !blink;
            RgbColor color = OFF;
            if (blink)
                color = getDimmedColor();
            displayDigits(nbToDigits(tempDateTime.hour, tempDateTime.min), true, true, color, getDimmedColor());
            tsMod = millis();
        }
        break;
    case SET_MIN:
    case SET_ALARM1_MIN:
    case SET_ALARM2_MIN:
        if ((millis() - tsMod) > INTERVAL_BLINK)
        {
            blink = !blink;
            RgbColor color = OFF;
            if (blink)
                color = getDimmedColor();
            displayDigits(nbToDigits(tempDateTime.hour, tempDateTime.min), true, true, getDimmedColor(), color);
            tsMod = millis();
        }
        break;
    case SET_DAY:
        if ((millis() - tsMod) > INTERVAL_BLINK)
        {
            blink = !blink;
            RgbColor color = OFF;
            if (blink)
                color = getDimmedColor();
            displayDigits(nbToDigits(tempDateTime.day, tempDateTime.month), false, false, color, getDimmedColor());
            tsMod = millis();
        }
        break;
    case SET_MONTH:
        if ((millis() - tsMod) > INTERVAL_BLINK)
        {
            blink = !blink;
            RgbColor color = OFF;
            if (blink)
                color = getDimmedColor();
            displayDigits(nbToDigits(tempDateTime.day, tempDateTime.month), false, false, getDimmedColor(), color);
            tsMod = millis();
        }
        break;
    case SET_YEAR:
        if ((millis() - tsMod) > INTERVAL_BLINK)
        {
            blink = !blink;
            RgbColor color = OFF;
            if (blink)
                color = getDimmedColor();
            uint8_t yy1 = (uint8_t)(tempDateTime.year / 100);
            uint8_t yy2 = 20;
            if (tempDateTime.year < 2000)
                yy2 = 19;
            displayDigits(nbToDigits(yy1, yy2), false, false, color, color);
            tsMod = millis();
        }
        break;
    default:
        displayMod = CLOCK;
        break;
    }

    /////   Handle Interrupts   /////
    if (flags[0]) // RTC Alarm
    {
        turnOnAudio(0);
        displayMisc(WHITE);
        alarming = true;
        Clock.LatchAlarmsTriggeredFlags();
        flags[0] = false;
        tsAlarmBeggin = millis();
    }
    if (flags[1]) // Snooze
    {
        if (alarming)
        {
            turnOffAlarm();
            snoozeEnable = true;
            tsSnooze = millis();
        }
        else if (displayMod == CLOCK)
        {
            switch (dimMod)
            {
            case DIM_AUTO:
                dimMod = DIM_LOW;
                dim = 85;
                break;
            case DIM_LOW:
                dimMod = DIM_MED;
                dim = 170;
                break;
            case DIM_MED:
                dimMod = DIM_HIGH;
                dim = 255;
                break;
            case DIM_HIGH:
                dimMod = DIM_AUTO;
                break;
            default:
                dimMod = DIM_AUTO;
                break;
            }
        }
        flags[1] = false;
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
        flags[2] = false;
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
        flags[3] = false;
        tsMod = millis();
    }
    if (flags[4]) // SET
    {
        if (alarming)
            turnOffAlarm();
        else if (displayMod == CLOCK || displayMod == DATE || displayMod == YEAR)
        {
            displayMod = SET_HOUR;
            RtcDateTime t = getClockTime();
            setTempDateTime(t);
        }
        else if (displayMod == SET_HOUR)
            displayMod = SET_MIN;
        else if (displayMod == SET_MIN)
            displayMod = SET_YEAR;
        else if (displayMod == SET_YEAR)
            displayMod = SET_MONTH;
        else if (displayMod == SET_MONTH)
            displayMod = SET_DAY;
        else if (displayMod == SET_DAY)
        {
            setClock();
            displayMod = CLOCK;
        }
        else if (displayMod == ALARM1)
        {
            displayMod = SET_ALARM1_HOUR;
            DS3231AlarmOne al = Clock.GetAlarmOne();
            setTempDateTime(al.Hour(), al.Minute(), 0, 0, 0);
        }
        else if (displayMod == SET_ALARM1_HOUR)
            displayMod = SET_ALARM1_MIN;
        else if (displayMod == SET_ALARM1_MIN)
        {
            setAlarm1Time();
            displayMod = CLOCK;
        }
        else if (displayMod == ALARM2)
        {
            displayMod = SET_ALARM2_HOUR;
            DS3231AlarmTwo al = Clock.GetAlarmTwo();
            setTempDateTime(al.Hour(), al.Minute(), 0, 0, 0);
        }
        else if (displayMod == SET_ALARM2_HOUR)
            displayMod = SET_ALARM2_MIN;
        else if (displayMod == SET_ALARM2_MIN)
        {
            setAlarm2Time();
            displayMod = CLOCK;
        }
        flags[4] = false;
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
        else if (displayMod == SET_HOUR || displayMod == SET_ALARM1_HOUR || displayMod == SET_ALARM2_HOUR)
        {
            tempDateTime.hour++;
            if (tempDateTime.hour > 23)
                tempDateTime.hour = 0;
        }
        else if (displayMod == SET_MIN || displayMod == SET_ALARM1_MIN || displayMod == SET_ALARM2_MIN)
        {
            tempDateTime.min++;
            if (tempDateTime.min > 59)
                tempDateTime.min = 0;
        }
        else if (displayMod == SET_DAY)
        {
            tempDateTime.day++;
            uint8_t limit = c_daysInMonth[tempDateTime.month - 1];
            if (tempDateTime.month == 2)
            {
                if (tempDateTime.year % 4 == 0 && !(tempDateTime.year % 100 == 0 && tempDateTime.year % 400 != 0)) // leap year
                    limit++;
            }
            if (tempDateTime.day > limit)
                tempDateTime.day = 1;
        }
        else if (displayMod == SET_MONTH)
        {
            tempDateTime.month++;
            if (tempDateTime.month > 12)
                tempDateTime.month = 1;
        }
        else if (displayMod == SET_YEAR)
        {
            tempDateTime.year++;
            if (tempDateTime.year > 2099)
                tempDateTime.year = 1970;
        }
        flags[5] = false;
    }
    if (flags[6]) // DOWN
    {
        if (alarming)
            turnOffAlarm();
        else if (displayMod == CLOCK)
            displayMod = SET_VOLUME;
        else if (displayMod == SET_VOLUME && vol != VOL_MIN)
        {
            vol--;
            displayed = false;
            tsMod = millis();
        }
        else if (displayMod == SET_HOUR || displayMod == SET_ALARM1_HOUR || displayMod == SET_ALARM2_HOUR)
        {
            if (tempDateTime.hour < 1)
                tempDateTime.hour = 23;
            else
                tempDateTime.hour--;
        }
        else if (displayMod == SET_MIN || displayMod == SET_ALARM1_MIN || displayMod == SET_ALARM2_MIN)
        {
            if (tempDateTime.min < 1)
                tempDateTime.min = 59;
            else
                tempDateTime.min--;
        }
        else if (displayMod == SET_DAY)
        {
            tempDateTime.day--;
            uint8_t limit = c_daysInMonth[tempDateTime.month - 1];
            if (tempDateTime.month == 2)
            {
                if (tempDateTime.year % 4 == 0 && !(tempDateTime.year % 100 == 0 && tempDateTime.year % 400 != 0)) // leap year
                    limit++;
            }
            if (tempDateTime.day < 1)
                tempDateTime.day = limit;
        }
        else if (displayMod == SET_MONTH)
        {
            tempDateTime.month--;
            if (tempDateTime.month < 1)
                tempDateTime.month = 12;
        }
        else if (displayMod == SET_YEAR)
        {
            tempDateTime.year--;
            if (tempDateTime.year < 1970)
                tempDateTime.year = 2099;
        }
        flags[6] = false;
    }
    if (flags[7]) // PLAY
    {
        if (alarming)
            turnOffAlarm();
        else if (displayMod == ALARM1)
            setAlarm(ALARM_ONE, !AlarmOne);
        else if (displayMod == ALARM2)
            setAlarm(ALARM_TWO, !AlarmTwo);
        else
        {
            if (Player.isBusy())
                turnOffAudio();
            else
                turnOnAudio(0);
        }
        flags[7] = false;
    }

    if (dimMod == DIM_AUTO && ((millis() - tsAutoDim) > INTERVAL_AUTO_DIM))
    {
        autoDim();
    }

    if (alarming && ((millis() - tsAlarmBeggin) > INTERVAL_STOP_ALARM))
    {
        turnOffAlarm();
    }

    if (snoozeEnable && ((millis() - tsSnooze) > INTERVAL_SNOOZE))
    {
        flags[0] = 1;
        snoozeEnable = false;
    }
}

RgbColor getDimmedColor()
{
    return dispColor.Dim(dim);
}

void autoDim()
{
    digitalWrite(PWR_DIM_SENSOR, HIGH);
    delay(1);
    int light = analogRead(DIM_SENSOR);
    //light = constrain(light, 0, 1023);
    dim = map(light, 0, 1023, 255, 70);
    digitalWrite(PWR_DIM_SENSOR, LOW);
}

void turnOffAlarm()
{
    turnOffAudio();
    displayMisc(OFF);
    alarming = false;
}

void IRAM_ATTR ISR()
{
    Serial.println("Enter ISR...");
    uint16_t GPIFs = GPIEC;
    for (size_t i = RTC_ALARM; i <= BTN_PLAY; i++)
    {
        if ((GPIFs >> i) & 0x01)
            flags[i] = 1;
    }
    Serial.println(GPIFs);
}
