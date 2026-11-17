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

#include "usb/UsbSerial.hpp"

#include <array>

#include "Eh14.hpp"
#include "hardware/Board.hpp"
#include "hardware/Clocks.hpp"
#include "hardware/Gpio.hpp"
#include "hardware/IrqPriority.hpp"
#include "hardware/Timebase.hpp"
#include "sam.h"
#include "tusb.h"

extern "C" void USB_Handler(void)
{
    tud_int_handler(BOARD_TUD_RHPORT);
}

extern "C" void tud_cdc_line_state_cb(uint8_t /*itf*/, bool dtr, bool /*rts*/)
{
    eh14::Eh14::usb.HandleLineState(dtr);
}

// TinyUSB needs a millisecond time base when built without an RTOS.
extern "C" std::uint32_t tusb_time_millis_api(void)
{
    return eh14::Timebase::GetMillis();
}

namespace eh14
{

bool UsbSerial::Init()
{
    PM->APBBMASK.reg |= PM_APBBMASK_USB;
    PM->AHBMASK.reg |= PM_AHBMASK_USB;
    Clocks::Connect(GCLK_CLKCTRL_ID_USB_Val, Clocks::kMainGenerator);
    Gpio::SetFunction(Board::kUsbDm, Gpio::Function::G);
    Gpio::SetFunction(Board::kUsbDp, Gpio::Function::G);

    NVIC_SetPriority(USB_IRQn, IrqPriority::kUsb);

    const tusb_rhport_init_t init = {TUSB_ROLE_DEVICE, TUSB_SPEED_FULL};
    return tusb_init(BOARD_TUD_RHPORT, &init);
}

void UsbSerial::Task()
{
    tud_task();
}

void UsbSerial::Disconnect()
{
    tud_disconnect();
}

void UsbSerial::Connect()
{
    tud_connect();
}

std::uint32_t UsbSerial::Available() const
{
    return tud_cdc_available();
}

std::uint32_t UsbSerial::Read(std::span<std::uint8_t> data)
{
    if (!tud_mounted())
    {
        return 0;
    }
    return tud_cdc_read(data.data(), static_cast<std::uint32_t>(data.size()));
}

void UsbSerial::Print(std::string_view text)
{
    if (!tud_mounted())
    {
        return;
    }
    const std::uint32_t start = Timebase::GetMillis();
    while (!text.empty())
    {
        const std::uint32_t written = tud_cdc_write(text.data(), text.size());
        text.remove_prefix(written);
        if (text.empty())
        {
            break;
        }
        // Buffer full: drop the rest if nobody is reading, else give the host a moment.
        tud_cdc_write_flush();
        if (!tud_cdc_connected() || (Timebase::GetMillis() - start) > kWriteTimeoutMs)
        {
            break;
        }
        tud_task();
    }
    tud_cdc_write_flush();
}

void UsbSerial::Print(std::uint32_t value)
{
    std::array<char, 10> digits = {};
    std::size_t count = 0;
    do
    {
        digits[digits.size() - 1U - count] = static_cast<char>('0' + (value % 10U));
        value /= 10U;
        count++;
    } while (value != 0U);
    Print(std::string_view(digits.data() + digits.size() - count, count));
}

void UsbSerial::PrintHex(std::uint32_t value)
{
    static constexpr std::string_view kHex = "0123456789ABCDEF";
    std::array<char, 8> digits = {};
    std::size_t count = 0;
    do
    {
        digits[digits.size() - 1U - count] = kHex[value & 0xFU];
        value >>= 4;
        count++;
    } while (value != 0U);
    Print(std::string_view(digits.data() + digits.size() - count, count));
}

void UsbSerial::Println(std::string_view text)
{
    Print(text);
    Print("\r\n");
}

void UsbSerial::Println(std::uint32_t value)
{
    Print(value);
    Print("\r\n");
}

bool UsbSerial::IsBootloaderRequested() const
{
    return bootloader_requested_ && (Timebase::GetMillis() - bootloader_request_ms_) >= kBootloaderDelayMs;
}

void UsbSerial::HandleLineState(bool dtr)
{
    cdc_line_coding_t coding = {};
    tud_cdc_get_line_coding(&coding);
    if (!dtr && coding.bit_rate == kBootloaderBaud)
    {
        bootloader_requested_ = true;
        bootloader_request_ms_ = Timebase::GetMillis();
    }
    else
    {
        bootloader_requested_ = false;
    }
}

} // namespace eh14
