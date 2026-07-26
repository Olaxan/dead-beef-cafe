#pragma once

#include <filesystem>

struct AudioHandle 
{ 
	unsigned int handle{0}; 
};

enum class AudioWaveform : uint8_t
{
	Saw = 0,
	Triangle,
	Sine,
	Square,
	Pulse,
	Noise,
	Warble
};

struct SpeechParams
{
	unsigned int base_freq{1330};
	float base_speed{10.f};
	float base_declination{0.5f};
	AudioWaveform waveform{AudioWaveform::Square};
};

class IAudioBase
{
public:

	virtual void init() = 0;
	virtual void deinit() = 0;
	virtual AudioHandle play_track(std::filesystem::path path) = 0;
	virtual AudioHandle speak(std::string line, SpeechParams params) = 0;
	virtual bool is_playing(AudioHandle h) = 0;
};