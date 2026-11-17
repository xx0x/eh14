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

namespace eh14
{

/**
 * @class RealTimeClock
 * @brief The DS3231 RTC (U6, or the DS3231M at U6_ALT): the time and the alarm.
 * @details
 * The DS3231 keeps time on the battery regardless of the power switch (VBAT is the cell itself). Only hours, minutes
 * and seconds are used, in 24-hour mode; setting the time also writes the fixed date 2020-03-31.
 *
 * If the oscillator stopped (OSF: the battery went flat or was removed), @ref Init restarts the time from
 * defaults::kHourAfterPowerLoss.
 *
 * The alarm is alarm 1, matching hours, minutes and seconds, once a day. On a match the chip sets A1F and, with A1IE
 * set, pulls INT/SQW low until the flag is cleared. The falling edge wakes the clock; Eh14 notes it here from the EIC
 * interrupt (@ref MarkAlarm, @ref TakeAlarm), and the main loop clears the flag over I2C. A1IE is the menu's "alarm
 * on" (C); setting the alarm time (D) also enables it.
 *
 * Alarm 2 is not used: it would pull INT/SQW low too, and ring the alarm.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class RealTimeClock
{
public:
    struct Time
    {
        std::uint8_t hour = 0;
        std::uint8_t minute = 0;
        std::uint8_t second = 0;
    };

    static constexpr std::uint8_t kAddress = 0x68;

    /**
     * @brief Restarts the time if the chip lost it, clears a stale alarm flag and sets up the alarm pin. Call before
     *        enabling its EIC line: a stale flag holds the line low, so no edge would come.
     * @return False if the chip does not answer.
     */
    bool Init();

    Time GetTime();

    /**
     * @brief Sets the time and the fixed date, which also clears OSF.
     */
    bool SetTime(const Time &time);

    Time GetAlarm();

    /**
     * @brief Sets the alarm time and enables the alarm.
     */
    bool SetAlarm(const Time &time);

    bool IsAlarmEnabled();

    /**
     * @brief Enables the alarm, clearing a match flagged while it was off - otherwise it would ring at once.
     */
    bool EnableAlarm();

    bool DisableAlarm();

    /**
     * @brief Clears the alarm flag, releasing INT/SQW for the next match.
     */
    bool ClearAlarmFlag();

    /**
     * @brief Notes the alarm's edge. Interrupt context.
     */
    void MarkAlarm()
    {
        alarm_pending_ = true;
    }

    /**
     * @brief Whether the alarm went off since the last call; clears it.
     */
    bool TakeAlarm()
    {
        const bool alarm = alarm_pending_;
        alarm_pending_ = false;
        return alarm;
    }

    /**
     * @brief Whether the alarm went off and has not been taken yet.
     */
    bool IsAlarmPending() const
    {
        return alarm_pending_;
    }

private:
    // DS3231 registers. Time and alarm are BCD.
    static constexpr std::uint8_t kRegSeconds = 0x00; // seconds, minutes, hours, day, date, month, year follow
    static constexpr std::uint8_t kRegAlarm1 = 0x07;  // seconds, minutes, hours, day/date
    static constexpr std::uint8_t kRegControl = 0x0E;
    static constexpr std::uint8_t kRegStatus = 0x0F;

    static constexpr std::uint8_t kControlA1ie = 0x01;
    static constexpr std::uint8_t kControlIntcn = 0x04;
    static constexpr std::uint8_t kStatusA1f = 0x01;
    static constexpr std::uint8_t kStatusOsf = 0x80;

    // The date written with every time: 2020-03-31, a Tuesday (the DS3231 counts Monday as 1).
    static constexpr std::uint8_t kDateWeekday = 2;
    static constexpr std::uint8_t kDateDay = 31;
    static constexpr std::uint8_t kDateMonth = 3;
    static constexpr std::uint8_t kDateYear = 20;

    // Alarm 1's day/date register: A1M4 set, so the day is ignored and the alarm matches daily. The date (14) is
    // never used.
    static constexpr std::uint8_t kAlarmDayIgnored = 0x80 | 0x14;

    static constexpr std::uint8_t ToBcd(std::uint8_t value)
    {
        return static_cast<std::uint8_t>(((value / 10U) << 4) | (value % 10U));
    }

    static constexpr std::uint8_t FromBcd(std::uint8_t bcd)
    {
        return static_cast<std::uint8_t>(((bcd >> 4) * 10U) + (bcd & 0x0FU));
    }

    bool ReadRegister(std::uint8_t reg, std::uint8_t &value);
    bool WriteRegister(std::uint8_t reg, std::uint8_t value);
    bool UpdateRegister(std::uint8_t reg, std::uint8_t clear, std::uint8_t set);

    volatile bool alarm_pending_ = false;
};

} // namespace eh14
