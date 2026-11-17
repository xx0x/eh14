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

#include "hardware/Buttons.hpp"

#include "Eh14.hpp"
#include "Eh14Params.hpp"
#include "hardware/Board.hpp"
#include "hardware/Gpio.hpp"
#include "hardware/Timebase.hpp"

namespace eh14
{

bool Buttons::Init()
{
    for (const Pin pin : {Board::kSnoozeButton, Board::kMenuButton, Board::kChangeButton})
    {
        Gpio::SetInput(pin, Gpio::Pull::UP);
        Gpio::SetFunction(pin, Gpio::Function::A);
    }

    Eh14::eic.Attach(Board::kSnoozeLine, ExternalInterrupts::Edge::FALLING, true, HandleSnoozeEdge, this);
    Eh14::eic.Attach(Board::kMenuLine, ExternalInterrupts::Edge::FALLING, true, HandleMenuEdge, this);
    Eh14::eic.Attach(Board::kChangeLine, ExternalInterrupts::Edge::FALLING, true, HandleChangeEdge, this);
    return true;
}

void Buttons::Press(Button button)
{
    const std::size_t index = Index(button);
    const std::uint32_t now = Timebase::GetMillis();
    if ((now - last_press_ms_[index]) < params::kDebounceMs)
    {
        return;
    }
    last_press_ms_[index] = now;
    stop_requested_ = true;
    if (!alarm_ringing_)
    {
        pressed_[index] = true;
    }
}

bool Buttons::IsHeld(Button button) const
{
    switch (button)
    {
    case Button::SNOOZE:
        return !Gpio::Read(Board::kSnoozeButton);
    case Button::MENU:
        return !Gpio::Read(Board::kMenuButton);
    case Button::CHANGE:
        return !Gpio::Read(Board::kChangeButton);
    default:
        return false;
    }
}

void Buttons::HandleSnoozeEdge(void *context)
{
    static_cast<Buttons *>(context)->Press(Button::SNOOZE);
}

void Buttons::HandleMenuEdge(void *context)
{
    static_cast<Buttons *>(context)->Press(Button::MENU);
}

void Buttons::HandleChangeEdge(void *context)
{
    static_cast<Buttons *>(context)->Press(Button::CHANGE);
}

} // namespace eh14
