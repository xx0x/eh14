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

#include "app/Voice.hpp"

#include "Eh14.hpp"
#include "hardware/Timebase.hpp"

namespace eh14
{

bool Voice::Say(std::uint8_t id)
{
    const std::array<std::uint8_t, 1> sentence = {id};
    return Play(sentence);
}

bool Voice::SaySentence(std::span<const std::uint8_t> ids)
{
    return Play(ids);
}

bool Voice::SayTime(std::uint8_t hour, std::uint8_t minute, std::uint8_t lead, bool outro)
{
    Sentence sentence = {};
    std::size_t count = 0;
    if (lead != kNoSound)
    {
        sentence[count++] = lead;
    }
    AddNumber(sentence, count, hour);
    sentence[count++] = WordFor(hour, SoundBank::kHours, SoundBank::kHours1, SoundBank::kHours2To4);
    AddNumber(sentence, count, minute);
    sentence[count++] = WordFor(minute, SoundBank::kMinutes, SoundBank::kMinutes1, SoundBank::kMinutes2To4);
    if (outro)
    {
        sentence[count++] = SoundBank::kOutro;
    }
    return Play(std::span<const std::uint8_t>(sentence.data(), count));
}

void Voice::AddNumber(Sentence &sentence, std::size_t &count, std::uint8_t number)
{
    // Numbers up to 20 have their own sound; above, the tens, then the ones unless zero.
    if (number < 21U)
    {
        sentence[count++] = number;
        return;
    }
    sentence[count++] = static_cast<std::uint8_t>((number / 10U) * 10U);
    if ((number % 10U) != 0U)
    {
        sentence[count++] = static_cast<std::uint8_t>(number % 10U);
    }
}

std::uint8_t Voice::WordFor(std::uint8_t number, std::uint8_t plain, std::uint8_t one, std::uint8_t two_to_four)
{
    const std::uint8_t last = number % 10U;
    const bool teen = number >= 11U && number <= 14U;
    if (last == 1U && !teen)
    {
        return one;
    }
    if (last >= 2U && last <= 4U && !teen)
    {
        return two_to_four;
    }
    return plain;
}

bool Voice::Play(std::span<const std::uint8_t> ids)
{
    // Queue up to the first sound the bank lacks.
    sentence_length_ = 0;
    bool complete = true;
    for (const std::uint8_t id : ids)
    {
        if (!Eh14::app.GetSoundBank().Has(id) || sentence_length_ >= kMaxSentence)
        {
            complete = false;
            break;
        }
        sentence_[sentence_length_++] = id;
    }
    if (sentence_length_ == 0U)
    {
        return false;
    }

    for (std::size_t i = 0; i < sentence_length_; i++)
    {
        debug_sentence_[i] = sentence_[i];
    }
    debug_sentence_length_ = sentence_length_;
    debug_frames_ = 0;
    const std::uint32_t start_ms = Timebase::GetMillis();

    Eh14::buttons.ClearStopRequest();
    StartSound(0);
    Eh14::audio.Start(*this);
    while (Eh14::audio.IsPlaying())
    {
        if (Eh14::buttons.IsStopRequested())
        {
            Eh14::audio.Stop();
            debug_play_ms_ = Timebase::GetMillis() - start_ms;
            return false;
        }
        Eh14::RunBackground();
    }
    debug_play_ms_ = Timebase::GetMillis() - start_ms;
    return complete && !Eh14::buttons.IsStopRequested();
}

void Voice::StartSound(std::size_t index)
{
    current_ = index;
    if (index >= sentence_length_)
    {
        samples_left_ = 0;
        return;
    }
    const SoundBank::Sound sound = Eh14::app.GetSoundBank().Get(sentence_[index]);
    address_ = sound.offset;
    samples_left_ = sound.length / 2U;
}

std::size_t Voice::Fill(std::span<std::int32_t> frames)
{
    const std::size_t wanted = frames.size() / 2U;
    std::size_t written = 0;
    while (written < wanted && current_ < sentence_length_)
    {
        if (samples_left_ == 0U)
        {
            StartSound(current_ + 1U);
            continue;
        }
        std::size_t samples = wanted - written;
        if (samples > samples_left_)
        {
            samples = samples_left_;
        }
        if (samples > kChunkBytes / 2U)
        {
            samples = kChunkBytes / 2U;
        }

        Eh14::flash.Read(address_, std::span<std::uint8_t>(chunk_.data(), samples * 2U));
        for (std::size_t i = 0; i < samples; i++)
        {
            const auto sample = static_cast<std::int16_t>(chunk_[2U * i] | (chunk_[(2U * i) + 1U] << 8));
            const std::int32_t value = sample * volume_;
            frames[2U * written] = value;
            frames[(2U * written) + 1U] = value;
            written++;
        }
        debug_frames_ = debug_frames_ + static_cast<std::uint32_t>(samples);
        address_ += static_cast<std::uint32_t>(samples * 2U);
        samples_left_ -= static_cast<std::uint32_t>(samples);
    }
    return written;
}

} // namespace eh14
