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

#include "hardware/IrReceiver.hpp"

#include "Eh14.hpp"
#include "hardware/Board.hpp"
#include "hardware/Gpio.hpp"
#include "hardware/Timebase.hpp"

namespace eh14
{

bool IrReceiver::Init()
{
    Gpio::SetInput(Board::kIrData, Gpio::Pull::UP);
    for (const Pin pin : {Board::kIrLatch, Board::kIrClock})
    {
        Gpio::SetInput(pin, Gpio::Pull::UP);
        Gpio::SetFunction(pin, Gpio::Function::A);
    }

    Eh14::eic.Attach(Board::kIrLatchLine, ExternalInterrupts::Edge::FALLING, true, HandleLatchEdge, this);
    Eh14::eic.Attach(Board::kIrClockLine, ExternalInterrupts::Edge::RISING, false, HandleClockEdge, this);
    return true;
}

bool IrReceiver::IsReceiving() const
{
    return receiving_ && (Timebase::GetMillis() - latch_ms_) < kMessageTimeoutMs;
}

void IrReceiver::HandleLatch()
{
    Eh14::buttons.RequestStop();
    value_ = 0;
    bit_count_ = 0;
    latch_ms_ = Timebase::GetMillis();
    receiving_ = true;
    Eh14::status_led.Set(true);
}

void IrReceiver::HandleClock()
{
    if (!receiving_ || bit_count_ >= 8U)
    {
        return;
    }
    if (Gpio::Read(Board::kIrData))
    {
        value_ = static_cast<std::uint8_t>(value_ | (0x80U >> bit_count_));
    }
    bit_count_ = static_cast<std::uint8_t>(bit_count_ + 1U);
    if (bit_count_ < 8U)
    {
        return;
    }

    switch (value_)
    {
    case kMessageSnooze:
        Eh14::buttons.Press(Button::SNOOZE);
        break;
    case kMessageMenu:
        Eh14::buttons.Press(Button::MENU);
        break;
    case kMessageChange:
        Eh14::buttons.Press(Button::CHANGE);
        break;
    case kMessageSound:
        Eh14::buttons.Press(Button::SOUND);
        break;
    default:
        // The ATtiny sends 0 at startup; any other value is not a button either.
        break;
    }
    Eh14::status_led.Set(false);
    receiving_ = false;
}

void IrReceiver::HandleLatchEdge(void *context)
{
    static_cast<IrReceiver *>(context)->HandleLatch();
}

void IrReceiver::HandleClockEdge(void *context)
{
    static_cast<IrReceiver *>(context)->HandleClock();
}

} // namespace eh14
