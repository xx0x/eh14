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

namespace eh14
{

/**
 * @class ExternalInterrupts
 * @brief The EIC: edges on the button, RTC alarm and IR receiver pins, passed to the handler attached to each line.
 * @details The EIC runs from the crystal's 32.768kHz generator, which keeps running in standby, so it sees edges
 *          during sleep. Lines marked as wake-up sources wake the clock. No line is filtered: the buttons are
 *          debounced by time (Buttons), and the IR receiver's edges are clean. Attach every line before @ref Start -
 *          the EIC accepts configuration only while disabled.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class ExternalInterrupts
{
public:
    enum class Edge : std::uint8_t
    {
        RISING = 1,
        FALLING = 2,
    };

    /**
     * @brief Called on a line's edge, in interrupt context.
     */
    using Handler = void (*)(void *context);

    /**
     * @brief Clocks the EIC. Call before attaching lines.
     */
    bool Init();

    /**
     * @brief Calls @p handler with @p context on every @p edge on EIC line @p line. With @p wakes, the edge also
     *        wakes the clock from standby.
     */
    void Attach(std::uint8_t line, Edge edge, bool wakes, Handler handler, void *context);

    /**
     * @brief Enables the EIC and its interrupt, clearing any flags raised so far.
     */
    void Start();

    /**
     * @brief Whether a wake-up line has seen its edge since @ref ClearWakeEvent; this ends a sleep (Sleep).
     */
    static bool HasWakeEvent()
    {
        return wake_event_;
    }

    static void ClearWakeEvent()
    {
        wake_event_ = false;
    }

    /**
     * @brief The EIC interrupt's entry point.
     */
    static void HandleInterrupt();

private:
    static constexpr std::uint8_t kLineCount = 16;

    struct Slot
    {
        Handler handler;
        void *context;
    };

    static void WaitSync();

    static inline std::array<Slot, kLineCount> slots_ = {};
    static inline volatile bool wake_event_ = false;
};

} // namespace eh14
