#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <RtcDS3231.h>
#include <Wire.h>
#include "Memory.h"

enum Alarms
{
    ALARM_ONE,
    ALARM_TWO
};

void initRTC();
void checkAlarms();
RtcDateTime getClockTime();
uint16_t adjustDstEurope(RtcDateTime &t);
void setTempDateTime(uint8_t hour, uint8_t min, uint8_t dd, uint8_t mm, uint16_t yyyy);
void setTempDateTime(RtcDateTime &t);
void setClock();
void setAlarm1Time(bool write = true);
void setAlarm2Time(bool write = true);
void setAlarm(Alarms alarms, bool on);
void setAlarmInterrupt();

#define LED_ALARM1 15
#define LED_ALARM2 16

RtcDS3231<TwoWire> Clock(Wire);
bool AlarmOne;
bool AlarmTwo;

struct TempDateTime
{
    uint8_t hour;
    uint8_t min;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} tempDateTime;

void initRTC()
{
    Clock.Begin(2, 14);
    //Wire.begin(2, 14);
    //Clock.Begin();
    Clock.Enable32kHzPin(false);
    if (!Clock.IsDateTimeValid())
    {
        displayMod = SET_HOUR;
        setTempDateTime(0, 0, 1, 1, 2020);
    }
    if (!Clock.GetIsRunning())
        Clock.SetIsRunning(true);
    checkAlarms();
}

void checkAlarms()
{
    if (!readAlarm(ALARM1_ADDR, tempDateTime.hour, tempDateTime.min, AlarmOne))
        setAlarm1Time();
    else
        setAlarm1Time(false);
    if (!readAlarm(ALARM2_ADDR, tempDateTime.hour, tempDateTime.min, AlarmTwo))
        setAlarm2Time();
    else
        setAlarm2Time(false);
}

RtcDateTime getClockTime()
{
    RtcDateTime t = Clock.GetDateTime();
    return RtcDateTime(t.TotalSeconds() + adjustDstEurope(t));
}

void setTempDateTime(uint8_t hour, uint8_t min, uint8_t dd, uint8_t mm, uint16_t yyyy)
{
    tempDateTime.hour = hour;
    tempDateTime.min = min;
    tempDateTime.day = dd;
    tempDateTime.month = mm;
    tempDateTime.year = yyyy;
}

void setTempDateTime(RtcDateTime &t)
{
    tempDateTime.hour = t.Hour();
    tempDateTime.min = t.Minute();
    tempDateTime.day = t.Day();
    tempDateTime.month = t.Month();
    tempDateTime.year = t.Year();
}

void setClock()
{
    Clock.SetDateTime(RtcDateTime(tempDateTime.year, tempDateTime.month, tempDateTime.day, tempDateTime.hour, tempDateTime.min, 0));
}

void setAlarm1Time(bool write)
{
    Clock.SetAlarmOne(DS3231AlarmOne(0, tempDateTime.hour, tempDateTime.min, 0, DS3231AlarmOneControl_HoursMinutesSecondsMatch));
    setAlarm(ALARM_ONE, true);
    if (write)
        writeAlarm(ALARM1_ADDR, tempDateTime.hour, tempDateTime.min, true);
}

void setAlarm2Time(bool write)
{
    Clock.SetAlarmTwo(DS3231AlarmTwo(0, tempDateTime.hour, tempDateTime.min, DS3231AlarmTwoControl_HoursMinutesMatch));
    setAlarm(ALARM_TWO, true);
    if (write)
        writeAlarm(ALARM2_ADDR, tempDateTime.hour, tempDateTime.min, true);
}

void setAlarm(Alarms alarms, bool on)
{
    if (alarms == ALARM_ONE)
    {
        AlarmOne = on;
        if (on)
            digitalWrite(LED_ALARM1, HIGH);
        else
            digitalWrite(LED_ALARM1, LOW);
    }
    else if (alarms == ALARM_TWO)
    {
        AlarmTwo = on;
        if (on)
            digitalWrite(LED_ALARM2, HIGH);
        else
            digitalWrite(LED_ALARM2, LOW);
    }
    setAlarmInterrupt();
}

void setAlarmInterrupt()
{
    DS3231SquareWavePinMode mode = DS3231SquareWavePin_ModeNone;
    if (AlarmOne && AlarmTwo)
        mode = DS3231SquareWavePin_ModeAlarmBoth;
    else if (AlarmOne)
        mode = DS3231SquareWavePin_ModeAlarmOne;
    else if (AlarmTwo)
        mode = DS3231SquareWavePin_ModeAlarmTwo;
    Clock.LatchAlarmsTriggeredFlags();
    Clock.SetSquareWavePin(mode);
}

uint16_t adjustDstEurope(RtcDateTime &t)
{
    /* You can use the following equations to calculate when DST starts and ends.
    The divisions are integer divisions, in which remainders are discarded.
    With: y = year.
        European Economic Community:
            Begin DST: Sunday March (31 - (5*y/4 + 4) mod 7) at 1h U.T.
            End DST: Sunday October (31 - (5*y/4 + 1) mod 7) at 1h U.T.
            Since 1996, valid through 2099
    (Equations by Wei-Hwa Huang (US), and Robert H. van Gent (EC))

    Adjustig Time with DST Europe/France/Paris: UTC+1h in winter, UTC+2h in summer */

    // last sunday of march
    int beginDSTDate = (31 - (5 * t.Year() / 4 + 4) % 7);
    //Serial.println(beginDSTDate);
    int beginDSTMonth = 3;
    //last sunday of october
    int endDSTDate = (31 - (5 * t.Year() / 4 + 1) % 7);
    //Serial.println(endDSTDate);
    int endDSTMonth = 10;
    // DST is valid as:
    if (((t.Month() > beginDSTMonth) && (t.Month() < endDSTMonth)) || ((t.Month() == beginDSTMonth) && (t.Day() >= beginDSTDate)) || ((t.Month() == endDSTMonth) && (t.Day() <= endDSTDate)))
        return 7200; // DST europe = utc +2 hour (summer time)
    else
        return 3600; // nonDST europe = utc +1 hour (winter time)
}

#endif // __CLOCK_H__