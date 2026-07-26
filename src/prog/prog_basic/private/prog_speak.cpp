#include "prog_basic.h"
#include "term_utils.h"

#include "audio_base.h"

#include "CLI/CLI.hpp"

#include <string>
#include <ranges>
#include <cstdlib>
#include <print>
#include <regex>

#include <iso646.h>


ProcessTask Programs::CmdSpeak(Proc& proc, std::vector<std::string> args)
{
	CLI::App app{"A simple TTS utility."};
	app.allow_windows_style_options(false);

	struct SpeechArgs
	{
		std::string line{"Now is the time for all good men to come to the aid of their country."};
	} params{};

	SpeechParams speech
	{
		.base_freq{1330},
		.base_speed{10.f},
		.base_declination{0.5f},
		.waveform{AudioWaveform::Square}
	};

	app.add_option("LINE", params.line, "The line to speak out");
	app.add_option("-f,--freq", speech.base_freq, "Frequency of the spoken line")->capture_default_str();
	app.add_option("-s,--speed", speech.base_speed, "Speed of the spoken line")->capture_default_str();
	app.add_option("-d,--declination", speech.base_declination, "Declination of the spoken line")->capture_default_str();
	app.add_option("-w,--wave", speech.waveform, "Waveform of the spoken line")->capture_default_str();

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

	IAudioBase* audio = proc.owning_os->get_audio();

	if (audio == nullptr)
	{
		proc.errln("No audio engine installed.");
		co_return 1;
	}

	if (proc.is_tty())
	{
		AudioHandle h = audio->speak(params.line, speech);

		while (audio->is_playing(h))
		{
			co_await proc.wait(0.1f);
		}
	}
	else
	{
		while (true)
		{
			auto res = co_await proc.read();

			if (not res)
			{
				co_return 0;
			}

			std::string safe = TermUtils::strip_ansi(*res);

			AudioHandle h = audio->speak(safe, speech);

			while (audio->is_playing(h))
			{
				co_await proc.wait(0.1f);
			}
		}
	}

    co_return 0;
}