#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <DFPlayerMini.h>

void initPlayer();
void turnOnAudio(int file_index);
void turnOffAudio();

#define PWR_AUDIO 0 // GPIO 0
#define VOL_MIN 0
#define VOL_MAX 30

DFPlayerMini Player;

void initPlayer()
{
    digitalWrite(PWR_AUDIO, HIGH);
    delay(1);
    Player.init(6, 5, 4);
    digitalWrite(PWR_AUDIO, LOW);
}

void turnOnAudio(int file_index)
{
    digitalWrite(PWR_AUDIO, HIGH);
    delay(1);
    Player.setVolume(vol);
    Player.playFile(file_index);
}

void turnOffAudio()
{
    if (Player.isBusy())
        Player.stop();
    digitalWrite(PWR_AUDIO, LOW);
}

#endif // __PLAYER_H__