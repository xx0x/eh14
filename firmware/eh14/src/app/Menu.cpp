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

#include "app/Menu.hpp"

#include "Eh14.hpp"
#include "Eh14Params.hpp"
#include "app/Application.hpp"
#include "app/SoundBank.hpp"
#include "hardware/Timebase.hpp"

namespace eh14
{

void Menu::Update()
{
    Buttons &buttons = Eh14::buttons;

    if (buttons.IsPressed(Button::MENU))
    {
        buttons.Clear(Button::MENU);
        buttons.Clear(Button::CHANGE);
        battery_shown_ = kNoBattery;
        if (!editor_.IsActive())
        {
            sound_played_ = false;
            SetItem(static_cast<Item>(static_cast<std::int8_t>(item_) + 1));
        }
        else
        {
            editor_.Next();
            if (editor_.IsDone())
            {
                if (item_ == Item::ALARM_SET)
                {
                    Eh14::rtc.SetAlarm(editor_.GetTime());
                    const RealTimeClock::Time alarm = Eh14::rtc.GetAlarm();
                    Eh14::display.ShowTime(alarm.hour, alarm.minute);
                    Eh14::WaitForDisplay();
                    Eh14::app.AnnounceTimeSet(SoundBank::kAlarmSet, SoundBank::kAlarmCurrent, alarm);
                }
                else if (item_ == Item::TIME_SET)
                {
                    Eh14::rtc.SetTime(editor_.GetTime());
                    const RealTimeClock::Time now = Eh14::rtc.GetTime();
                    Eh14::display.ShowTime(now.hour, now.minute);
                    Eh14::WaitForDisplay();
                    Eh14::app.AnnounceTimeSet(SoundBank::kTimeSet, SoundBank::kTimeCurrent, now);
                }
                Exit(true);
                return;
            }
        }
        Eh14::app.CancelSleep();
        if (item_ >= Item::COUNT)
        {
            SetItem(Item::VOLUME);
            Eh14::display.Clear();
            Eh14::WaitForDisplay();
            Eh14::SmartDelay(100);
        }
    }

    if (!IsActive())
    {
        return;
    }

    const std::uint32_t now = Timebase::GetMillis();
    const bool timed_out = (now - buttons.GetLastPressMs(Button::MENU)) > params::kMenuTimeoutMs &&
                           (now - buttons.GetLastPressMs(Button::CHANGE)) > params::kMenuTimeoutMs;
    if (timed_out || buttons.IsPressed(Button::SNOOZE))
    {
        Eh14::Delay(200);
        Exit(true);
        return;
    }

    switch (item_)
    {
    case Item::VOLUME:
        UpdateVolume();
        break;
    case Item::ALARM_SOUND:
        UpdateAlarmSound();
        break;
    case Item::ALARM_ON:
        UpdateAlarmOn();
        break;
    case Item::ALARM_SET:
    case Item::TIME_SET:
        UpdateTimeItem();
        break;
    case Item::SILENT:
        UpdateSilent();
        break;
    case Item::ALARM_LOOPS:
        UpdateAlarmLoops();
        break;
    case Item::BATTERY:
        UpdateBattery();
        break;
    default:
        break;
    }
    Eh14::SmartDelay(100);
}

void Menu::Exit(bool save)
{
    Eh14::buttons.Clear(Button::MENU);
    Eh14::buttons.Clear(Button::CHANGE);
    Eh14::buttons.Clear(Button::SNOOZE);
    SetItem(Item::NONE);
    editor_.End();
    Eh14::display.Clear();
    Eh14::WaitForDisplay();
    Eh14::app.SetExitedFromMenu();
    Eh14::app.RequestSleep();

    if (save)
    {
        Eh14::app.GetSettings().Save();
        Eh14::Delay(50);
    }
}

void Menu::SetItem(Item item)
{
    item_ = item;
    Eh14::status_led.SetInhibited(item_ == Item::SILENT);
}

void Menu::UpdateVolume()
{
    Settings &settings = Eh14::app.GetSettings();
    if (Eh14::buttons.TakePress(Button::CHANGE))
    {
        if (sound_played_)
        {
            const std::uint8_t volume = static_cast<std::uint8_t>(settings.GetVolume() + 1U);
            settings.SetVolume((volume >= params::kVolumes.size()) ? 0U : volume);
            Eh14::app.ApplyVolume();
        }
        sound_played_ = true;
        ShowItemValue(Glyph::A, static_cast<std::uint8_t>(settings.GetVolume() + 1U));
        Eh14::SmartDelay(300);
        if (!Eh14::buttons.IsAnyPressed())
        {
            Eh14::app.GetVoice().Say(SoundBank::kWarning);
        }
    }
    else
    {
        ShowItemValue(Glyph::A, static_cast<std::uint8_t>(settings.GetVolume() + 1U));
    }
}

void Menu::UpdateAlarmSound()
{
    Settings &settings = Eh14::app.GetSettings();
    if (Eh14::buttons.TakePress(Button::CHANGE))
    {
        std::uint8_t sound = settings.GetAlarmSound();
        if (sound_played_)
        {
            sound++;
        }
        sound_played_ = true;
        if (sound >= Eh14::app.GetSoundBank().GetAlarmCount())
        {
            sound = 0;
        }
        settings.SetAlarmSound(sound);
        ShowItemValue(Glyph::B, static_cast<std::uint8_t>(sound + 1U));
        Eh14::app.GetVoice().Say(static_cast<std::uint8_t>(SoundBank::kFirstAlarm + sound));
    }
    else
    {
        ShowItemValue(Glyph::B, static_cast<std::uint8_t>(settings.GetAlarmSound() + 1U));
    }
}

void Menu::UpdateAlarmOn()
{
    bool enabled = Eh14::rtc.IsAlarmEnabled();
    if (Eh14::buttons.TakePress(Button::CHANGE))
    {
        enabled = !enabled;
        if (enabled)
        {
            Eh14::rtc.EnableAlarm();
        }
        else
        {
            Eh14::rtc.DisableAlarm();
        }
    }
    ShowItem(Glyph::C, Glyph::O, enabled ? Glyph::N : Glyph::F);
}

void Menu::UpdateTimeItem()
{
    Buttons &buttons = Eh14::buttons;
    const bool alarm = item_ == Item::ALARM_SET;

    if (buttons.IsPressed(Button::CHANGE) && !editor_.IsActive())
    {
        buttons.Clear(Button::CHANGE);
        Eh14::display.Clear();
        Eh14::WaitForDisplay();
        editor_.Begin(alarm ? Eh14::rtc.GetAlarm() : Eh14::rtc.GetTime());
        return;
    }

    if (!editor_.IsActive())
    {
        // Cycle through the hours, the minutes and a blank.
        const Glyph letter = alarm ? Glyph::D : Glyph::E;
        const RealTimeClock::Time time = alarm ? Eh14::rtc.GetAlarm() : Eh14::rtc.GetTime();
        ShowItemValue(letter, time.hour);
        Eh14::SmartDelay(params::kMenuTimeStepMs);
        if (!buttons.IsAnyPressed())
        {
            ShowItemValue(letter, time.minute);
            Eh14::SmartDelay(params::kMenuTimeStepMs);
        }
        if (!buttons.IsAnyPressed())
        {
            ShowItem(letter, Glyph::NONE, Glyph::NONE);
            Eh14::SmartDelay(params::kMenuTimeStepMs);
        }
        return;
    }

    if (buttons.TakePress(Button::CHANGE))
    {
        editor_.Increase();
    }
    // The digits set so far, up to the one being set; the rest stays blank.
    for (std::uint8_t digit = 0; digit <= editor_.GetCurrent() && digit < TimeEditor::kDigits; digit++)
    {
        Eh14::display.ShowDigit(digit, Display::Digit(editor_.GetDigit(digit)));
    }
    Eh14::WaitForDisplay();
}

void Menu::UpdateSilent()
{
    Settings &settings = Eh14::app.GetSettings();
    if (Eh14::buttons.TakePress(Button::CHANGE))
    {
        // Off, L1..L8, H1..H8, off.
        std::uint8_t level = static_cast<std::uint8_t>(settings.GetSilentLevel() + 1U);
        if (level > params::kSilentThresholds.size())
        {
            if (settings.IsSilentHighPower())
            {
                settings.SetSilentHighPower(false);
                level = 0;
            }
            else
            {
                settings.SetSilentHighPower(true);
                level = 1;
            }
        }
        settings.SetSilentLevel(level);
    }
    if (!settings.IsSilentEnabled())
    {
        ShowItem(Glyph::F, Glyph::O, Glyph::F);
    }
    else
    {
        ShowItem(Glyph::F, settings.IsSilentHighPower() ? Glyph::H : Glyph::L,
                 Display::Digit(settings.GetSilentLevel()));
    }
}

void Menu::UpdateAlarmLoops()
{
    Settings &settings = Eh14::app.GetSettings();
    if (Eh14::buttons.TakePress(Button::CHANGE))
    {
        const std::uint8_t loops = static_cast<std::uint8_t>(settings.GetAlarmLoops() + 1U);
        settings.SetAlarmLoops((loops >= params::kAlarmLoops.size()) ? 0U : loops);
    }
    const std::uint8_t loops = params::kAlarmLoops[settings.GetAlarmLoops()];
    if (loops > 0U)
    {
        ShowItemValue(Glyph::G, loops);
    }
    else
    {
        ShowItem(Glyph::G, Glyph::O, Glyph::O);
    }
}

void Menu::UpdateBattery()
{
    if (battery_shown_ == kNoBattery)
    {
        ShowItem(Glyph::H, Glyph::UNDERLINE, Glyph::UNDERLINE);
        Eh14::SmartDelay(300);
    }
    battery_shown_ = Eh14::analog_inputs.ReadBatteryPercent();
    Eh14::SmartDelay(10);
    ShowItemValue(Glyph::H, battery_shown_);
    Eh14::SmartDelay(params::kMenuBatteryRefreshMs);
}

void Menu::ShowItem(Glyph letter, Glyph third, Glyph fourth)
{
    Eh14::display.Show({letter, Glyph::NONE, third, fourth});
    Eh14::WaitForDisplay();
}

void Menu::ShowItemValue(Glyph letter, std::uint8_t value)
{
    ShowItem(letter, Display::Digit(static_cast<std::uint8_t>((value / 10U) % 10U)), Display::Digit(value));
}

} // namespace eh14
