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
#include <cstddef>
#include <cstdint>
#include <span>

#include "app/SoundBank.hpp"
#include "hardware/AudioSource.hpp"

namespace eh14
{

/**
 * @class Voice
 * @brief Plays the bank's sounds, singly or as a sentence, and says the time.
 * @details
 * A sentence (a time announcement: "presny cas, osmnact hodin, dvacet jedna minut, gong") is queued whole and
 * streamed from the flash without gaps between its sounds. Samples are scaled by the volume (params::kVolumes).
 *
 * Every play blocks until it ends, calling Eh14::RunBackground, or until a button requests a stop
 * (Buttons::IsStopRequested, cleared at the start). It returns false when stopped or when the bank lacks a sound; a
 * sentence then plays up to the missing sound.
 *
 * The Czech words for hours and minutes follow the number's last digit: 1 takes "hodina", 2 to 4 "hodiny", the rest
 * "hodin" - except 11 to 14, which take "hodin". So 21 is "dvacet jedna hodina".
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Voice : public AudioSource
{
public:
    /**
     * @brief No sound, where one is optional.
     */
    static constexpr std::uint8_t kNoSound = 0xFF;

    /**
     * @brief Sets what each sample is multiplied by (params::kVolumes).
     */
    void SetVolume(std::uint32_t volume)
    {
        volume_ = static_cast<std::int32_t>(volume);
    }

    /**
     * @brief Plays sound @p id.
     * @return False if the bank lacks it, or a button stopped it.
     */
    bool Say(std::uint8_t id);

    /**
     * @brief Plays the sounds @p ids one right after the other.
     * @return False if the bank lacks one of them (the ones before it are played), or a button stopped it.
     */
    bool SaySentence(std::span<const std::uint8_t> ids);

    /**
     * @brief Says the time, preceded by @p lead (the intro, or @ref kNoSound for none) and optionally followed by the
     *        gong.
     */
    bool SayTime(std::uint8_t hour, std::uint8_t minute, std::uint8_t lead = SoundBank::kIntro, bool outro = true);

    std::size_t Fill(std::span<std::int32_t> frames) override;

    /**
     * @brief The last sentence played, the frames sent to the DMA and the play time in ms - to check over SWD that
     *        every sound plays whole and at the right speed (CLAUDE.md).
     */
    static inline volatile std::uint8_t debug_sentence_[8] = {};
    static inline volatile std::uint32_t debug_sentence_length_ = 0;
    static inline volatile std::uint32_t debug_frames_ = 0;
    static inline volatile std::uint32_t debug_play_ms_ = 0;

private:
    static constexpr std::size_t kMaxSentence = 8;

    /**
     * @brief Bytes read from the flash at a time while filling a buffer.
     */
    static constexpr std::size_t kChunkBytes = 512;

    using Sentence = std::array<std::uint8_t, kMaxSentence>;

    /**
     * @brief Appends the words of @p number to @p sentence.
     */
    static void AddNumber(Sentence &sentence, std::size_t &count, std::uint8_t number);

    /**
     * @brief Which of the three word forms (@p plain, @p one, @p two_to_four) goes with @p number.
     */
    static std::uint8_t WordFor(std::uint8_t number, std::uint8_t plain, std::uint8_t one, std::uint8_t two_to_four);

    bool Play(std::span<const std::uint8_t> ids);

    void StartSound(std::size_t index);

    std::int32_t volume_ = 0;

    // The sentence being played, and where in it Fill is.
    Sentence sentence_ = {};
    std::size_t sentence_length_ = 0;
    std::size_t current_ = 0;
    std::uint32_t address_ = 0;
    std::uint32_t samples_left_ = 0;

    static inline std::array<std::uint8_t, kChunkBytes> chunk_ = {};
};

} // namespace eh14
