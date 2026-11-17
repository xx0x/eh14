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

#include "hardware/AudioSource.hpp"
#include "sam.h"

namespace eh14
{

/**
 * @class AudioOut
 * @brief The MAX98357A amplifier (U5) on I2S, fed by DMA from a double buffer the main loop refills.
 * @details
 * I2S serializer 0 sends two 32-bit slots per frame in I2S format, clocked from Clocks::kAudioGenerator (48.005MHz /
 * 34 / 64 = 22061Hz frame rate, for sounds recorded at 22050Hz). Both slots carry the same sample; /SD_MODE is driven
 * high, which plays the left slot.
 *
 * DMA channel 0 moves one word per I2S request, through two descriptors that point at each other, one per buffer
 * half. When a half has been sent, the DMA interrupt counts it, and @ref Refill fills it again from the AudioSource
 * in the main loop. Each half has 23ms (@ref kFramesPerHalf frames) to be refilled; nothing in the main loop takes
 * that long during playback (a segment flip takes under 2ms, reading a half's samples from the flash 2ms).
 *
 * At the end of the sound, the last descriptor is shortened to the remaining frames plus one silent frame and ends
 * the chain: the DMA stops, and the I2S keeps repeating the silent frame. @ref Stop writes silence the same way.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class AudioOut
{
public:
    static constexpr std::size_t kFramesPerHalf = 512;

    bool Init();

    /**
     * @brief Plays @p source from the start, cutting off whatever was playing.
     */
    void Start(AudioSource &source);

    /**
     * @brief Refills the halves the DMA has sent. Call continuously during playback (Eh14::RunBackground).
     */
    void Refill();

    /**
     * @brief Stops at once, leaving silence.
     */
    void Stop();

    bool IsPlaying() const;

    /**
     * @brief Switches the amplifier on or off (SP_ENABLE, its /SD_MODE).
     */
    void SetAmplifier(bool on);

    /**
     * @brief The DMA interrupt's entry point.
     */
    void HandleDmaInterrupt();

    /**
     * @brief How many times a half was replayed before being refilled (an audible glitch each). Should stay 0.
     */
    static inline volatile std::uint32_t debug_underruns_ = 0;

private:
    static constexpr std::uint8_t kChannel = 0;
    static constexpr std::size_t kWordsPerHalf = kFramesPerHalf * 2;

    void FillHalf(std::size_t half);
    void SetDescriptor(std::size_t half, std::size_t words, bool last);
    static void WriteSilence();
    static void SelectChannel();

    // The DMAC's descriptor table (channel 0 only) and its write-back area, and the second half's descriptor.
    alignas(16) static inline DmacDescriptor descriptor_table_[1] = {};
    alignas(16) static inline DmacDescriptor writeback_[1] = {};
    alignas(16) static inline DmacDescriptor second_descriptor_ = {};

    alignas(4) static inline std::array<std::array<std::int32_t, kWordsPerHalf>, 2> buffer_ = {};

    AudioSource *source_ = nullptr;
    std::size_t next_half_ = 0;
    bool source_ended_ = true;
    volatile std::uint32_t free_halves_ = 0;
};

} // namespace eh14
