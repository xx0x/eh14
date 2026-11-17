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

#include "hardware/AudioOut.hpp"

#include <span>

#include "Eh14.hpp"
#include "hardware/Board.hpp"
#include "hardware/Clocks.hpp"
#include "hardware/Gpio.hpp"
#include "hardware/IrqPriority.hpp"

extern "C" void DMAC_Handler(void)
{
    eh14::Eh14::audio.HandleDmaInterrupt();
}

namespace eh14
{

bool AudioOut::Init()
{
    Gpio::SetOutput(Board::kSpeakerEnable, false);

    // The I2S: two 32 bit slots, I2S framing, serializer 0 sending.
    PM->APBCMASK.reg |= PM_APBCMASK_I2S;
    Clocks::Connect(GCLK_CLKCTRL_ID_I2S_0_Val, Clocks::kAudioGenerator);
    I2S->CTRLA.reg = I2S_CTRLA_SWRST;
    while (I2S->SYNCBUSY.bit.SWRST)
    {
    }
    I2S->CLKCTRL[0].reg =
        I2S_CLKCTRL_SLOTSIZE_32 | I2S_CLKCTRL_NBSLOTS(1) | I2S_CLKCTRL_FSWIDTH_SLOT | I2S_CLKCTRL_BITDELAY_I2S;
    I2S->SERCTRL[0].reg = I2S_SERCTRL_SERMODE_TX | I2S_SERCTRL_DATASIZE_32 | I2S_SERCTRL_CLKSEL_CLK0;
    I2S->CTRLA.reg = I2S_CTRLA_ENABLE | I2S_CTRLA_CKEN0 | I2S_CTRLA_SEREN0;
    while (I2S->SYNCBUSY.reg != 0U)
    {
    }
    Gpio::SetFunction(Board::kI2sSd, Gpio::Function::G);
    Gpio::SetFunction(Board::kI2sSck, Gpio::Function::G);
    Gpio::SetFunction(Board::kI2sFs, Gpio::Function::G);

    // The DMA: one channel, one word per serializer request.
    PM->AHBMASK.reg |= PM_AHBMASK_DMAC;
    PM->APBBMASK.reg |= PM_APBBMASK_DMAC;
    DMAC->CTRL.reg = 0;
    DMAC->CTRL.reg = DMAC_CTRL_SWRST;
    while (DMAC->CTRL.bit.SWRST)
    {
    }
    DMAC->BASEADDR.reg = reinterpret_cast<std::uint32_t>(descriptor_table_);
    DMAC->WRBADDR.reg = reinterpret_cast<std::uint32_t>(writeback_);
    DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xF);

    SelectChannel();
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    while (DMAC->CHCTRLA.bit.SWRST)
    {
    }
    DMAC->CHCTRLB.reg =
        DMAC_CHCTRLB_LVL(0) | DMAC_CHCTRLB_TRIGSRC(I2S_DMAC_ID_TX_0) | DMAC_CHCTRLB_TRIGACT_BEAT;
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;

    NVIC_SetPriority(DMAC_IRQn, IrqPriority::kAudioDma);
    NVIC_EnableIRQ(DMAC_IRQn);

    WriteSilence();
    return true;
}

void AudioOut::Start(AudioSource &source)
{
    Stop();
    source_ = &source;
    source_ended_ = false;
    free_halves_ = 0;

    FillHalf(0);
    if (!source_ended_)
    {
        FillHalf(1);
    }
    next_half_ = 0;

    SelectChannel();
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;
}

void AudioOut::Refill()
{
    while (free_halves_ > 0U && !source_ended_ && source_ != nullptr)
    {
        if (free_halves_ > 1U)
        {
            // Both halves were sent before one was refilled: the DMA is replaying a stale half.
            debug_underruns_ = debug_underruns_ + 1U;
        }
        FillHalf(next_half_);
        next_half_ ^= 1U;
        __disable_irq();
        free_halves_ = free_halves_ - 1U;
        __enable_irq();
    }
}

void AudioOut::Stop()
{
    SelectChannel();
    DMAC->CHCTRLA.reg = 0;
    while (DMAC->CHCTRLA.bit.ENABLE)
    {
    }
    source_ = nullptr;
    source_ended_ = true;
    WriteSilence();
}

bool AudioOut::IsPlaying() const
{
    SelectChannel();
    return DMAC->CHCTRLA.bit.ENABLE;
}

void AudioOut::SetAmplifier(bool on)
{
    Gpio::Write(Board::kSpeakerEnable, on);
}

void AudioOut::HandleDmaInterrupt()
{
    SelectChannel();
    const std::uint8_t flags = DMAC->CHINTFLAG.reg;
    DMAC->CHINTFLAG.reg = flags;
    if ((flags & DMAC_CHINTFLAG_TCMPL) != 0U)
    {
        free_halves_ = free_halves_ + 1U;
    }
}

void AudioOut::FillHalf(std::size_t half)
{
    std::array<std::int32_t, kWordsPerHalf> &words = buffer_[half];
    const std::size_t frames = source_->Fill(std::span<std::int32_t>(words));
    if (frames < kFramesPerHalf)
    {
        // The end: the remaining frames plus one silent frame for the I2S to repeat after the DMA stops.
        words[frames * 2U] = 0;
        words[(frames * 2U) + 1U] = 0;
        SetDescriptor(half, (frames + 1U) * 2U, true);
        source_ended_ = true;
    }
    else
    {
        SetDescriptor(half, kWordsPerHalf, false);
    }
}

void AudioOut::SetDescriptor(std::size_t half, std::size_t words, bool last)
{
    DmacDescriptor &descriptor = (half == 0U) ? descriptor_table_[0] : second_descriptor_;
    DmacDescriptor &next = (half == 0U) ? second_descriptor_ : descriptor_table_[0];

    descriptor.BTCTRL.reg = DMAC_BTCTRL_VALID | DMAC_BTCTRL_BLOCKACT_INT | DMAC_BTCTRL_BEATSIZE_WORD |
                            DMAC_BTCTRL_SRCINC | DMAC_BTCTRL_STEPSIZE_X1;
    descriptor.BTCNT.reg = static_cast<std::uint16_t>(words);
    // With an incrementing source, SRCADDR is the end address.
    descriptor.SRCADDR.reg = reinterpret_cast<std::uint32_t>(buffer_[half].data() + words);
    descriptor.DSTADDR.reg = reinterpret_cast<std::uint32_t>(&I2S->DATA[0].reg);
    descriptor.DESCADDR.reg = last ? 0U : reinterpret_cast<std::uint32_t>(&next);
}

void AudioOut::WriteSilence()
{
    for (int slot = 0; slot < 2; slot++)
    {
        for (std::uint32_t wait = 0; wait < 100000U && (I2S->INTFLAG.reg & I2S_INTFLAG_TXRDY0) == 0U; wait++)
        {
        }
        I2S->DATA[0].reg = 0;
    }
}

void AudioOut::SelectChannel()
{
    DMAC->CHID.reg = DMAC_CHID_ID(kChannel);
}

} // namespace eh14
