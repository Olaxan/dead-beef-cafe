#include "prog_media.h"

#include "soloud.h"
#include "soloud_wav.h"

ProcessTask Programs::CmdRadio(Proc& proc, std::vector<std::string> args)
{
	SoLoud::Soloud gSoloud; // SoLoud engine
	SoLoud::Wav gWave;      // One wave file

	gSoloud.init(); // Initialize SoLoud

    gWave.load("assets/audio/track1.wav"); // Load a wave

	int h = gSoloud.play(gWave); // Play the wave

    // Wait while sound plays
    while (gSoloud.getActiveVoiceCount() > 0)
    {
        co_await proc.wait(1.f);
    }

    gSoloud.deinit(); // Clean up!

    co_return 0;
}