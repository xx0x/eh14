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

#include "hardware/AnalogInputs.hpp"

#include "Eh14Params.hpp"
#include "hardware/Board.hpp"
#include "hardware/Clocks.hpp"
#include "hardware/Gpio.hpp"
#include "sam.h"

namespace eh14
{

bool AnalogInputs::Init()
{
    PM->APBCMASK.reg |= PM_APBCMASK_ADC;
    Clocks::Connect(GCLK_CLKCTRL_ID_ADC_Val, Clocks::kMainGenerator);

    ADC->CTRLA.reg = ADC_CTRLA_SWRST;
    WaitSync();

    const std::uint32_t bias =
        (*reinterpret_cast<const std::uint32_t *>(ADC_FUSES_BIASCAL_ADDR) & ADC_FUSES_BIASCAL_Msk) >> ADC_FUSES_BIASCAL_Pos;
    std::uint32_t linearity = (*reinterpret_cast<const std::uint32_t *>(ADC_FUSES_LINEARITY_0_ADDR) &
                               ADC_FUSES_LINEARITY_0_Msk) >>
                              ADC_FUSES_LINEARITY_0_Pos;
    linearity |= ((*reinterpret_cast<const std::uint32_t *>(ADC_FUSES_LINEARITY_1_ADDR) & ADC_FUSES_LINEARITY_1_Msk) >>
                  ADC_FUSES_LINEARITY_1_Pos)
                 << 5;
    ADC->CALIB.reg = static_cast<std::uint16_t>(ADC_CALIB_BIAS_CAL(bias) | ADC_CALIB_LINEARITY_CAL(linearity));

    ADC->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC1;
    ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1;
    ADC->SAMPCTRL.reg = ADC_SAMPCTRL_SAMPLEN(kSampleTime);
    ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV512 | ADC_CTRLB_RESSEL_12BIT;
    WaitSync();
    ADC->INPUTCTRL.reg = ADC_INPUTCTRL_GAIN_DIV2 | ADC_INPUTCTRL_MUXNEG_GND | ADC_INPUTCTRL_MUXPOS(0);
    WaitSync();

    Gpio::SetFunction(Board::kLightSensor, Gpio::Function::B);
    Gpio::SetFunction(Board::kBatterySensor, Gpio::Function::B);
    return true;
}

std::uint16_t AnalogInputs::Read(std::uint8_t channel)
{
    ADC->INPUTCTRL.bit.MUXPOS = channel;
    WaitSync();
    ADC->CTRLA.bit.ENABLE = 1;
    WaitSync();

    std::uint16_t value = 0;
    for (int conversion = 0; conversion < 2; conversion++)
    {
        ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;
        ADC->SWTRIG.bit.START = 1;
        WaitSync();
        while (!ADC->INTFLAG.bit.RESRDY)
        {
        }
        value = static_cast<std::uint16_t>(ADC->RESULT.reg);
        WaitSync();
    }

    ADC->CTRLA.bit.ENABLE = 0;
    WaitSync();
    return value;
}

std::uint16_t AnalogInputs::ReadLight()
{
    return Read(Board::kLightSensorChannel);
}

std::uint8_t AnalogInputs::ReadBatteryPercent()
{
    std::uint32_t raw = Read(Board::kBatterySensorChannel);
    if (raw < params::kBatteryRawEmpty)
    {
        raw = params::kBatteryRawEmpty;
    }
    if (raw > params::kBatteryRawFull)
    {
        raw = params::kBatteryRawFull;
    }
    return static_cast<std::uint8_t>(((raw - params::kBatteryRawEmpty) * 99U) /
                                     (params::kBatteryRawFull - params::kBatteryRawEmpty));
}

void AnalogInputs::WaitSync()
{
    while (ADC->STATUS.bit.SYNCBUSY)
    {
    }
}

} // namespace eh14
