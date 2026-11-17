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

#include "hardware/SegmentDriver.hpp"

#include "Eh14Params.hpp"
#include "hardware/Board.hpp"
#include "hardware/Gpio.hpp"
#include "hardware/Timebase.hpp"

namespace eh14
{

void SegmentDriver::Init()
{
    Gpio::SetOutput(Board::kDisplayEnable, false);
    Gpio::SetOutput(Board::kDisplayLatch, false);
    Gpio::SetOutput(Board::kDisplayData, false);
    Gpio::SetOutput(Board::kDisplayClock, false);
    Clear();
}

void SegmentDriver::Clear()
{
    Gpio::Write(Board::kDisplayLatch, false);
    ShiftOut(0);
    Gpio::Write(Board::kDisplayLatch, true);
}

bool SegmentDriver::Pulse(std::uint8_t digit, std::uint8_t segment, bool on)
{
    if (digit >= kDigits || segment >= kSegments)
    {
        return false;
    }
    const std::uint64_t frame = Frame(digit, segment, on);
    if (!IsSafe(frame))
    {
        return false;
    }

    Gpio::Write(Board::kDisplayLatch, false);
    ShiftOut(frame);
    Gpio::Write(Board::kDisplayLatch, true);
    const std::uint32_t start = Timebase::GetMicros();

    // Shift the zeros in while the coil is on; the outputs take them at the latch's rising edge.
    Gpio::Write(Board::kDisplayLatch, false);
    ShiftOut(0);
    while ((Timebase::GetMicros() - start) < params::kSegmentPulseUs)
    {
    }
    Gpio::Write(Board::kDisplayLatch, true);
    return true;
}

void SegmentDriver::ShiftOut(std::uint64_t frame)
{
    for (int bit = 63; bit >= 0; bit--)
    {
        Gpio::Write(Board::kDisplayData, ((frame >> bit) & 1U) != 0U);
        BitDelay();
        Gpio::Write(Board::kDisplayClock, true);
        BitDelay();
        Gpio::Write(Board::kDisplayClock, false);
    }
    Gpio::Write(Board::kDisplayData, false);
    BitDelay();
}

void SegmentDriver::BitDelay()
{
    // About 1.5us at 48MHz (the loop is 4 or 5 cycles), longer before the clocks are up. Only a lower bound matters.
    for (std::uint32_t i = 16; i > 0; i--)
    {
        __asm volatile("nop");
    }
}

} // namespace eh14
