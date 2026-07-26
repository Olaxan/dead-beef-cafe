#include "soloud_impl.h"

#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_openmpt.h"
#include "soloud_error.h"

#include "soloud.h"
#include "soloud_speech.h"

#include <print>
#include <cstdio>

void SoloudAudio::init()
{
	gSoloud.init();
}

void SoloudAudio::deinit()
{
	gSoloud.deinit();
}

AudioHandle SoloudAudio::play_track(std::filesystem::path path)
{
    if (SoLoud::result res = chip.load(path.string().c_str()); res == SoLoud::SO_NO_ERROR)
	{
		return AudioHandle
		{
			.handle = gSoloud.play(chip)
		};
	}
	else
	{
		std::println(stderr, "Failed to play track '{}': {}", path.string(), res);
		return AudioHandle{};
	}
}

AudioHandle SoloudAudio::speak(std::string line, SpeechParams params)
{
	speech.setText(line.c_str());
	speech.setParams(
		params.base_freq,
		params.base_speed,
		params.base_declination,
		static_cast<KLATT_WAVEFORM>(params.waveform));

	SoLoud::handle h = gSoloud.play(speech);

	return AudioHandle
	{
		.handle = h
	};
}

bool SoloudAudio::is_playing(AudioHandle h)
{
	return gSoloud.isValidVoiceHandle(h.handle);
}
