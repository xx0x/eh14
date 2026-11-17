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
 * @class SpiBus
 * @brief SERCOM4 as an SPI master, mode 0 at 12MHz, for the flash, the only device on the bus.
 * @details Polled, one byte at a time: the audio reads 2K every 23ms, which takes 2ms. The chip select belongs to
 *          SpiFlash.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class SpiBus
{
public:
    bool Init();

    std::uint8_t Transfer(std::uint8_t byte);

    void Write(std::span<const std::uint8_t> data);

    void Read(std::span<std::uint8_t> data);

private:
    static constexpr std::uint8_t kBaud = 1;
};

} // namespace eh14
