#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <RtcDS3231.h>
#include <Wire.h>

void initRTC();
RtcDateTime getClockTime();
uint16_t adjustDstEurope(RtcDateTime &t);

RtcDS3231<TwoWire> Clock(Wire);

void initRTC()
{
    Clock.Enable32kHzPin(false);
    Clock.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
}

RtcDateTime getClockTime()
{
    RtcDateTime t = Clock.GetDateTime();
    return RtcDateTime(t.TotalSeconds() + adjustDstEurope(t));
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