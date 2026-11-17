/*
MIT License

Copyright (c) 2026 Vaclav Mach

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "app/Application.hpp"

#include <array>

#include "Eh14.hpp"
#include "Eh14Params.hpp"
#include "hardware/SegmentDriver.hpp"
#include "hardware/Timebase.hpp"

namespace eh14
{

bool Application::WaitForTestModeButtons()
{
    bool held = false;
    while (Eh14::buttons.IsHeld(Button::MENU) && Eh14::buttons.IsHeld(Button::CHANGE))
    {
        held = true;
        Eh14::status_led.Set(true);
        Eh14::Delay(50);
        Eh14::status_led.Set(false);
        Eh14::Delay(50);
    }
    return held;
}

bool Application::Init(bool test_mode)
{
    test_mode_pending_ = test_mode;
    // Seed the random alarm sound from the chip's unique ID and the time.
    const RealTimeClock::Time now = Eh14::rtc.GetTime();
    random_state_ = *reinterpret_cast<const std::uint32_t *>(0x0080A00C) ^ (now.second * 2654435761U) ^ now.minute;
    if (random_state_ == 0U)
    {
        random_state_ = 1;
    }

    PowerUpDisplay();
    WaitForSoundBank();

    Eh14::audio.SetAmplifier(true);
    settings_.Load(sound_bank_.GetAlarmCount());
    ApplyVolume();

    if (settings_.IsIntroEnabled())
    {
        PlayIntro();
    }
    else if (!test_mode_pending_)
    {
        ShowTime(Eh14::rtc.GetTime(), true);
    }
    return true;
}

void Application::Loop()
{
    if (test_mode_pending_)
    {
        test_mode_pending_ = false;
        test_mode_.Run();
    }

    serial_.Poll();
    HandleAlarm();
    if (!Eh14::rtc.IsAlarmPending())
    {
        menu_.Update();
    }
    HandleSilentMode();

    if (!Eh14::rtc.IsAlarmPending())
    {
        HandleRandomSound();
        if (!menu_.IsActive())
        {
            HandleTime();
        }
        HandleSleep();
    }
}

void Application::RunBackground()
{
    if constexpr (params::kFlipDisplayWhileSpeaking)
    {
        const std::uint32_t now = Timebase::GetMillis();
        if (following_time_ && (now - last_follow_ms_) >= params::kClockCheckIntervalMs)
        {
            last_follow_ms_ = now;
            const RealTimeClock::Time time = Eh14::rtc.GetTime();
            Eh14::display.ShowTime(time.hour, time.minute);
        }
    }
}

void Application::AnnounceTimeSet(std::uint8_t set_sound, std::uint8_t current_sound, const RealTimeClock::Time &time)
{
    const std::array<std::uint8_t, 2> sentence = {SoundBank::kIntro, set_sound};
    if (voice_.SaySentence(sentence))
    {
        Eh14::Delay(params::kAnnouncementPauseMs);
        voice_.SayTime(time.hour, time.minute, current_sound, true);
    }
}

void Application::ReloadSoundBank()
{
    sound_bank_.Load();
    if (settings_.GetAlarmSound() >= sound_bank_.GetAlarmCount())
    {
        settings_.SetAlarmSound(0);
    }
}

void Application::ApplyVolume()
{
    voice_.SetVolume(params::kVolumes[settings_.GetVolume() % params::kVolumes.size()]);
}

void Application::PowerUpDisplay()
{
    Eh14::Delay(kStartupSettleMs);
    Eh14::status_led.Set(false);
    Eh14::display.SetPower(false);
    Eh14::Delay(kStartupClearMs);
    SegmentDriver::Clear();
    Eh14::Delay(kStartupClearMs);
    Eh14::display.SetPower(true);
    Eh14::Delay(params::kDisplayPowerUpMs);
}

void Application::WaitForSoundBank()
{
    // The first EEEE is forced: the segments may be in any state.
    bool first = true;
    while (!sound_bank_.Load())
    {
        Eh14::display.Show({Glyph::E, Glyph::E, Glyph::E, Glyph::E}, first);
        first = false;
        Eh14::WaitForDisplay();
        Eh14::Delay(kEmptyWaitMs);
        serial_.Poll();
        Eh14::Delay(kEmptyWaitMs);
    }
}

void Application::PlayIntro()
{
    Eh14::display.Clear(true);
    Eh14::WaitForDisplay();
    Eh14::Delay(kIntroPauseMs);
    Eh14::display.Show({Glyph::E, Glyph::H, Display::Digit(1), Display::Digit(4)});
    Eh14::WaitForDisplay();
    voice_.Say(SoundBank::kFirstAlarm);
    Eh14::Delay(kIntroPauseMs);
    Eh14::display.Clear();
    Eh14::WaitForDisplay();
    Eh14::status_led.Flash(kReadyFlashes, kReadyFlashOnMs, kReadyFlashOffMs);
    Eh14::usb.Println("EH14 ready\n");

    const RealTimeClock::Time now = Eh14::rtc.GetTime();
    ShowTime(now);
    voice_.SayTime(now.hour, now.minute);

    settings_.SetIntroEnabled(false);
    settings_.Save();
}

void Application::HandleAlarm()
{
    if (!Eh14::rtc.TakeAlarm())
    {
        return;
    }
    Eh14::rtc.ClearAlarmFlag();
    Eh14::buttons.SetAlarmRinging(true);
    // Only a press from now on stops it, not an earlier one.
    Eh14::buttons.ClearStopRequest();

    const RealTimeClock::Time alarm = Eh14::rtc.GetAlarm();
    const auto sound = static_cast<std::uint8_t>(SoundBank::kFirstAlarm + settings_.GetAlarmSound());
    const std::uint8_t loops = params::kAlarmLoops[settings_.GetAlarmLoops() % params::kAlarmLoops.size()];

    // A press stops it during a play or between two.
    for (unsigned played = 0; loops == 0U || played < loops; played++)
    {
        Eh14::Delay(kBeforeAlarmPlayMs);
        ShowTime(alarm);
        if (Eh14::buttons.IsStopRequested())
        {
            break;
        }
        voice_.Say(sound);
        const std::uint32_t gap_start = Timebase::GetMillis();
        while (!Eh14::buttons.IsStopRequested() && (Timebase::GetMillis() - gap_start) < params::kAlarmGapMs)
        {
            Eh14::RunBackground();
        }
        if (Eh14::buttons.IsStopRequested())
        {
            break;
        }
    }

    Eh14::buttons.SetAlarmRinging(false);
    sleep_requested_ = true;
    menu_.Exit(false);
    Eh14::Delay(kAfterAlarmMs);
}

void Application::HandleTime()
{
    Buttons &buttons = Eh14::buttons;

    if (buttons.TakePress(Button::CHANGE))
    {
        BlankDisplay();
    }

    if (settings_.IsSilentEnabled() && silent_ && !buttons.IsPressed(Button::SNOOZE))
    {
        sleep_requested_ = true;
        return;
    }

    const std::uint32_t now_ms = Timebase::GetMillis();
    if ((now_ms - last_clock_check_ms_) < params::kClockCheckIntervalMs && !buttons.IsPressed(Button::SNOOZE) &&
        !exited_from_menu_)
    {
        return;
    }
    exited_from_menu_ = false;
    last_clock_check_ms_ = now_ms;

    RealTimeClock::Time now = Eh14::rtc.GetTime();
    ShowTime(now);
    if (buttons.TakePress(Button::SNOOZE))
    {
        following_time_ = params::kFlipDisplayWhileSpeaking;
        last_follow_ms_ = Timebase::GetMillis();
        voice_.SayTime(now.hour, now.minute);
        following_time_ = false;

        // The minute may have turned over while it talked.
        Eh14::SmartDelay(100);
        now = Eh14::rtc.GetTime();
        ShowTime(now);
        Eh14::SmartDelay(100);
        if (Eh14::analog_inputs.ReadBatteryPercent() < params::kBatteryLowPercent)
        {
            Eh14::status_led.Flash(kBatteryFlashes, kBatteryFlashOnMs, kBatteryFlashOffMs);
        }
    }
    sleep_requested_ = true;
}

void Application::BlankDisplay()
{
    settings_.SetIntroEnabled(true);
    settings_.Save();
    Eh14::display.ClearBackwards();
    Eh14::WaitForDisplay();
    Eh14::SmartDelay(params::kDisplayBlankMs);
    settings_.SetIntroEnabled(false);
    settings_.Save();
}

void Application::HandleSilentMode()
{
    if (!settings_.IsSilentEnabled())
    {
        return;
    }

    std::uint32_t sum = 0;
    for (std::uint8_t reading = 0; reading < params::kSilentReadings; reading++)
    {
        sum += Eh14::analog_inputs.ReadLight();
        Eh14::SmartDelay(params::kSilentReadingGapMs);
    }

    // Compare the sum against threshold x readings: the average without rounding.
    const auto threshold = static_cast<std::int32_t>(params::kSilentThresholds[settings_.GetSilentLevel() - 1U]);
    const std::int32_t excess = static_cast<std::int32_t>(sum) - (threshold * params::kSilentReadings);
    const bool dark = excess <= 0;
    const std::int32_t above = excess / params::kSilentReadings;
    if (dark != silent_ && (!silent_ || above > params::kSilentHysteresis))
    {
        silent_ = dark;
        const bool in_menu = menu_.GetItem() == Menu::Item::SILENT;
        if (silent_)
        {
            Eh14::usb.Println("Silent mode activated.");
            if (in_menu)
            {
                voice_.Say(SoundBank::kDong);
            }
        }
        else
        {
            if (in_menu)
            {
                voice_.Say(SoundBank::kDing);
            }
            Eh14::usb.Println("Silent mode deactivated.");
        }
    }

    if (!menu_.IsActive() && silent_)
    {
        Eh14::display.Show({Glyph::DASH, Glyph::DASH, Glyph::DASH, Glyph::DASH});
        Eh14::WaitForDisplay();
    }
}

void Application::HandleRandomSound()
{
    if (!Eh14::buttons.TakePress(Button::SOUND))
    {
        return;
    }
    const std::uint8_t alarms = sound_bank_.GetAlarmCount();
    if (alarms > 0U)
    {
        voice_.Say(static_cast<std::uint8_t>(SoundBank::kFirstAlarm + Random(alarms)));
    }
}

void Application::HandleSleep()
{
    if (!sleep_requested_ || settings_.IsSilentHighPower() || Eh14::buttons.IsAnyPressed() ||
        Eh14::ir_receiver.IsReceiving())
    {
        return;
    }
    sleep_requested_ = false;
    if (Sleep())
    {
        WakeUp();
    }
}

bool Application::Sleep()
{
    const RealTimeClock::Time now = Eh14::rtc.GetTime();
    const std::int32_t sleep_s = static_cast<std::int32_t>(params::kSleepUntilSecond) - now.second;
    if (sleep_s < params::kMinimumSleepS)
    {
        return false;
    }

    Eh14::audio.SetAmplifier(false);
    Eh14::display.SetPower(false);
    Eh14::Delay(kPowerDownMs);
    Eh14::usb.Disconnect();
    Eh14::sleep.SleepFor(static_cast<std::uint32_t>(sleep_s) * 1000U);
    Eh14::usb.Connect();
    return true;
}

void Application::WakeUp()
{
    Eh14::display.SetPower(true);
    Eh14::Delay(kDisplayWakeMs);
    Eh14::audio.SetAmplifier(true);
    Eh14::Delay(kAmplifierWakeMs);
}

void Application::ShowTime(const RealTimeClock::Time &time, bool force)
{
    Eh14::display.ShowTime(time.hour, time.minute, force);
    Eh14::WaitForDisplay();
}

std::uint32_t Application::Random(std::uint32_t range)
{
    random_state_ ^= random_state_ << 13;
    random_state_ ^= random_state_ >> 17;
    random_state_ ^= random_state_ << 5;
    return random_state_ % range;
}

} // namespace eh14
