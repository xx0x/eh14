#include "pins.h"
#include "Adafruit_ZeroI2S.h"
#include <SPIMemory.h> // modified
#include "RTClib.h"    // modified
#include <Adafruit_SleepyDog.h>

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

    pinMode(PIN_IR_LATCH, INPUT_PULLUP);
    pinMode(PIN_IR_DATA, INPUT_PULLUP);
    pinMode(PIN_IR_CLOCK, INPUT_PULLUP);

    pinMode(PIN_LIGHT_SENSOR, INPUT);

    displaySetup();

    Serial.begin(115200);
    Serial.println("EH14 startup\n");

    clockSetup();

    // IR interrupts
    attachInterrupt(digitalPinToInterrupt(PIN_IR_LATCH), callbackIrLatch, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_IR_CLOCK), callbackIrClock, RISING);

    // button interrupts
    attachInterrupt(digitalPinToInterrupt(PIN_SNOOZE_BUTTON), callbackSnoozeButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_MENU_BUTTON), callbackMenuButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_CHANGE_BUTTON), callbackChangeButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_ALARM), callbackAlarm, FALLING);

    sleepSetup();
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
    silentLoop();

    if (!alarmTriggered)
    {
        menuLoop();
        randomSoundLoop();
        if (!IS_MENU_ACTIVE)
        {
            timeLoop();
        }

        if (goToSleep && !silentModeHighPower)
        {
            goToSleep = false;
            if (turnOff())
            {
                turnOn();
            }
        }
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

void callbackIrLatch()
{
    stopPlaying = true;
    irRecieved = 0;
    irRecievedCount = 0;
    displaySetLed(true);
}

void callbackIrClock()
{
    irRecieved = bitWrite(irRecieved, 7 - irRecievedCount, digitalRead(PIN_IR_DATA));
    irRecievedCount++;
    if (irRecievedCount == 8)
    {
        switch (irRecieved)
        {
        case MESSAGE_SNOOZE:
            callbackButton(SNOOZE_BUTTON);
            break;
        case MESSAGE_MENU:
            callbackButton(MENU_BUTTON);
            break;
        case MESSAGE_CHANGE:
            callbackButton(CHANGE_BUTTON);
            break;
        case MESSAGE_SOUND:
            callbackButton(SOUND_BUTTON);
            break;
        }
        displaySetLed(false);
    }
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
        goToSleep = true;
        menuExit(); //  if alarm triggered in menu, close it
        delay(1000);
    }
}

void timeLoop()
{
    // ignore if silent mode
    if (SILENT_MODE_ENABLED && isSilent && !SNOOZE_BUTTON_PRESSED && !exitedFromMenu)
    {
        goToSleep = true;
        return;
    }

    // don't update time more often than 500ms
    if ((millis() - lastClockCheck < CLOCK_CHECK_INTERVAL) && !SNOOZE_BUTTON_PRESSED && !exitedFromMenu)
    {
        return;
    }

    exitedFromMenu = false;
    lastClockCheck = millis();

    DateTime now = rtc.now();
    displayTime(now.hour(), now.minute());
    if (SNOOZE_BUTTON_PRESSED)
    {
        SNOOZE_BUTTON_PRESSED = false;
        sayTime(now.hour(), now.minute(), 0);
    }
    if (!ANY_BUTTON_PRESSED)
    {
        goToSleep = true;
    }
}

void menuLoop()
{

    if (MENU_BUTTON_PRESSED)
    {
        MENU_BUTTON_PRESSED = false;
        CHANGE_BUTTON_PRESSED = false;
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
    case MENU_SILENT_MODE: // G
        if (CHANGE_BUTTON_PRESSED)
        {
            CHANGE_BUTTON_PRESSED = false;
            silentThreshhold++;
            if (silentThreshhold >= SILENT_MODE_THRESHOLDS_COUNT)
            {
                if (silentModeHighPower)
                {
                    silentModeHighPower = false;
                    silentThreshhold = -1;
                }
                else
                {
                    silentModeHighPower = true;
                    silentThreshhold = 0;
                }
            }
        }
        if (!SILENT_MODE_ENABLED)
        {
            displayWriteNumbers(LETTER_G, LETTER_NONE, LETTER_O, LETTER_F);
        }
        else
        {
            displayWriteNumbers(LETTER_G, LETTER_NONE, silentModeHighPower ? LETTER_H : LETTER_L, silentThreshhold + 1);
        }
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
    currentMenuItem = -1;
    timeSetCurrentDigit = -1;
    displayClear();
    exitedFromMenu = true;
    goToSleep = true;
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

void silentLoop()
{
    if (!SILENT_MODE_ENABLED)
    {
        return;
    }

    float lightValue;
    for (byte i = 0; i < SILENT_MODE_MEASURES; i++)
    {
        lightValue += ((float)analogRead(PIN_LIGHT_SENSOR)) / SILENT_MODE_MEASURES;
        smartDelay(50);
    }
    bool currentIsSilent = lightValue <= SILENT_MODE_THRESHOLD;
    if (currentIsSilent != isSilent)
    {
        isSilent = currentIsSilent;
        if (isSilent)
        {
            Serial.println("Silent mode activated.");
            if (currentMenuItem == MENU_SILENT_MODE)
            {
                sayDong();
            }
        }
        else
        {
            if (currentMenuItem == MENU_SILENT_MODE)
            {
                sayDing();
            }
            Serial.println("Silent mode deactivated.");
        }
    }
}

void randomSoundLoop()
{
    if (buttonPressed[SOUND_BUTTON])
    {
        buttonPressed[SOUND_BUTTON] = false;
        byte soundNumber = random(SAMPLE_ALARM_BASE, SAMPLE_ALARM_BASE + alarmsCount - 1);
        saySample(soundNumber);
    }
}

/**
 * SMART SLEEPING
 * Retrieves seconds from RTC.
 * If time between 0 - 55, sleep for that time (if greater than 2s).
 */
bool turnOff()
{
    DateTime now = rtc.now();
    int sleepFor = 55 - now.second();
    if (sleepFor > 2)
    {
        flash.powerDown();
        digitalWrite(PIN_SPEAKER_ENABLE, LOW);
        digitalWrite(PIN_DISPLAY_ENABLE, LOW);
        delay(100);
        sleepStart(sleepFor * 1000);
        return true;
    }
    return false;
}

bool turnOn()
{
    digitalWrite(PIN_SPEAKER_ENABLE, HIGH);
    digitalWrite(PIN_DISPLAY_ENABLE, HIGH);
    flash.powerUp();
    delay(100);
    return true;
}
