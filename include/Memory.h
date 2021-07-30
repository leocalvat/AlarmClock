#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <Arduino.h>
#include <EEPROM.h>
#include "Player.h"

bool writeAlarm(int addr, uint8_t hh, uint8_t mm, bool on);
bool readAlarm(int addr, uint8_t &hh, uint8_t &mm, bool &on);
bool writeVol(uint8_t volume);
bool readVol(uint8_t &volume);

#define MEMORY_SIZE 16 // Max 4096
#define ALARM1_ADDR 0  // 0-hh 1-mm 2-on 3-signature
#define ALARM2_ADDR 4  // 4-hh 5-mm 6-on 7-signature
#define VOLUME_ADDR 8  // 8-vol 9-signature
#define SIGNATURE 0x7E // data signature

bool writeAlarm(int addr, uint8_t hh, uint8_t mm, bool on)
{
    EEPROM.write(addr, hh);
    EEPROM.write(addr + 1, mm);
    EEPROM.write(addr + 2, on);
    EEPROM.write(addr + 3, SIGNATURE);
    Serial.print("Sending " + String(hh) + "H" + String(mm) + " to address 0x");
    Serial.println(addr, HEX);

    if (EEPROM.commit())
        return true;
    return false;
}

bool readAlarm(int addr, uint8_t &hh, uint8_t &mm, bool &on)
{
    hh = 0;
    mm = 0;
    on = false;
    if (EEPROM.read(addr + 3) != SIGNATURE)
        return false;
    uint8_t hour = EEPROM.read(addr);
    uint8_t min = EEPROM.read(addr + 1);
    if (hour < 0 || hour > 23 || min < 0 || min > 59)
        return false;
    if (EEPROM.read(addr + 2) == 1)
        on = true;
    hh = hour;
    mm = min;

    Serial.print("Reading " + String(hh) + "H" + String(mm) + " to address 0x");
    Serial.println(addr, HEX);
    return true;
}

bool writeVol(uint8_t volume)
{
    EEPROM.write(VOLUME_ADDR, vol);
    EEPROM.write(VOLUME_ADDR + 1, SIGNATURE);
    Serial.print("Sending \"" + String(vol) + "\" to address 0x");
    Serial.println(VOLUME_ADDR, HEX);

    if (EEPROM.commit())
        return true;
    return false;
}

bool readVol(uint8_t &volume)
{
    volume = VOL_DEFAULT;
    if (EEPROM.read(VOLUME_ADDR + 1) != SIGNATURE)
        return false;
    uint8_t vol = EEPROM.read(VOLUME_ADDR);
    if (vol < VOL_MIN || vol > VOL_MAX)
        return false;
    else
        volume = vol;
    return true;
}

void eraseMemory()
{
    for (int i = 0; i < MEMORY_SIZE; i++)
        EEPROM.write(i, 0);
    EEPROM.commit();
    //delay(500);
}

#endif // __MEMORY_H__