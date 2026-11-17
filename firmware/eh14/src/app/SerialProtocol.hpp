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

#include "hardware/SpiFlash.hpp"

namespace eh14
{

/**
 * @class SerialProtocol
 * @brief The commands a host sends over the serial port: identify, set the time, upload a sound bank.
 * @details
 * A command is one byte; whatever follows belongs to it until the line has been quiet for @ref kSessionTimeoutMs. The
 * answers are log lines a host parses (SoundBank logs the bank, this class the rest), so their wording is part of the
 * protocol.
 *
 * - @ref kCommandDevice ('?'): answers "Device: EH14" and the capacity for sounds (the flash minus its first sector).
 * - @ref kCommandSetTime ('%'): six bytes follow, the digits of HHMMSS as numbers 0..9 (not ASCII). Once the line is
 *   quiet, the time is set, shown and announced.
 * - @ref kCommandUpload ('@'): the whole flash is erased (settings included) and everything that follows is the bank
 *   (SoundBank), written page by page from SoundBank::kHeaderAddress. "Bytes written: N" reports progress, "Total
 *   bytes: N" the end. Once the line is quiet, the bank is reloaded.
 *
 * @ref Poll returns at once if nothing was received; otherwise it stays until the line goes quiet, calling
 * Eh14::RunBackground, which keeps the clock awake through an upload.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class SerialProtocol
{
public:
    static constexpr std::uint8_t kCommandUpload = '@';
    static constexpr std::uint8_t kCommandDevice = '?';
    static constexpr std::uint8_t kCommandSetTime = '%';

    static constexpr std::uint32_t kSessionTimeoutMs = 1000;

    void Poll();

private:
    enum class Mode
    {
        NONE,
        UPLOAD,
        SET_TIME,
    };

    /**
     * @brief Upload progress report interval, in bytes; the status LED toggles with each report.
     */
    static constexpr std::uint32_t kReportEveryBytes = 8192;

    static constexpr std::size_t kTimeDigits = 6;

    /**
     * @brief The longest a chip erase may take, in ms (the AT25SF641 takes tens of seconds).
     */
    static constexpr std::uint32_t kChipEraseTimeoutMs = 200000;

    void HandleByte(std::uint8_t byte);
    void BeginUpload();
    void UploadByte(std::uint8_t byte);
    void EndUpload();
    void SetTime();

    Mode mode_ = Mode::NONE;

    std::array<std::uint8_t, kTimeDigits> time_digits_ = {};
    std::size_t time_count_ = 0;

    std::array<std::uint8_t, SpiFlash::kPageSize> page_ = {};
    std::size_t page_fill_ = 0;
    std::uint32_t address_ = 0;
    std::uint32_t next_report_ = 0;
    bool led_on_ = false;
};

} // namespace eh14
