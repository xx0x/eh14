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

#include "app/TestMode.hpp"

#include "Eh14.hpp"
#include "app/Application.hpp"
#include "app/SoundBank.hpp"

namespace eh14
{

void TestMode::Run()
{
    Display &display = Eh14::display;

    display.Show({Glyph::S, Glyph::E, Glyph::L, Glyph::F}, true);
    Eh14::WaitForDisplay();
    Eh14::Delay(1000);
    display.Show({Glyph::T, Glyph::E, Glyph::S, Glyph::T});
    Eh14::WaitForDisplay();
    Eh14::Delay(1000);

    for (std::uint8_t value = 0; value < 10U; value++)
    {
        for (std::uint8_t digit = 0; digit < Display::kDigits; digit++)
        {
            display.ShowDigit(digit, Display::Digit(value));
            Eh14::WaitForDisplay();
            Eh14::Delay(100);
        }
    }
    Eh14::Delay(100);
    display.Clear();
    Eh14::WaitForDisplay();

    // The bank: how many alarm sounds, and the last one played.
    const std::uint8_t alarms = Eh14::app.GetSoundBank().GetAlarmCount();
    display.ShowDigit(2, Display::Digit(alarms / 10U));
    display.ShowDigit(3, Display::Digit(alarms));
    Eh14::WaitForDisplay();
    Eh14::app.GetVoice().Say(static_cast<std::uint8_t>(SoundBank::kFirstAlarm + alarms - 1U));
    Eh14::Delay(100);
    display.Clear();
    Eh14::WaitForDisplay();

    // The alarm: a minute turning over onto it.
    display.ShowDigit(0, Glyph::A);
    Eh14::WaitForDisplay();
    Eh14::rtc.SetTime({6, 59, 58});
    Eh14::rtc.SetAlarm({7, 0, 0});
    Eh14::Delay(3000);
    const bool alarm_ok = Eh14::rtc.TakeAlarm();
    Eh14::rtc.ClearAlarmFlag();
    Eh14::buttons.SetAlarmRinging(false);
    if (alarm_ok)
    {
        display.Show({Glyph::A, Glyph::NONE, Glyph::O, Glyph::K});
    }
    else
    {
        display.Show({Glyph::A, Glyph::B, Glyph::A, Glyph::D});
    }
    Eh14::WaitForDisplay();
    Eh14::Delay(1000);
    display.Clear();
    Eh14::WaitForDisplay();

    // The light sensor, without and with the status LED next to it.
    display.ShowNumber(Eh14::analog_inputs.ReadLight());
    Eh14::WaitForDisplay();
    Eh14::Delay(1000);
    Eh14::status_led.Set(true);
    Eh14::Delay(100);
    display.ShowNumber(Eh14::analog_inputs.ReadLight());
    Eh14::WaitForDisplay();
    Eh14::Delay(1000);
    Eh14::status_led.Set(false);
    display.Clear();
    Eh14::WaitForDisplay();
}

} // namespace eh14
