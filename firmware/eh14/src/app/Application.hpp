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

#pragma once

#include <cstdint>

#include "app/Menu.hpp"
#include "app/SerialProtocol.hpp"
#include "app/Settings.hpp"
#include "app/SoundBank.hpp"
#include "app/TestMode.hpp"
#include "app/Voice.hpp"
#include "hardware/RealTimeClock.hpp"

namespace eh14
{

/**
 * @class Application
 * @brief The clock itself: shows the time, announces it, rings the alarm, and sleeps whenever it can.
 * @details
 * ### Startup
 * The boost converter is switched on and given its second, then the sound bank is loaded: without one, the clock
 * shows EEEE and waits for an upload (SerialProtocol). Then the settings. If the display was blanked with CHANGE
 * before the clock was switched off, the intro plays (EH14, the first alarm sound, the time announced); otherwise the
 * time is shown with every segment written.
 *
 * ### The main loop (@ref Loop)
 * Each pass serves the serial port, rings the alarm if due, runs the menu or the time mode, checks the light for the
 * silent mode, plays a random alarm sound for the remote's MELODIE, then sleeps if nothing is left to do: until
 * second 55 of the minute, in watchdog periods (Sleep). From second 55 it stays awake and reads the time every
 * params::kClockCheckIntervalMs, so the new minute shows right away. The silent mode's H keeps the clock awake, and
 * so does pending activity (a press not yet taken, the remote sending).
 *
 * ### The time mode
 * SNOOZE announces the time; afterwards the time is shown again in case the minute turned over, and the status LED
 * flashes twice if the battery is low. With params::kFlipDisplayWhileSpeaking, the display follows the clock while it
 * talks (@ref RunBackground). CHANGE blanks the display for params::kDisplayBlankMs and enables the intro in case the
 * clock is switched off meanwhile - a way to hand it over without the time showing. In the silent mode the display
 * shows ---- until the light comes back.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Application
{
public:
    /**
     * @brief Blinks the status LED while MENU and CHANGE are held at power-up.
     * @return Whether they were held (the test mode).
     */
    static bool WaitForTestModeButtons();

    /**
     * @brief Startup, from display power to the time shown (see the class description).
     */
    bool Init(bool test_mode);

    void Loop();

    /**
     * @brief The application's part of Eh14::RunBackground: the display follows the clock while it talks, if
     *        params::kFlipDisplayWhileSpeaking.
     */
    void RunBackground();

    /**
     * @brief Announces a time just set: "presny cas" and @p set_sound, a pause, then @p current_sound and the time.
     */
    void AnnounceTimeSet(std::uint8_t set_sound, std::uint8_t current_sound, const RealTimeClock::Time &time);

    /**
     * @brief Reloads the sound bank after an upload and keeps the alarm sound in range.
     */
    void ReloadSoundBank();

    /**
     * @brief Sets the voice to the volume in the settings.
     */
    void ApplyVolume();

    void RequestSleep()
    {
        sleep_requested_ = true;
    }

    void CancelSleep()
    {
        sleep_requested_ = false;
    }

    /**
     * @brief Makes the time mode show the time on its next pass.
     */
    void SetExitedFromMenu()
    {
        exited_from_menu_ = true;
    }

    Settings &GetSettings()
    {
        return settings_;
    }

    SoundBank &GetSoundBank()
    {
        return sound_bank_;
    }

    Voice &GetVoice()
    {
        return voice_;
    }

private:
    void PowerUpDisplay();
    void WaitForSoundBank();
    void PlayIntro();

    void HandleAlarm();
    void HandleTime();
    void HandleSilentMode();
    void HandleRandomSound();
    void HandleSleep();

    /**
     * @brief CHANGE in time mode: the display blanked, backwards, for params::kDisplayBlankMs.
     */
    void BlankDisplay();

    /**
     * @brief Powers down and sleeps until second params::kSleepUntilSecond, if that is far enough away.
     * @return Whether it slept.
     */
    bool Sleep();

    /**
     * @brief Switches the boost converter and the amplifier back on, waiting for each.
     */
    void WakeUp();

    void ShowTime(const RealTimeClock::Time &time, bool force = false);

    /**
     * @brief A random number in 0..@p range - 1 (xorshift, seeded at startup).
     */
    std::uint32_t Random(std::uint32_t range);

    // Power settling times for Sleep and WakeUp, in ms.
    static constexpr std::uint32_t kPowerDownMs = 100;
    static constexpr std::uint32_t kDisplayWakeMs = 150;
    static constexpr std::uint32_t kAmplifierWakeMs = 150;

    // Display power-up at startup, in ms: pauses around clearing the registers before the boost converter goes on.
    static constexpr std::uint32_t kStartupSettleMs = 200;
    static constexpr std::uint32_t kStartupClearMs = 100;

    // The intro: the pause around EH14, and the LED's three flashes.
    static constexpr std::uint32_t kIntroPauseMs = 100;
    static constexpr unsigned kReadyFlashes = 3;
    static constexpr unsigned kReadyFlashOnMs = 20;
    static constexpr unsigned kReadyFlashOffMs = 80;

    // The low battery warning: two flashes.
    static constexpr unsigned kBatteryFlashes = 2;
    static constexpr unsigned kBatteryFlashOnMs = 40;
    static constexpr unsigned kBatteryFlashOffMs = 120;

    // The pause after an alarm, and before each play, in ms.
    static constexpr std::uint32_t kAfterAlarmMs = 1000;
    static constexpr std::uint32_t kBeforeAlarmPlayMs = 20;

    // No sound bank: the wait before and after each serial port poll, in ms.
    static constexpr std::uint32_t kEmptyWaitMs = 1000;

    Settings settings_;
    SoundBank sound_bank_;
    Voice voice_;
    Menu menu_;
    SerialProtocol serial_;
    TestMode test_mode_;

    bool test_mode_pending_ = false;
    bool sleep_requested_ = true;
    bool exited_from_menu_ = false;
    bool silent_ = false;
    std::uint32_t last_clock_check_ms_ = 0;

    // The display following the clock while it talks (params::kFlipDisplayWhileSpeaking).
    bool following_time_ = false;
    std::uint32_t last_follow_ms_ = 0;

    std::uint32_t random_state_ = 1;
};

} // namespace eh14
