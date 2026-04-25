#include "AppAudio.h"

#ifdef POKEMONCOLLECTION_SIM

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

namespace {
constexpr int kSimAudioChannelCount = 8;
constexpr float kTwoPi = 6.28318530717958647692f;

struct AudioClip {
  std::vector<int16_t> pcm;
  size_t framePosition = 0;
};

struct ChannelState {
  std::deque<AudioClip> queue;
  AudioClip active;
  bool hasActive = false;
  uint8_t volume = 255;
};

struct SimAudioState {
  SDL_AudioDeviceID device = 0;
  SDL_AudioSpec obtained = {};
  std::mutex mutex;
  bool initialized = false;
  uint8_t masterVolume = 255;
  ChannelState channels[kSimAudioChannelCount];
};

SimAudioState gAudio;

int clampChannelIndex(int channel) {
  return std::clamp(channel, 0, kSimAudioChannelCount - 1);
}

float normalizedVolume(uint8_t value) {
  return static_cast<float>(value) / 255.0f;
}

void ensureActiveClip(ChannelState& channel) {
  if (!channel.hasActive && !channel.queue.empty()) {
    channel.active = std::move(channel.queue.front());
    channel.queue.pop_front();
    channel.hasActive = !channel.active.pcm.empty();
    channel.active.framePosition = 0;
  }
}

void mixAudioCallback(void*, Uint8* stream, int len) {
  if (stream == nullptr || len <= 0) {
    return;
  }
  SDL_memset(stream, 0, static_cast<size_t>(len));

  if (!gAudio.initialized || gAudio.obtained.channels <= 0) {
    return;
  }

  const int sampleCount = len / static_cast<int>(sizeof(int16_t));
  auto* out = reinterpret_cast<int16_t*>(stream);
  std::vector<int32_t> accum(static_cast<size_t>(sampleCount), 0);

  std::lock_guard<std::mutex> lock(gAudio.mutex);
  const float master = normalizedVolume(gAudio.masterVolume);

  for (ChannelState& channel : gAudio.channels) {
    ensureActiveClip(channel);
    if (!channel.hasActive) {
      continue;
    }

    const float gain = master * normalizedVolume(channel.volume);
    size_t sampleIndex = 0;
    while (sampleIndex < static_cast<size_t>(sampleCount)) {
      ensureActiveClip(channel);
      if (!channel.hasActive) {
        break;
      }

      const size_t totalFrames = channel.active.pcm.size() / static_cast<size_t>(gAudio.obtained.channels);
      if (channel.active.framePosition >= totalFrames) {
        channel.hasActive = false;
        channel.active = {};
        continue;
      }

      const size_t framesAvailable = totalFrames - channel.active.framePosition;
      const size_t framesWanted = (static_cast<size_t>(sampleCount) - sampleIndex) / static_cast<size_t>(gAudio.obtained.channels);
      const size_t framesToMix = std::min(framesAvailable, framesWanted);
      const size_t baseSample = channel.active.framePosition * static_cast<size_t>(gAudio.obtained.channels);
      const size_t sampleSpan = framesToMix * static_cast<size_t>(gAudio.obtained.channels);

      for (size_t i = 0; i < sampleSpan; ++i) {
        accum[sampleIndex + i] += static_cast<int32_t>(static_cast<float>(channel.active.pcm[baseSample + i]) * gain);
      }

      channel.active.framePosition += framesToMix;
      sampleIndex += sampleSpan;

      if (channel.active.framePosition >= totalFrames) {
        channel.hasActive = false;
        channel.active = {};
      }
    }
  }

  for (int i = 0; i < sampleCount; ++i) {
    out[i] = static_cast<int16_t>(std::clamp(accum[static_cast<size_t>(i)], -32768, 32767));
  }
}

std::vector<int16_t> convertToOutputPcm(const int16_t* rawData, size_t sampleCount, uint32_t sampleRate, bool stereo) {
  std::vector<int16_t> output;
  if (!gAudio.initialized || rawData == nullptr || sampleCount == 0) {
    return output;
  }

  const int srcChannels = stereo ? 2 : 1;
  SDL_AudioCVT cvt;
  if (SDL_BuildAudioCVT(
          &cvt,
          AUDIO_S16SYS,
          static_cast<Uint8>(srcChannels),
          static_cast<int>(sampleRate),
          gAudio.obtained.format,
          gAudio.obtained.channels,
          gAudio.obtained.freq) < 0) {
    return output;
  }

  const int srcBytes = static_cast<int>(sampleCount * sizeof(int16_t));
  cvt.len = srcBytes;
  cvt.buf = static_cast<Uint8*>(SDL_malloc(static_cast<size_t>(srcBytes) * static_cast<size_t>(cvt.len_mult)));
  if (cvt.buf == nullptr) {
    return output;
  }

  SDL_memcpy(cvt.buf, rawData, static_cast<size_t>(srcBytes));
  if (SDL_ConvertAudio(&cvt) != 0) {
    SDL_free(cvt.buf);
    return output;
  }

  output.resize(static_cast<size_t>(cvt.len_cvt) / sizeof(int16_t));
  SDL_memcpy(output.data(), cvt.buf, static_cast<size_t>(cvt.len_cvt));
  SDL_free(cvt.buf);
  return output;
}

std::vector<int16_t> generateTonePcm(float frequency, uint32_t durationMs) {
  std::vector<int16_t> output;
  if (!gAudio.initialized || frequency <= 0.0f || durationMs == 0) {
    return output;
  }

  const uint32_t frameCount = static_cast<uint32_t>((static_cast<uint64_t>(gAudio.obtained.freq) * durationMs) / 1000ULL);
  output.resize(static_cast<size_t>(frameCount) * static_cast<size_t>(gAudio.obtained.channels));

  const float attackFrames = std::max(1.0f, static_cast<float>(gAudio.obtained.freq) * 0.005f);
  const float releaseFrames = std::max(1.0f, static_cast<float>(gAudio.obtained.freq) * 0.02f);
  const float phaseStep = kTwoPi * frequency / static_cast<float>(gAudio.obtained.freq);
  float phase = 0.0f;

  for (uint32_t frame = 0; frame < frameCount; ++frame) {
    float env = 1.0f;
    if (frame < attackFrames) {
      env = static_cast<float>(frame) / attackFrames;
    } else if (frame + releaseFrames >= frameCount) {
      env = static_cast<float>(frameCount - frame) / releaseFrames;
    }
    const float wave = (std::sin(phase) >= 0.0f) ? 0.7f : -0.7f;
    const int16_t sample = static_cast<int16_t>(std::clamp(wave * env * 32767.0f, -32767.0f, 32767.0f));
    for (int ch = 0; ch < gAudio.obtained.channels; ++ch) {
      output[static_cast<size_t>(frame) * static_cast<size_t>(gAudio.obtained.channels) + static_cast<size_t>(ch)] = sample;
    }
    phase += phaseStep;
  }

  return output;
}

bool enqueueClip(std::vector<int16_t>&& pcm, uint32_t repeat, int channel, bool stopCurrentSound) {
  if (!gAudio.initialized || pcm.empty()) {
    return false;
  }

  const int idx = clampChannelIndex(channel);
  std::lock_guard<std::mutex> lock(gAudio.mutex);
  ChannelState& state = gAudio.channels[idx];
  if (stopCurrentSound) {
    state.queue.clear();
    state.active = {};
    state.hasActive = false;
  }

  const uint32_t repeatCount = std::max<uint32_t>(1, repeat);
  for (uint32_t i = 0; i < repeatCount; ++i) {
    AudioClip clip;
    clip.pcm = pcm;
    clip.framePosition = 0;
    state.queue.push_back(std::move(clip));
  }
  return true;
}
}  // namespace

