#include "pins.h"
#include "Adafruit_ZeroI2S.h"
#include <SPIMemory.h> // modified
#include "RTClib.h"    // modified

// Include parts
#include "./globals.h"
#include "./display.h"
#include "./clock.h"
#include "./digits.h"
#include "./flash.h"
#include "./say.h"
#include "./sleep.h"

void setup()
{
    analogReadResolution(12);
    pinMode(PIN_SNOOZE_BUTTON, INPUT_PULLUP);
    pinMode(PIN_MENU_BUTTON, INPUT_PULLUP);
    pinMode(PIN_CHANGE_BUTTON, INPUT_PULLUP);
    pinMode(PIN_RC_SWITCH, INPUT_PULLUP);

    Serial.begin(115200);
    Serial.println("EH14 startup\n");

    displaySetup();
    clockSetup();
    sleepSetup(callbackSnoozeButton, callbackMenuButton, callbackChangeButton, callbackAlarm);
    flash.begin();
    while (!flashSetup())
    {
        displayEmpty();
        delay(1000);
        serialLoop();
        delay(1000);
    }

    saySetup();
    displayClockReady();
    Serial.println("EH14 ready\n");
}

void loop()
{
    serialLoop();
    alarmLoop();
    if (!alarmTriggered)
    {
        menuLoop();
        if (!IS_MENU_ACTIVE)
        {
            timeLoop();
        }
        /*
        if (goToSleep)
        {
            goToSleep = false;
            turnOff();
            turnOn();
        }
        */
    }
}

void callbackButton(byte buttonNumber)
{
    if (millis() - lastTimeButton[buttonNumber] < DEBOUNCE_TIME)
    {
        return;
    }
    lastTimeButton[buttonNumber] = millis();
    stopPlaying = true;
    if (!alarmTriggered)
    {
        buttonPressed[buttonNumber] = true;
    }
}

void callbackSnoozeButton()
{
    callbackButton(SNOOZE_BUTTON);
}

void callbackMenuButton()
{
    callbackButton(MENU_BUTTON);
}

void callbackChangeButton()
{
    callbackButton(CHANGE_BUTTON);
}

void callbackAlarm()
{
    alarmTriggered = true;
    rtc.clearAlarm(1);
}

void alarmLoop()
{
    if (alarmTriggered)
    {
        DateTime alarm = rtc.getAlarmDateTime(1);
        for (byte i = 0; i < ALARM_MAX_LOOPS; i++)
        {
            delay(20);
            displayTime(alarm.hour(), alarm.minute());
            saySample(SAMPLE_ALARM_BASE + currentAlarm);
            if (stopPlaying)
            {
                break;
            }
            delay(500);
        }
        alarmTriggered = false;
        //         goToSleep = true;
        //        menuExit(); //  if alarm triggered in menu, close it
        delay(1000);
    }
}

void timeLoop()
{
    DateTime now = rtc.now();
    displayTime(now.hour(), now.minute());
    if (SNOOZE_BUTTON_PRESSED)
    {
        SNOOZE_BUTTON_PRESSED = false;
        sayTime(now.hour(), now.minute(), 0);
    }
    smartDelay(1000);
}

