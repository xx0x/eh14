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
 * @class SegmentDriver
 * @brief Flips one segment of the electromechanical display with a coil pulse.
 * @details
 * Each digit has 7 coils between 8 half bridges (a P and an N MOSFET, to 19V and to ground): coil s sits between
 * bridges s and s+1, and the direction of the current sets the segment on or off. Two 74HC595s per digit drive the
 * bridges, two outputs per bridge (N gate, P gate). The four digits form one 64-bit chain, shifted out bit 63 first,
 * so digit 0 is closest to the MCU. Digit d is bits 16d..16d+15, its bridge k bits 2k and 2k+1.
 *
 * @warning Both outputs of a pair on shorts 19V to ground through the two MOSFETs and destroys them. Every frame goes
 *          through @ref IsSafe, and all segment frames are also checked at compile time.
 *
 * @ref Pulse latches the segment's frame, shifts zeros in while the coil is on, and latches them
 * params::kSegmentPulseUs after the start.
 *
 * The 595s power up with random content: Eh14 calls @ref Init first thing after reset, which clears them with the
 * boost converter off. The lines run over the 16-pin ribbon to all eight chips, so they are clocked at about 250kHz.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class SegmentDriver
{
public:
    static constexpr std::uint8_t kDigits = 4;
    static constexpr std::uint8_t kSegments = 7;

    /**
     * @brief Bit patterns of a segment's two bridges, at bit 2s of its digit. On: bridge s low, s+1 high (bits 0 and
     *        3); off: the reverse (bits 1 and 2).
     */
    static constexpr std::uint64_t kOnPattern = 0b1001;
    static constexpr std::uint64_t kOffPattern = 0b0110;

    /**
     * @brief Whether @p frame is safe to latch: no pair with both outputs on, and at most one coil driven.
     */
    static constexpr bool IsSafe(std::uint64_t frame)
    {
        if ((frame & (frame >> 1) & 0x5555555555555555ULL) != 0U)
        {
            return false;
        }
        return __builtin_popcountll(frame) <= 2;
    }

    /**
     * @brief The frame that flips segment @p segment of digit @p digit on or off.
     */
    static constexpr std::uint64_t Frame(std::uint8_t digit, std::uint8_t segment, bool on)
    {
        return (on ? kOnPattern : kOffPattern) << ((16U * digit) + (2U * segment));
    }

    /**
     * @brief Sets up the pins (boost converter off; data, clock and latch low) and clears the registers. Safe before
     *        the clocks are set up.
     */
    static void Init();

    /**
     * @brief Shifts in and latches 64 zeros: every bridge off.
     */
    static void Clear();

    /**
     * @brief Drives a segment on or off for params::kSegmentPulseUs, then clears it. Busy-waits; the pulse is timed
     *        by Timebase, so interrupts cannot shorten it.
     * @return False, with nothing driven, for a segment that does not exist.
     */
    static bool Pulse(std::uint8_t digit, std::uint8_t segment, bool on);

    /**
     * @brief Whether every frame @ref Frame can make is safe; checked by a static_assert below the class.
     */
    static constexpr bool AllFramesSafe()
    {
        for (std::uint8_t digit = 0; digit < kDigits; digit++)
        {
            for (std::uint8_t segment = 0; segment < kSegments; segment++)
            {
                if (!IsSafe(Frame(digit, segment, true)) || !IsSafe(Frame(digit, segment, false)))
                {
                    return false;
                }
                // A segment's pattern must stay within its own digit's 16 bits.
                if ((Frame(digit, segment, true) | Frame(digit, segment, false)) >> (16U * digit) > 0xFFFFU)
                {
                    return false;
                }
            }
        }
        return true;
    }

private:
    static void ShiftOut(std::uint64_t frame);
    static void BitDelay();
};

static_assert(SegmentDriver::IsSafe(0), "an empty frame must be safe");
static_assert(SegmentDriver::AllFramesSafe(), "a segment pattern would short a half bridge");

} // namespace eh14
