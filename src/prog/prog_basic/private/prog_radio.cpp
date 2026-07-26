#include "prog_basic.h"

#include "audio_base.h"

ProcessTask Programs::CmdRadio(Proc& proc, std::vector<std::string> args)
{
    IAudioBase* audio = proc.owning_os->get_audio();

    if (audio == nullptr)
    {
        proc.errln("No audio engine installed.");
        co_return 1;
    }

    AudioHandle h = audio->play_track("assets/mod/searching_for_truth.mod");

    while (audio->is_playing(h))
    {
        co_await proc.wait(1.f);
    }

    co_return 0;
}