void menuLoop()
{

    if (MENU_BUTTON_PRESSED)
    {
        MENU_BUTTON_PRESSED = false;
        if (!TIME_SET_DIGITS_ACTIVE)
        {
            menuSoundHasPlayed = false;
            currentMenuItem++;
        }
        else
        {
            digitsNext();
            if (timeSetCurrentDigit >= 4)
            {
                if (currentMenuItem == MENU_ALARM_SET)
                {
                    digitsSaveAsAlarm();
                    DateTime alarm = rtc.getAlarmDateTime(1);
                    displayTime(alarm.hour(), alarm.minute());
                    if (saySample(SAMPLE_INTRO))
                    {
                        if (saySample(SAMPLE_ALARM_SET))
                        {
                            delay(300);
                            if (saySample(SAMPLE_ALARM_CURRENT))
                            {
                                sayTime(alarm.hour(), alarm.minute(), 00, false, true);
                            }
                        }
                    }
                }
                else if (currentMenuItem == MENU_TIME_SET)
                {
                    digitsSaveAsClock();
                    DateTime now = rtc.now();
                    displayTime(now.hour(), now.minute());
                    if (saySample(SAMPLE_INTRO))
                    {
                        if (saySample(SAMPLE_TIME_SET))
                        {
                            delay(300);
                            if (saySample(SAMPLE_TIME_CURRENT))
                            {
                                sayTime(now.hour(), now.minute(), now.second(), false, true);
                            }
                        }
                    }
                }
                menuExit();
                return;
            }
        }
        goToSleep = false;
        if (currentMenuItem >= MENU_ITEMS_COUNT)
        {
            currentMenuItem = 0;
        }
    }

    if (!IS_MENU_ACTIVE)
    {
        return;
    }

    if ((millis() - lastTimeButton[MENU_BUTTON] > MENU_TIMEOUT && millis() - lastTimeButton[CHANGE_BUTTON] > MENU_TIMEOUT) || SNOOZE_BUTTON_PRESSED)
    {
        delay(200);
        menuExit();
        return;
    }

    byte helpVal = 0;
    bool alarmEnabled;

    switch (currentMenuItem)
    {
    case MENU_VOLUME: // A
        if (CHANGE_BUTTON_PRESSED)
        {
            CHANGE_BUTTON_PRESSED = false;
            if (menuSoundHasPlayed)
            {
                currentVolume++;
            }
            menuSoundHasPlayed = true;
            if (currentVolume >= VOLUMES_COUNT)
            {
                currentVolume = 0;
            }
            helpVal = currentVolume + 1;
            displayWriteNumbers(LETTER_A, LETTER_NONE, helpVal / 10, helpVal % 10);
            saySample(SAMPLE_WARNING);
        }
        else
        {
            helpVal = currentVolume + 1;
            displayWriteNumbers(LETTER_A, LETTER_NONE, helpVal / 10, helpVal % 10);
        }
        break;
    case MENU_ALARM_SOUND: // B
        if (CHANGE_BUTTON_PRESSED)
        {
            CHANGE_BUTTON_PRESSED = false;
            if (menuSoundHasPlayed)
            {
                currentAlarm++;
            }
            menuSoundHasPlayed = true;
            if (currentAlarm >= alarmsCount)
            {
                currentAlarm = 0;
            }
            helpVal = currentAlarm + 1;
            displayWriteNumbers(LETTER_B, LETTER_NONE, helpVal / 10, helpVal % 10);
            saySample(currentAlarm + SAMPLE_ALARM_BASE);
        }
        else
        {
            helpVal = currentAlarm + 1;
            displayWriteNumbers(LETTER_B, LETTER_NONE, helpVal / 10, helpVal % 10);
        }
        break;
    case MENU_ALARM_ON: // C
        alarmEnabled = clockIsAlarmEnabled();
        if (CHANGE_BUTTON_PRESSED)
        {
            CHANGE_BUTTON_PRESSED = false;
            alarmEnabled = !alarmEnabled;
            if (alarmEnabled)
            {
                clockEnableAlarm();
            }
            else
            {
                clockDisableAlarm();
            }
        }
        if (alarmEnabled)
        {
            displayWriteNumbers(LETTER_C, LETTER_NONE, LETTER_O, LETTER_N);
        }
        else
        {
            displayWriteNumbers(LETTER_C, LETTER_NONE, LETTER_O, LETTER_F);
        }
        break;

    case MENU_ALARM_SET: // D
    case MENU_TIME_SET:  // E
        // SET ALARM / TIME
        if (CHANGE_BUTTON_PRESSED && !TIME_SET_DIGITS_ACTIVE)
        {
            CHANGE_BUTTON_PRESSED = false;
            timeSetCurrentDigit = 0;
            displayClear();
            digitsPrepare(currentMenuItem == MENU_ALARM_SET);
        }
        else
        {
            if (!TIME_SET_DIGITS_ACTIVE)
            {
                if (currentMenuItem == MENU_ALARM_SET)
                {
                    displayWriteNumbers(LETTER_D, LETTER_NONE, LETTER_NONE, LETTER_NONE);
                }
                else
                {
                    displayWriteNumbers(LETTER_E, LETTER_NONE, LETTER_NONE, LETTER_NONE);
                }
            }
            else if (timeSetCurrentDigit >= 0 && timeSetCurrentDigit <= 3)
            {
                if (CHANGE_BUTTON_PRESSED)
                {
                    CHANGE_BUTTON_PRESSED = false;
                    digitsIncrease();
                }

                for (byte i = 0; i <= timeSetCurrentDigit; i++)
                {
                    displayWriteNumber(i, timeSetDigits[i]);
                }
            }
        }

        break;
    case MENU_BATTERY: // F
        displayWriteNumbers(LETTER_F, LETTER_NONE, LETTER_NONE, LETTER_NONE);
        break;
    default:
        break;
    }
    smartDelay(100);
}

void menuExit()
{
    MENU_BUTTON_PRESSED = false;
    CHANGE_BUTTON_PRESSED = false;
    SNOOZE_BUTTON_PRESSED = false;
    //    goToSleep = true;
    currentMenuItem = -1;
    timeSetCurrentDigit = -1;
    displayClear();
}

void serialLoop()
{
    while (Serial.available() > 0)
    {
        byte inByte = Serial.read();
        switch (serialMode)
        {
        case SERIAL_MODE_FLASH:
            flashProcessByte(inByte);
            break;
        default:
            serialMode = inByte;
            if (serialMode == SERIAL_MODE_FLASH)
            {
                flashStart();
            }
        }
    }
    if (serialMode == SERIAL_MODE_FLASH)
    {
        flashEnd();
    }
    serialMode = SERIAL_MODE_NONE;
}