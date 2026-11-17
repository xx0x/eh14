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

#include "hardware/SegmentDriver.hpp"

namespace eh14
{

/**
 * @brief What a digit can show. 0..9 are the digits, so Glyph(7) is a 7.
 */
enum class Glyph : std::uint8_t
{
    DIGIT_0 = 0,
    DIGIT_9 = 9,
    A = 10,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    UNDERLINE,
    DASH,
    NONE,
    COUNT,
};

/**
 * @class Display
 * @brief The four electromechanical seven-segment digits and their 19V boost converter.
 * @details
 * The segments hold their state without power. A flip is loud and slow (a coil pulse, then params::kSegmentGapMs), so
 * only the segments that differ are flipped, one at a time: digit by digit from the left, segments a to g - or all in
 * reverse for @ref ClearBackwards.
 *
 * @ref Show and the others only set the target; @ref Update, called from Eh14::RunBackground, flips the next segment
 * when its time has come. To wait until a change is fully shown, wait for @ref IsIdle (Eh14::WaitForDisplay).
 *
 * The shown state is only known from what was flipped, and is assumed blank at power-up. With force, every segment is
 * pulsed, changed or not.
 *
 * The boost converter (U9, MIC2288, enabled through Q1) makes the 19V from the battery. It is on while awake and off
 * during sleep.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Display
{
public:
    static constexpr std::uint8_t kDigits = SegmentDriver::kDigits;

    using Glyphs = std::array<Glyph, kDigits>;

    bool Init();

    void SetPower(bool on);

    void Show(const Glyphs &glyphs, bool force = false);

    void ShowDigit(std::uint8_t digit, Glyph glyph, bool force = false);

    /**
     * @brief Hours and minutes, with leading zeros: 07:05 as 0705.
     */
    void ShowTime(std::uint8_t hour, std::uint8_t minute, bool force = false);

    /**
     * @brief A number of up to four digits, with leading zeros.
     */
    void ShowNumber(std::uint16_t number);

    void Clear(bool force = false);

    /**
     * @brief Blanks the display from the last segment of the last digit back to the first one.
     */
    void ClearBackwards(bool force = false);

    /**
     * @brief Flips the next pending segment if the pause after the last flip is over; returns at once otherwise. A
     *        flip takes about 1.6ms.
     */
    void Update();

    /**
     * @brief Whether the target is shown and the pause after the last flip is over.
     */
    bool IsIdle() const;

    static constexpr Glyph Digit(std::uint8_t value)
    {
        return static_cast<Glyph>(value % 10U);
    }

private:
    enum class Order
    {
        FORWARD,
        BACKWARD,
    };

    /**
     * @brief The segments each glyph lights: a in bit 0, then clockwise, g in bit 6.
     */
    static constexpr std::array<std::uint8_t, static_cast<std::size_t>(Glyph::COUNT)> kGlyphSegments = {
        0b00111111, // 0
        0b00000110, // 1
        0b01011011, // 2
        0b01001111, // 3
        0b01100110, // 4
        0b01101101, // 5
        0b01111101, // 6
        0b00000111, // 7
        0b01111111, // 8
        0b01100111, // 9
        0b01110111, // A
        0b01111100, // b
        0b00111001, // C
        0b01011110, // d
        0b01111001, // E
        0b01110001, // F
        0b01101111, // g
        0b01110110, // H
        0b00000110, // I
        0b00011110, // J
        0b01110101, // k
        0b00111000, // L
        0b00010101, // M
        0b01010100, // n
        0b01011100, // o
        0b01110011, // P
        0b01100111, // q
        0b00110011, // R
        0b01101101, // S
        0b01111000, // t
        0b00111110, // U
        0b00011100, // v
        0b00011101, // w
        0b01110110, // X
        0b01110010, // Y
        0b01011011, // Z
        0b00001000, // _
        0b01000000, // -
        0b00000000, // blank
    };

    static constexpr std::uint8_t kAllSegments = 0x7F;

    static constexpr std::uint8_t SegmentsOf(Glyph glyph)
    {
        return kGlyphSegments[static_cast<std::size_t>(glyph) % kGlyphSegments.size()];
    }

    /**
     * @brief Finds the next segment to flip, in the current order. False if there is none.
     */
    bool FindNext(std::uint8_t &digit, std::uint8_t &segment) const;

    bool HasPending() const;

    std::array<std::uint8_t, kDigits> shown_ = {};
    std::array<std::uint8_t, kDigits> target_ = {};
    std::array<std::uint8_t, kDigits> forced_ = {};
    Order order_ = Order::FORWARD;
    std::uint32_t last_flip_ms_ = 0;
    bool flipped_ = false;
};

} // namespace eh14
