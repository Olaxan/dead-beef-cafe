#pragma once

#include "audio_base.h"

#include "soloud.h"
#include "soloud_openmpt.h"
#include "soloud_speech.h"

class SoloudAudio : public IAudioBase
{
public:

	void init() override;
	void deinit() override;
	AudioHandle play_track(std::filesystem::path path) override;
	AudioHandle speak(std::string line, SpeechParams params) override;
	bool is_playing(AudioHandle h) override;

private:

	SoLoud::Soloud gSoloud;
	SoLoud::Openmpt chip;
	SoLoud::Speech speech;

};