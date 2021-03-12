
#include "display.h"

#define PIN_SNOOZE_BUTTON 14
#define PIN_MENU_BUTTON 15
#define PIN_CHANGE_BUTTON 16
#define PIN_RC_SWITCH 17

#define IS_SNOOZE_BUTTON_PRESSED (digitalRead(PIN_SNOOZE_BUTTON) == LOW)
#define IS_MENU_BUTTON_PRESSED (digitalRead(PIN_MENU_BUTTON) == LOW)
#define IS_CHANGE_BUTTON_PRESSED (digitalRead(PIN_CHANGE_BUTTON) == LOW)
#define IS_RC_SWITCH_ON (digitalRead(PIN_RC_SWITCH) == LOW)

void setup()
{
    pinMode(PIN_SNOOZE_BUTTON, INPUT_PULLUP);
    pinMode(PIN_MENU_BUTTON, INPUT_PULLUP);
    pinMode(PIN_CHANGE_BUTTON, INPUT_PULLUP);
    pinMode(PIN_RC_SWITCH, INPUT_PULLUP);

    displaySetup();

    for (byte i = 0; i < 10; i++)
    {
        displaySetLed(1);
        delay(20);
        displaySetLed(0);
        delay(80);
    }
}

void loop()
{
    /*
    displayTime(17, 45);
    delay(2000);
    displayTime(8, 21);
    delay(2000);
    displayTime(8, 22);
    delay(2000);
    displayTime(11, 1);
    delay(2000);
    displayTime(4, 20);
    delay(2000);
    displayDisable();
    delay(5000);
    displayEnable();
    delay(1000);
    */
}
