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

#include "hardware/RealTimeClock.hpp"

#include <array>

#include "Eh14.hpp"
#include "Eh14Defaults.hpp"
#include "hardware/Board.hpp"
#include "hardware/Gpio.hpp"

namespace eh14
{

bool RealTimeClock::Init()
{
    // INT/SQW is open drain with its own pull-up (R10).
    Gpio::SetInput(Board::kRtcAlarm, Gpio::Pull::NONE);
    Gpio::SetFunction(Board::kRtcAlarm, Gpio::Function::A);

    std::uint8_t status = 0;
    if (!ReadRegister(kRegStatus, status))
    {
        return false;
    }
    if ((status & kStatusOsf) != 0U)
    {
        SetTime({defaults::kHourAfterPowerLoss, defaults::kMinuteAfterPowerLoss, 0});
    }

    // INT/SQW as the alarm interrupt, not a square wave (the chip's default, set explicitly). Clear any earlier
    // match, or it would hold the line low and no edge would come.
    return UpdateRegister(kRegControl, 0, kControlIntcn) && ClearAlarmFlag();
}

RealTimeClock::Time RealTimeClock::GetTime()
{
    std::array<std::uint8_t, 3> regs = {};
    if (!Eh14::i2c.ReadRegisters(kAddress, kRegSeconds, regs))
    {
        return {};
    }
    return {FromBcd(regs[2] & 0x3FU), FromBcd(regs[1] & 0x7FU), FromBcd(regs[0] & 0x7FU)};
}

bool RealTimeClock::SetTime(const Time &time)
{
    const std::array<std::uint8_t, 8> data = {
        kRegSeconds,          ToBcd(time.second),  ToBcd(time.minute),  ToBcd(time.hour),
        ToBcd(kDateWeekday),  ToBcd(kDateDay),     ToBcd(kDateMonth),   ToBcd(kDateYear),
    };
    if (!Eh14::i2c.Write(kAddress, data))
    {
        return false;
    }
    return UpdateRegister(kRegStatus, kStatusOsf, 0);
}

RealTimeClock::Time RealTimeClock::GetAlarm()
{
    std::array<std::uint8_t, 3> regs = {};
    if (!Eh14::i2c.ReadRegisters(kAddress, kRegAlarm1, regs))
    {
        return {};
    }
    return {FromBcd(regs[2] & 0x3FU), FromBcd(regs[1] & 0x7FU), FromBcd(regs[0] & 0x7FU)};
}

bool RealTimeClock::SetAlarm(const Time &time)
{
    if (!ClearAlarmFlag())
    {
        return false;
    }
    const std::array<std::uint8_t, 5> data = {
        kRegAlarm1, ToBcd(time.second), ToBcd(time.minute), ToBcd(time.hour), kAlarmDayIgnored,
    };
    if (!Eh14::i2c.Write(kAddress, data))
    {
        return false;
    }
    return UpdateRegister(kRegControl, 0, kControlIntcn | kControlA1ie);
}

bool RealTimeClock::IsAlarmEnabled()
{
    std::uint8_t control = 0;
    return ReadRegister(kRegControl, control) && (control & kControlA1ie) != 0U;
}

bool RealTimeClock::EnableAlarm()
{
    return ClearAlarmFlag() && UpdateRegister(kRegControl, 0, kControlIntcn | kControlA1ie);
}

bool RealTimeClock::DisableAlarm()
{
    return UpdateRegister(kRegControl, kControlA1ie, 0);
}

bool RealTimeClock::ClearAlarmFlag()
{
    return UpdateRegister(kRegStatus, kStatusA1f, 0);
}

bool RealTimeClock::ReadRegister(std::uint8_t reg, std::uint8_t &value)
{
    return Eh14::i2c.ReadRegisters(kAddress, reg, std::span<std::uint8_t>(&value, 1));
}

bool RealTimeClock::WriteRegister(std::uint8_t reg, std::uint8_t value)
{
    const std::array<std::uint8_t, 2> data = {reg, value};
    return Eh14::i2c.Write(kAddress, data);
}

bool RealTimeClock::UpdateRegister(std::uint8_t reg, std::uint8_t clear, std::uint8_t set)
{
    std::uint8_t value = 0;
    if (!ReadRegister(reg, value))
    {
        return false;
    }
    return WriteRegister(reg, static_cast<std::uint8_t>((value & ~clear) | set));
}

} // namespace eh14
