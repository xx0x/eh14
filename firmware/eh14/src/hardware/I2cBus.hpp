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
#include <span>

namespace eh14
{

/**
 * @class I2cBus
 * @brief SERCOM3 as an I2C master at 100kHz (101kHz exactly) for the DS3231, the only device on the bus.
 * @details Blocking, polled, main loop only. Every wait has a timeout, so a missing or stuck chip returns false
 *          within a few ms instead of hanging.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class I2cBus
{
public:
    bool Init();

    /**
     * @brief Writes @p data to the device at 7-bit @p address.
     */
    bool Write(std::uint8_t address, std::span<const std::uint8_t> data);

    /**
     * @brief Writes @p reg, then reads @p data after a repeated start.
     */
    bool ReadRegisters(std::uint8_t address, std::uint8_t reg, std::span<std::uint8_t> data);

private:
    static constexpr std::uint8_t kBaud = 232;
    static constexpr std::uint32_t kTimeoutMs = 10;

    enum class Command : std::uint8_t
    {
        READ_NEXT = 2,
        STOP = 3,
    };

    static void WaitSync();
    static bool WaitFlag(std::uint8_t flag);
    static void SendCommand(Command command, bool nack);
    static bool StartWrite(std::uint8_t address);
};

} // namespace eh14
