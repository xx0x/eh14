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
#include <string_view>

namespace eh14
{

/**
 * @class UsbSerial
 * @brief The USB-C port as a serial port (TinyUSB CDC): the log, the commands (SerialProtocol) and the bootloader
 *        request.
 * @details
 * The port exists only while the clock is awake: it detaches before every sleep (@ref Disconnect) and reattaches
 * after. Open the menu first to keep the clock awake.
 *
 * Text goes to whoever has the port open; with nobody reading, TinyUSB keeps the newest text and drops the oldest.
 * Every line ends in CR LF.
 *
 * ### The bootloader
 * Opening the port at 1200 baud and closing it (DTR dropped at 1200) requests the bootloader, as the Arduino IDE
 * does. 250ms later, giving the host time to release the port, the clock writes the bootloader's magic word at the
 * top of RAM and resets (Eh14::EnterBootloader). Any other line state change in between cancels the request. The UF2
 * bootloader then stays active as a USB drive.
 *
 * Everything except the interrupt itself runs from the main loop (@ref Task, from Eh14::RunBackground), callbacks
 * included.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class UsbSerial
{
public:
    /**
     * @brief The word at the top of RAM that keeps the UF2 bootloader from starting the firmware after a reset.
     */
    static constexpr std::uint32_t kBootloaderMagic = 0xF01669EF;
    static constexpr std::uintptr_t kBootloaderMagicAddress = 0x20007FFC;

    bool Init();

    /**
     * @brief Runs TinyUSB's device task. Call continuously (Eh14::RunBackground).
     */
    void Task();

    /**
     * @brief Detaches from the bus (D+ pull-up off) before sleep.
     */
    void Disconnect();

    void Connect();

    /**
     * @brief How many received bytes are waiting.
     */
    std::uint32_t Available() const;

    /**
     * @brief Takes as many received bytes as fit into @p data.
     * @return How many it took.
     */
    std::uint32_t Read(std::span<std::uint8_t> data);

    void Print(std::string_view text);
    void Print(std::uint32_t value);

    /**
     * @brief @p value in uppercase hex, without leading zeros.
     */
    void PrintHex(std::uint32_t value);

    void Println(std::string_view text = {});
    void Println(std::uint32_t value);

    /**
     * @brief Whether the host requested the bootloader at least 250ms ago and has not cancelled.
     */
    bool IsBootloaderRequested() const;

    /**
     * @brief Called by TinyUSB's CDC when the host changes DTR/RTS.
     */
    void HandleLineState(bool dtr);

private:
    static constexpr std::uint32_t kBootloaderBaud = 1200;
    static constexpr std::uint32_t kBootloaderDelayMs = 250;

    /**
     * @brief How long a write waits for room in a full TX buffer while the port is open, in ms. The rest is then
     *        dropped, so a host that stopped reading cannot hang the clock.
     */
    static constexpr std::uint32_t kWriteTimeoutMs = 50;

    bool bootloader_requested_ = false;
    std::uint32_t bootloader_request_ms_ = 0;
};

} // namespace eh14
