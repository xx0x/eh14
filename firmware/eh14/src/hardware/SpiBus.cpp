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

#include "hardware/SpiBus.hpp"

#include "hardware/Board.hpp"
#include "hardware/Clocks.hpp"
#include "hardware/Gpio.hpp"
#include "sam.h"

namespace eh14
{

bool SpiBus::Init()
{
    PM->APBCMASK.reg |= PM_APBCMASK_SERCOM4;
    Clocks::Connect(GCLK_CLKCTRL_ID_SERCOM4_CORE_Val, Clocks::kMainGenerator);

    SERCOM4->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_SWRST;
    while (SERCOM4->SPI.CTRLA.bit.SWRST || SERCOM4->SPI.SYNCBUSY.bit.SWRST)
    {
    }

    // Data out on pad 2 with the clock on pad 3 (DOPO 1), data in on pad 0 (DIPO 0); mode 0, MSB first.
    SERCOM4->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_MODE_SPI_MASTER | SERCOM_SPI_CTRLA_DOPO(1) | SERCOM_SPI_CTRLA_DIPO(0);
    SERCOM4->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_RXEN;
    while (SERCOM4->SPI.SYNCBUSY.bit.CTRLB)
    {
    }
    SERCOM4->SPI.BAUD.reg = kBaud;
    SERCOM4->SPI.CTRLA.reg |= SERCOM_SPI_CTRLA_ENABLE;
    while (SERCOM4->SPI.SYNCBUSY.bit.ENABLE)
    {
    }

    Gpio::SetFunction(Board::kSpiMiso, Gpio::Function::D);
    Gpio::SetFunction(Board::kSpiMosi, Gpio::Function::D);
    Gpio::SetFunction(Board::kSpiSck, Gpio::Function::D);
    return true;
}

std::uint8_t SpiBus::Transfer(std::uint8_t byte)
{
    while (!SERCOM4->SPI.INTFLAG.bit.DRE)
    {
    }
    SERCOM4->SPI.DATA.reg = byte;
    while (!SERCOM4->SPI.INTFLAG.bit.RXC)
    {
    }
    return static_cast<std::uint8_t>(SERCOM4->SPI.DATA.reg);
}

void SpiBus::Write(std::span<const std::uint8_t> data)
{
    for (const std::uint8_t byte : data)
    {
        Transfer(byte);
    }
}

void SpiBus::Read(std::span<std::uint8_t> data)
{
    for (std::uint8_t &byte : data)
    {
        byte = Transfer(0);
    }
}

} // namespace eh14
