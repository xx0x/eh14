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

#include "hardware/I2cBus.hpp"

#include "hardware/Board.hpp"
#include "hardware/Clocks.hpp"
#include "hardware/Gpio.hpp"
#include "hardware/Timebase.hpp"
#include "sam.h"

namespace eh14
{

bool I2cBus::Init()
{
    PM->APBCMASK.reg |= PM_APBCMASK_SERCOM3;
    Clocks::Connect(GCLK_CLKCTRL_ID_SERCOM3_CORE_Val, Clocks::kMainGenerator);

    SERCOM3->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_SWRST;
    while (SERCOM3->I2CM.CTRLA.bit.SWRST || SERCOM3->I2CM.SYNCBUSY.bit.SWRST)
    {
    }

    SERCOM3->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_MODE_I2C_MASTER;
    SERCOM3->I2CM.BAUD.reg = SERCOM_I2CM_BAUD_BAUD(kBaud);
    SERCOM3->I2CM.CTRLA.reg |= SERCOM_I2CM_CTRLA_ENABLE;
    WaitSync();

    // The bus state starts unknown; with no other master on the bus it must be idle.
    SERCOM3->I2CM.STATUS.reg = SERCOM_I2CM_STATUS_BUSSTATE(1);
    WaitSync();

    Gpio::SetFunction(Board::kI2cSda, Gpio::Function::C);
    Gpio::SetFunction(Board::kI2cScl, Gpio::Function::C);
    return true;
}

bool I2cBus::Write(std::uint8_t address, std::span<const std::uint8_t> data)
{
    if (!StartWrite(address))
    {
        return false;
    }
    for (const std::uint8_t byte : data)
    {
        SERCOM3->I2CM.DATA.reg = byte;
        WaitSync();
        if (!WaitFlag(SERCOM_I2CM_INTFLAG_MB) || SERCOM3->I2CM.STATUS.bit.RXNACK)
        {
            SendCommand(Command::STOP, false);
            return false;
        }
    }
    SendCommand(Command::STOP, false);
    return true;
}

bool I2cBus::ReadRegisters(std::uint8_t address, std::uint8_t reg, std::span<std::uint8_t> data)
{
    if (data.empty() || !Write(address, std::span<const std::uint8_t>(&reg, 1)))
    {
        return false;
    }

    SERCOM3->I2CM.ADDR.reg = SERCOM_I2CM_ADDR_ADDR((address << 1U) | 1U);
    WaitSync();
    for (std::size_t i = 0; i < data.size(); i++)
    {
        if (!WaitFlag(SERCOM_I2CM_INTFLAG_SB))
        {
            SendCommand(Command::STOP, true);
            return false;
        }
        const bool last = (i + 1U) == data.size();
        // ACK or NACK goes out with the next command: the last byte is NACKed, then STOP.
        SERCOM3->I2CM.CTRLB.bit.ACKACT = last ? 1U : 0U;
        WaitSync();
        data[i] = static_cast<std::uint8_t>(SERCOM3->I2CM.DATA.reg);
        WaitSync();
        SendCommand(last ? Command::STOP : Command::READ_NEXT, last);
    }
    return true;
}

bool I2cBus::StartWrite(std::uint8_t address)
{
    SERCOM3->I2CM.ADDR.reg = SERCOM_I2CM_ADDR_ADDR(address << 1U);
    WaitSync();
    if (!WaitFlag(SERCOM_I2CM_INTFLAG_MB) || SERCOM3->I2CM.STATUS.bit.RXNACK)
    {
        SendCommand(Command::STOP, false);
        return false;
    }
    return true;
}

void I2cBus::SendCommand(Command command, bool nack)
{
    SERCOM3->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(static_cast<std::uint8_t>(command)) |
                              (nack ? SERCOM_I2CM_CTRLB_ACKACT : 0U);
    WaitSync();
}

bool I2cBus::WaitFlag(std::uint8_t flag)
{
    const std::uint32_t start = Timebase::GetMillis();
    while ((SERCOM3->I2CM.INTFLAG.reg & flag) == 0U)
    {
        if ((SERCOM3->I2CM.STATUS.reg & (SERCOM_I2CM_STATUS_BUSERR | SERCOM_I2CM_STATUS_ARBLOST)) != 0U ||
            (Timebase::GetMillis() - start) > kTimeoutMs)
        {
            // Bus error or timeout: force the bus state back to idle for the next transfer.
            SERCOM3->I2CM.STATUS.reg = SERCOM_I2CM_STATUS_BUSERR | SERCOM_I2CM_STATUS_ARBLOST |
                                       SERCOM_I2CM_STATUS_BUSSTATE(1);
            WaitSync();
            return false;
        }
    }
    return true;
}

void I2cBus::WaitSync()
{
    while (SERCOM3->I2CM.SYNCBUSY.reg != 0U)
    {
    }
}

} // namespace eh14
