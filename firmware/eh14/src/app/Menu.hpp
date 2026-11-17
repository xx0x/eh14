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

#include "app/TimeEditor.hpp"
#include "hardware/Display.hpp"

namespace eh14
{

/**
 * @class Menu
 * @brief The menu: MENU opens it and steps through the items, CHANGE changes the item shown, SNOOZE or a minute
 *        without MENU or CHANGE closes it.
 * @details
 * Each item is shown by its letter in the first digit (@ref Item). In the volume and alarm sound items, the first
 * CHANGE plays the current value and each further one selects the next. The alarm and clock time items cycle through
 * hours, minutes and a blank until CHANGE starts editing (TimeEditor). After the last item, MENU blanks the display
 * and starts over.
 *
 * The status LED stays still while the silent mode item is shown, as it sits next to the light sensor
 * (StatusLed::SetInhibited).
 *
 * @ref Update is one pass, called from the main loop while the menu is open or MENU was pressed. It waits for the
 * display, sounds and pauses, calling Eh14::RunBackground meanwhile.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Menu
{
public:
    enum class Item : std::int8_t
    {
        NONE = -1,
        VOLUME,      ///< A: 12 volume steps
        ALARM_SOUND, ///< b: which alarm sound
        ALARM_ON,    ///< C: the alarm on or off
        ALARM_SET,   ///< d: the alarm time
        TIME_SET,    ///< E: the clock time
        SILENT,      ///< F: the silent mode - off, L1..L8, H1..H8
        ALARM_LOOPS, ///< g: how many times the alarm plays
        BATTERY,     ///< H: the battery's charge
        COUNT,
    };

    void Update();

    /**
     * @brief Closes the menu, optionally saving the settings, and blanks the display for the time to come back.
     */
    void Exit(bool save);

    bool IsActive() const
    {
        return item_ != Item::NONE;
    }

    Item GetItem() const
    {
        return item_;
    }

private:
    /**
     * @brief The battery item before its first measurement.
     */
    static constexpr std::uint8_t kNoBattery = 0xFF;

    void SetItem(Item item);
    void UpdateVolume();
    void UpdateAlarmSound();
    void UpdateAlarmOn();
    void UpdateTimeItem();
    void UpdateSilent();
    void UpdateAlarmLoops();
    void UpdateBattery();

    /**
     * @brief Shows the item's letter, a blank and two more glyphs, and waits for the display.
     */
    void ShowItem(Glyph letter, Glyph third, Glyph fourth);

    /**
     * @brief Shows the item's letter, a blank and @p value in two digits, and waits for the display.
     */
    void ShowItemValue(Glyph letter, std::uint8_t value);

    Item item_ = Item::NONE;
    bool sound_played_ = false;
    std::uint8_t battery_shown_ = kNoBattery;
    TimeEditor editor_;
};

} // namespace eh14
