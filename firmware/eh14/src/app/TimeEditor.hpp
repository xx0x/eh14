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

#include <array>
#include <cstdint>

#include "hardware/RealTimeClock.hpp"

namespace eh14
{

/**
 * @class TimeEditor
 * @brief Sets a time digit by digit for the menu's time items (D, E): CHANGE increments the current digit, MENU moves
 *        to the next, and after the fourth the time is set.
 * @details Each digit counts only through valid values: tens of hours 0..2, hours 0..3 after a 2, tens of minutes
 *          0..5. An hour made invalid by changing its tens (19 becoming 29) restarts from 0 when the next digit is
 *          reached.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class TimeEditor
{
public:
    static constexpr std::uint8_t kDigits = 4;

    /**
     * @brief Starts at the first digit, from @p time.
     */
    void Begin(const RealTimeClock::Time &time);

    void End()
    {
        current_ = kInactive;
    }

    bool IsActive() const
    {
        return current_ != kInactive;
    }

    /**
     * @brief Whether every digit has been set (MENU after the last one).
     */
    bool IsDone() const
    {
        return IsActive() && current_ >= kDigits;
    }

    /**
     * @brief The digit being set, 0..3.
     */
    std::uint8_t GetCurrent() const
    {
        return static_cast<std::uint8_t>(current_);
    }

    std::uint8_t GetDigit(std::uint8_t index) const
    {
        return digits_[index % kDigits];
    }

    void Increase();

    void Next();

    RealTimeClock::Time GetTime() const
    {
        return {static_cast<std::uint8_t>((digits_[0] * 10U) + digits_[1]),
                static_cast<std::uint8_t>((digits_[2] * 10U) + digits_[3]), 0};
    }

private:
    static constexpr std::int8_t kInactive = -1;

    std::array<std::uint8_t, kDigits> digits_ = {};
    std::int8_t current_ = kInactive;
};

} // namespace eh14
