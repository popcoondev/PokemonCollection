#ifndef APP_AUDIO_H
#define APP_AUDIO_H

#include <Arduino.h>

void appAudioBegin(uint32_t sampleRate);
void appAudioStopAll();
void appAudioStopChannel(int channel);
void appAudioSetMasterVolume(uint8_t volume);
void appAudioSetChannelVolume(int channel, uint8_t volume);
bool appAudioPlayTone(float frequency, uint32_t durationMs, int channel, bool stopCurrentSound = true);
bool appAudioPlayRaw16(
    const int16_t* rawData,
    size_t sampleCount,
    uint32_t sampleRate,
    bool stereo,
    uint32_t repeat,
    int channel,
    bool stopCurrentSound = false);
int appAudioQueuedCount(int channel);

#endif
