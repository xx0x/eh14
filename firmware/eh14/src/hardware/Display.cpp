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

#include "hardware/Display.hpp"

#include "Eh14Params.hpp"
#include "hardware/Board.hpp"
#include "hardware/Gpio.hpp"
#include "hardware/Timebase.hpp"

namespace eh14
{

bool Display::Init()
{
    // SegmentDriver set up the pins at reset. The boost converter stays off until the rest of the clock is up.
    SetPower(false);
    return true;
}

void Display::SetPower(bool on)
{
    Gpio::Write(Board::kDisplayEnable, on);
}

void Display::Show(const Glyphs &glyphs, bool force)
{
    order_ = Order::FORWARD;
    for (std::uint8_t digit = 0; digit < kDigits; digit++)
    {
        target_[digit] = SegmentsOf(glyphs[digit]);
        if (force)
        {
            forced_[digit] = kAllSegments;
        }
    }
}

void Display::ShowDigit(std::uint8_t digit, Glyph glyph, bool force)
{
    if (digit >= kDigits)
    {
        return;
    }
    order_ = Order::FORWARD;
    target_[digit] = SegmentsOf(glyph);
    if (force)
    {
        forced_[digit] = kAllSegments;
    }
}

void Display::ShowTime(std::uint8_t hour, std::uint8_t minute, bool force)
{
    Show({Digit(hour / 10U), Digit(hour), Digit(minute / 10U), Digit(minute)}, force);
}

void Display::ShowNumber(std::uint16_t number)
{
    Show({Digit(static_cast<std::uint8_t>((number / 1000U) % 10U)), Digit(static_cast<std::uint8_t>((number / 100U) % 10U)),
          Digit(static_cast<std::uint8_t>((number / 10U) % 10U)), Digit(static_cast<std::uint8_t>(number % 10U))});
}

void Display::Clear(bool force)
{
    Show({Glyph::NONE, Glyph::NONE, Glyph::NONE, Glyph::NONE}, force);
}

void Display::ClearBackwards(bool force)
{
    Clear(force);
    order_ = Order::BACKWARD;
}

void Display::Update()
{
    if (flipped_ && (Timebase::GetMillis() - last_flip_ms_) < params::kSegmentGapMs)
    {
        return;
    }
    std::uint8_t digit = 0;
    std::uint8_t segment = 0;
    if (!FindNext(digit, segment))
    {
        return;
    }

    const std::uint8_t mask = static_cast<std::uint8_t>(1U << segment);
    const bool on = (target_[digit] & mask) != 0U;
    SegmentDriver::Pulse(digit, segment, on);
    shown_[digit] = static_cast<std::uint8_t>(on ? (shown_[digit] | mask) : (shown_[digit] & ~mask));
    forced_[digit] = static_cast<std::uint8_t>(forced_[digit] & ~mask);
    last_flip_ms_ = Timebase::GetMillis();
    flipped_ = true;
}

bool Display::IsIdle() const
{
    return !HasPending() && (!flipped_ || (Timebase::GetMillis() - last_flip_ms_) >= params::kSegmentGapMs);
}

bool Display::HasPending() const
{
    for (std::uint8_t digit = 0; digit < kDigits; digit++)
    {
        if (((shown_[digit] ^ target_[digit]) | forced_[digit]) != 0U)
        {
            return true;
        }
    }
    return false;
}

bool Display::FindNext(std::uint8_t &digit, std::uint8_t &segment) const
{
    for (std::uint8_t step = 0; step < kDigits * SegmentDriver::kSegments; step++)
    {
        const std::uint8_t index =
            (order_ == Order::FORWARD) ? step : static_cast<std::uint8_t>((kDigits * SegmentDriver::kSegments) - 1U - step);
        const std::uint8_t d = index / SegmentDriver::kSegments;
        const std::uint8_t s = index % SegmentDriver::kSegments;
        const std::uint8_t pending = static_cast<std::uint8_t>((shown_[d] ^ target_[d]) | forced_[d]);
        if (((pending >> s) & 1U) != 0U)
        {
            digit = d;
            segment = s;
            return true;
        }
    }
    return false;
}

} // namespace eh14
