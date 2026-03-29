#include "prog_media.h"

#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_openmpt.h"

ProcessTask Programs::CmdRadio(Proc& proc, std::vector<std::string> args)
{
	SoLoud::Soloud gSoloud;
	gSoloud.init();

	SoLoud::Openmpt chip;

    chip.load("assets/mod/aryx.s3m");

	SoLoud::handle h = gSoloud.play(chip);

    // Wait while sound plays
    while (gSoloud.isValidVoiceHandle(h))
    {
        co_await proc.wait(1.f);
    }

    gSoloud.deinit();

    co_return 0;
}