#include "prog_media.h"

#include "soloud.h"
#include "soloud_speech.h"

#include "CLI/CLI.hpp"

#include <string>
#include <ranges>
#include <cstdlib>
#include <print>

#include <iso646.h>


ProcessTask Programs::CmdSpeak(Proc& proc, std::vector<std::string> args)
{

	CLI::App app{"A simple TTS utility."};
	app.allow_windows_style_options(false);

	struct SpeechArgs
	{
		std::string line{"Now is the time for all good men to come to the aid of their country."};
		unsigned int base_freq{1330};
		float base_speed{10.f};
		float base_declination{0.5f};
		KLATT_WAVEFORM waveform{KW_SQUARE};
	} params{};

	app.add_option("LINE", params.line, "The line to speak out");
	app.add_option("-f,--freq", params.base_freq, "Frequency of the spoken line")->capture_default_str();
	app.add_option("-s,--speed", params.base_speed, "Speed of the spoken line")->capture_default_str();
	app.add_option("-d,--declination", params.base_declination, "Declination of the spoken line")->capture_default_str();
	app.add_option("-w,--wave", params.base_freq, "Waveform of the spoken line")->capture_default_str();

	try
	{
		std::ranges::reverse(args);
		args.pop_back();
        app.parse(std::move(args));
    }
	catch(const CLI::ParseError& e)
	{
		int res = app.exit(e, proc.s_out, proc.s_err);
        co_return res;
    }

	// Define a couple of variables
	SoLoud::Soloud soloud;  // SoLoud engine core
	SoLoud::Speech speech;  // A sound source (speech, in this case)

	// Configure sound source
	speech.setText(params.line.c_str());
	speech.setParams(params.base_freq, params.base_speed, params.base_declination, params.waveform);

	// initialize SoLoud.
	soloud.init();

	// Play the sound source (we could do this several times if we wanted)
	soloud.play(speech);

	// Wait until sounds have finished
	while (soloud.getActiveVoiceCount() > 0)
	{
		// Still going, sleep for a bit
		co_await proc.wait(0.5f);
	}

	// Clean up SoLoud
	soloud.deinit();

    co_return 0;
}