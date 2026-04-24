#include "AppAudio.h"

#ifdef POKEMONCOLLECTION_SIM

void appAudioBegin(uint32_t sampleRate) {
  (void)sampleRate;
}

void appAudioStopAll() {
}

void appAudioStopChannel(int channel) {
  (void)channel;
}

void appAudioSetMasterVolume(uint8_t volume) {
  (void)volume;
}

void appAudioSetChannelVolume(int channel, uint8_t volume) {
  (void)channel;
  (void)volume;
}

bool appAudioPlayTone(float frequency, uint32_t durationMs, int channel, bool stopCurrentSound) {
  (void)frequency;
  (void)durationMs;
  (void)channel;
  (void)stopCurrentSound;
  return true;
}

bool appAudioPlayRaw16(
    const int16_t* rawData,
    size_t sampleCount,
    uint32_t sampleRate,
    bool stereo,
    uint32_t repeat,
    int channel,
    bool stopCurrentSound) {
  (void)rawData;
  (void)sampleCount;
  (void)sampleRate;
  (void)stereo;
  (void)repeat;
  (void)channel;
  (void)stopCurrentSound;
  return true;
}

int appAudioQueuedCount(int channel) {
  return (channel == 0) ? 2 : 0;
}

#else

#include <M5Unified.h>

void appAudioBegin(uint32_t sampleRate) {
  auto spkCfg = M5.Speaker.config();
  spkCfg.sample_rate = sampleRate;
  M5.Speaker.config(spkCfg);
  M5.Speaker.begin();
}

void appAudioStopAll() {
  M5.Speaker.stop();
}

void appAudioStopChannel(int channel) {
  M5.Speaker.stop(channel);
}

void appAudioSetMasterVolume(uint8_t volume) {
  M5.Speaker.setVolume(volume);
}

void appAudioSetChannelVolume(int channel, uint8_t volume) {
  M5.Speaker.setChannelVolume(channel, volume);
}

bool appAudioPlayTone(float frequency, uint32_t durationMs, int channel, bool stopCurrentSound) {
  return M5.Speaker.tone(frequency, durationMs, channel, stopCurrentSound);
}

bool appAudioPlayRaw16(
    const int16_t* rawData,
    size_t sampleCount,
    uint32_t sampleRate,
    bool stereo,
    uint32_t repeat,
    int channel,
    bool stopCurrentSound) {
  return M5.Speaker.playRaw(
      rawData,
      sampleCount,
      sampleRate,
      stereo,
      repeat,
      channel,
      stopCurrentSound);
}

int appAudioQueuedCount(int channel) {
  return M5.Speaker.isPlaying(channel);
}

#endif
