
#include "display.h"

void setup()
{
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
}
