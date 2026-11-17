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

#include "app/TimeEditor.hpp"

namespace eh14
{

void TimeEditor::Begin(const RealTimeClock::Time &time)
{
    digits_ = {static_cast<std::uint8_t>(time.hour / 10U), static_cast<std::uint8_t>(time.hour % 10U),
               static_cast<std::uint8_t>(time.minute / 10U), static_cast<std::uint8_t>(time.minute % 10U)};
    current_ = 0;
}

void TimeEditor::Increase()
{
    if (!IsActive() || IsDone())
    {
        return;
    }
    std::uint8_t &digit = digits_[current_];
    digit = static_cast<std::uint8_t>(digit + 1U);
    if (digit > 9U)
    {
        digit = 0;
    }
    if (current_ == 0 && digits_[0] > 2U)
    {
        digits_[0] = 0;
    }
    if (current_ == 1 && digits_[0] == 2U && digits_[1] > 3U)
    {
        digits_[1] = 0;
    }
    if (current_ == 2 && digits_[2] > 5U)
    {
        digits_[2] = 0;
    }
}

void TimeEditor::Next()
{
    current_ = static_cast<std::int8_t>(current_ + 1);
    if (digits_[0] > 1U && digits_[1] > 3U)
    {
        digits_[1] = 0;
    }
}

} // namespace eh14
