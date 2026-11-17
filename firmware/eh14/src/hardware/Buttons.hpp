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

namespace eh14
{

enum class Button : std::uint8_t
{
    SNOOZE, ///< HLASENI: the time announcement, closes the menu
    MENU,   ///< NASTAVENI: opens the menu, the next item
    CHANGE, ///< ZMENA: changes the item; in time mode blanks the display
    SOUND,  ///< MELODIE: on the remote control only - a random alarm sound
    COUNT,
};

/**
 * @class Buttons
 * @brief The clock's three buttons and the remote control's four (through IrReceiver), as presses for the main loop.
 * @details A press is caught on its falling edge in the EIC interrupt and stays flagged until the main loop takes it,
 *          so presses are not lost while the clock is flipping digits or talking. The edge also wakes the clock.
 *
 * A second press of the same button within params::kDebounceMs is a bounce. Every valid press also requests a stop of
 * whatever is playing (@ref IsStopRequested) and records its time for the menu timeout. While the alarm rings
 * (@ref SetAlarmRinging), a press only stops it.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Buttons
{
public:
    bool Init();

    /**
     * @brief A press of @p button, from the EIC or from the IR receiver. Interrupt context.
     */
    void Press(Button button);

    bool IsPressed(Button button) const
    {
        return pressed_[Index(button)];
    }

    void Clear(Button button)
    {
        pressed_[Index(button)] = false;
    }

    /**
     * @brief Whether @p button was pressed, forgetting the press.
     */
    bool TakePress(Button button)
    {
        const bool pressed = pressed_[Index(button)];
        pressed_[Index(button)] = false;
        return pressed;
    }

    /**
     * @brief Whether any of the clock's three buttons has a press waiting (SOUND excluded).
     */
    bool IsAnyPressed() const
    {
        return IsPressed(Button::SNOOZE) || IsPressed(Button::MENU) || IsPressed(Button::CHANGE);
    }

    /**
     * @brief Whether any button, SOUND included, has a press waiting; cuts Eh14::SmartDelay short.
     */
    bool IsAnyPressedIncludingSound() const
    {
        return IsAnyPressed() || IsPressed(Button::SOUND);
    }

    std::uint32_t GetLastPressMs(Button button) const
    {
        return last_press_ms_[Index(button)];
    }

    /**
     * @brief Whether @p button is held down now, read from the pin. Always false for the remote's SOUND.
     */
    bool IsHeld(Button button) const;

    /**
     * @brief Asks whatever plays to stop. Every press does, and so does the remote control as it starts sending.
     */
    void RequestStop()
    {
        stop_requested_ = true;
    }

    bool IsStopRequested() const
    {
        return stop_requested_;
    }

    void ClearStopRequest()
    {
        stop_requested_ = false;
    }

    void SetAlarmRinging(bool ringing)
    {
        alarm_ringing_ = ringing;
    }

private:
    static constexpr std::size_t kCount = static_cast<std::size_t>(Button::COUNT);

    static constexpr std::size_t Index(Button button)
    {
        return static_cast<std::size_t>(button);
    }

    static void HandleSnoozeEdge(void *context);
    static void HandleMenuEdge(void *context);
    static void HandleChangeEdge(void *context);

    std::array<volatile bool, kCount> pressed_ = {};
    std::array<volatile std::uint32_t, kCount> last_press_ms_ = {};
    volatile bool stop_requested_ = false;
    volatile bool alarm_ringing_ = false;
};

} // namespace eh14
