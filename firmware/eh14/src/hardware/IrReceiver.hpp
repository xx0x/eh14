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
 * @class IrReceiver
 * @brief The remote control, through the ATtiny85 on the main board (U8).
 * @details The ATtiny decodes the remote's NEC codes (firmware/eh14_ir_reciever) and sends each button as one byte
 *          over three wires: TINY_LATCH low, then the byte MSB first (data set, TINY_CLOCK high for 9ms, low for
 *          1ms), then the latch released. The bits are read on the clock's rising edges in the EIC interrupt, and the
 *          complete byte is pressed as a button.
 *
 * The latch's edge wakes the clock, stops whatever is playing and lights the status LED until the byte is in.
 * Meanwhile @ref IsReceiving keeps the clock awake, for at most @ref kMessageTimeoutMs in case the ATtiny stops
 * halfway.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class IrReceiver
{
public:
    static constexpr std::uint8_t kMessageSnooze = 0xA1;
    static constexpr std::uint8_t kMessageMenu = 0xA2;
    static constexpr std::uint8_t kMessageChange = 0xA3;
    static constexpr std::uint8_t kMessageSound = 0xA4;

    bool Init();

    bool IsReceiving() const;

private:
    /**
     * @brief How long a byte may take, in ms (a byte takes 90).
     */
    static constexpr std::uint32_t kMessageTimeoutMs = 200;

    static void HandleLatchEdge(void *context);
    static void HandleClockEdge(void *context);

    void HandleLatch();
    void HandleClock();

    volatile std::uint8_t value_ = 0;
    volatile std::uint8_t bit_count_ = 0;
    volatile bool receiving_ = false;
    volatile std::uint32_t latch_ms_ = 0;
};

} // namespace eh14