void appAudioBegin(uint32_t sampleRate) {
  if (gAudio.initialized) {
    return;
  }

  SDL_AudioSpec desired = {};
  desired.freq = static_cast<int>(sampleRate);
  desired.format = AUDIO_S16SYS;
  desired.channels = 2;
  desired.samples = 1024;
  desired.callback = mixAudioCallback;

  gAudio.device = SDL_OpenAudioDevice(nullptr, 0, &desired, &gAudio.obtained, 0);
  if (gAudio.device == 0) {
    return;
  }
  gAudio.initialized = true;
  SDL_PauseAudioDevice(gAudio.device, 0);
}

void appAudioStopAll() {
  if (!gAudio.initialized) {
    return;
  }
  std::lock_guard<std::mutex> lock(gAudio.mutex);
  for (ChannelState& channel : gAudio.channels) {
    channel.queue.clear();
    channel.active = {};
    channel.hasActive = false;
  }
}

void appAudioStopChannel(int channel) {
  if (!gAudio.initialized) {
    return;
  }
  std::lock_guard<std::mutex> lock(gAudio.mutex);
  ChannelState& state = gAudio.channels[clampChannelIndex(channel)];
  state.queue.clear();
  state.active = {};
  state.hasActive = false;
}

void appAudioSetMasterVolume(uint8_t volume) {
  gAudio.masterVolume = volume;
}

void appAudioSetChannelVolume(int channel, uint8_t volume) {
  gAudio.channels[clampChannelIndex(channel)].volume = volume;
}

bool appAudioPlayTone(float frequency, uint32_t durationMs, int channel, bool stopCurrentSound) {
  auto pcm = generateTonePcm(frequency, durationMs);
  return enqueueClip(std::move(pcm), 1, channel, stopCurrentSound);
}

bool appAudioPlayRaw16(
    const int16_t* rawData,
    size_t sampleCount,
    uint32_t sampleRate,
    bool stereo,
    uint32_t repeat,
    int channel,
    bool stopCurrentSound) {
  auto pcm = convertToOutputPcm(rawData, sampleCount, sampleRate, stereo);
  return enqueueClip(std::move(pcm), repeat, channel, stopCurrentSound);
}

int appAudioQueuedCount(int channel) {
  if (!gAudio.initialized) {
    return 0;
  }
  std::lock_guard<std::mutex> lock(gAudio.mutex);
  const ChannelState& state = gAudio.channels[clampChannelIndex(channel)];
  return static_cast<int>(state.queue.size() + (state.hasActive ? 1 : 0));
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
