#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <Arduino.h>
#include <EEPROM.h>
#include "Player.h"

#define MEMORY_SIZE 16 // Max 4096
#define ALARM1_ADDR 0  // 0-hh 1-mm 2-on 3-signature
#define ALARM2_ADDR 4  // 4-hh 5-mm 6-on 7-signature
#define VOLUME_ADDR 8  // 8-vol 9-signature
#define SIGNATURE 0x7E // data signature

int writeAlarm(int addr, uint8_t hh, uint8_t mm, bool on)
{
    EEPROM.write(addr, hh);
    EEPROM.write(addr + 1, mm);
    EEPROM.write(addr + 2, on);
    EEPROM.write(addr + 3, SIGNATURE);
    Serial.print("Sending " + String(hh) + "H" + String(mm) + " to address 0x");
    Serial.println(addr, HEX);

    if (EEPROM.commit())
        return 0;
    else
        return -1;
}

uint8_t readAlarm(int addr, uint8_t &hh, uint8_t &mm, bool &on)
{
    on = false;
    hh = EEPROM.read(addr);
    mm = EEPROM.read(addr + 1);
    if (EEPROM.read(addr + 2) == 1)
        on = true;
    if (EEPROM.read(addr + 3) != SIGNATURE)
        return 255;
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59)
    {
        hh = 0;
        mm = 0;
        on = false;
    }
    Serial.print("Sending " + String(hh) + "H" + String(mm) + " to address 0x");
    Serial.println(addr, HEX);
    return 0;
}

int writeVol(uint8_t vol)
{
    EEPROM.write(VOLUME_ADDR, vol);
    EEPROM.write(VOLUME_ADDR + 1, SIGNATURE);
    Serial.print("Sending \"" + String(vol) + "\" to address 0x");
    Serial.println(VOLUME_ADDR, HEX);

    if (EEPROM.commit())
        return 0;
    else
        return -1;
}

uint8_t readVol()
{
    uint8_t vol = EEPROM.read(VOLUME_ADDR);
    if (EEPROM.read(VOLUME_ADDR + 1) != SIGNATURE)
        return 255;
    if (vol < VOL_MIN)
        return VOL_MIN;
    else if (vol > VOL_MAX)
        return VOL_MAX;
    else
        return vol;
}

void eraseMemory()
{
    for (int i = 0; i < MEMORY_SIZE; i++)
        EEPROM.write(i, 0);
    EEPROM.commit();
    //delay(500);
}

#endif // __MEMORY_H__