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
#include "./battery.h"
#include "./test.h"

void setup()
{
    analogReadResolution(12);

    // inputs and outputs (more specified in *.h files)
    pinMode(PIN_SNOOZE_BUTTON, INPUT_PULLUP);
    pinMode(PIN_MENU_BUTTON, INPUT_PULLUP);
    pinMode(PIN_CHANGE_BUTTON, INPUT_PULLUP);
    pinMode(PIN_IR_LATCH, INPUT_PULLUP);
    pinMode(PIN_IR_DATA, INPUT_PULLUP);
    pinMode(PIN_IR_CLOCK, INPUT_PULLUP);
    pinMode(PIN_LIGHT_SENSOR, INPUT);

    while (digitalRead(PIN_MENU_BUTTON) == LOW && digitalRead(PIN_CHANGE_BUTTON) == LOW)
    {
        testModeEnabled = true;
        displaySetLed(true);
        delay(50);
        displaySetLed(false);
        delay(50);
    }

    Serial.begin(115200);
    Serial.println("EH14 startup\n");

    // Starts clock (if battery disconnected in the meantime, reset to 00:00)
    clockSetup();

    // IR interrupts
    attachInterrupt(digitalPinToInterrupt(PIN_IR_LATCH), callbackIrLatch, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_IR_CLOCK), callbackIrClock, RISING);

    // Button interrupts
    attachInterrupt(digitalPinToInterrupt(PIN_SNOOZE_BUTTON), callbackSnoozeButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_MENU_BUTTON), callbackMenuButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_CHANGE_BUTTON), callbackChangeButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_ALARM), callbackAlarm, FALLING);

    // Prepares interrups for waking up
    sleepSetup();

    // Starts flash, calculates samples' offsets
    flash.begin();
    while (!flashSetup())
    {
        displayEmpty();
        delay(1000);
        serialLoop();
        delay(1000);
    }

    // Starts I2S AMP
    saySetup();

    // Load settings stored in flash (volumne, alarm number...)
    flashLoadSettings();

    // Clears the display
    displaySetup();

    if (currentIntroEnabled)
    {
        displayClear(true);
        delay(100);

        // shows "EH14"
        displayLogo();

        // Intro music - says first alarm sample
        saySample(SAMPLE_ALARM_BASE);
        delay(100);

        // Clears display
        displayClear();

        // Flashes status led
        displayClockReady();

        Serial.println("EH14 ready\n");

        // Displays and says current time
        DateTime now = rtc.now();
        displayTime(now.hour(), now.minute());
        sayTime(now.hour(), now.minute(), 0);

        currentIntroEnabled = false;
        flashSaveSettings();
    }
    else if (!testModeEnabled)
    {
        DateTime now = rtc.now();
        displayTime(now.hour(), now.minute(), true);
    }
}

// Main loop references partial loops
void loop()
{
    testLoop();
    serialLoop();
    alarmLoop();

    if (!alarmTriggered)
    {
        menuLoop();
    }

    silentLoop();

    if (!alarmTriggered)
    {
        randomSoundLoop();
        if (!IS_MENU_ACTIVE)
        {
            timeLoop();
        }

        if (goToSleep && !silentModeHighPower && !ANY_BUTTON_PRESSED && !irNowRecieving)
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
    if ((millis() - lastTimeButton[buttonNumber] < DEBOUNCE_TIME) && millis() >= lastTimeButton[buttonNumber])
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
    irNowRecieving = true;
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
    irNowRecieving = false;
}

void alarmLoop()
{
    if (alarmTriggered)
    {
        DateTime alarm = rtc.getAlarmDateTime(1);
        if (ALARM_CURRENT_LOOPS > 0)
        {
            for (byte i = 0; i < ALARM_CURRENT_LOOPS; i++)
            {
                delay(20);
                displayTime(alarm.hour(), alarm.minute());
                saySample(SAMPLE_ALARM_BASE + currentAlarm);
                if (stopPlaying)
                {
                    break;
                }
                delay(WAIT_BETWEEN_ALARMS);
            }
        }
        else
        {
            stopPlaying = false;
            while (!stopPlaying)
            {
                delay(20);
                displayTime(alarm.hour(), alarm.minute());
                saySample(SAMPLE_ALARM_BASE + currentAlarm);
                delay(WAIT_BETWEEN_ALARMS);
            }
        }
        alarmTriggered = false;
        goToSleep = true;
        menuExit(false); //  if alarm triggered in menu, close it
        delay(1000);
    }
}

void timeLoop()
{
    if (CHANGE_BUTTON_PRESSED)
    {
        CHANGE_BUTTON_PRESSED = false;
        currentIntroEnabled = true;
        flashSaveSettings();
        displayClearBackwards();
        smartDelay(CHANGE_CLICK_DISPLAY_CLEAR_TIMING);
        currentIntroEnabled = false;
        flashSaveSettings();
    }

    // ignore if silent mode
    if (SILENT_MODE_ENABLED && isSilent && !SNOOZE_BUTTON_PRESSED)
    {
        goToSleep = true;
        return;
    }

    // don't update time more often than 500ms
    if ((millis() - lastClockCheck < CLOCK_CHECK_INTERVAL) && !SNOOZE_BUTTON_PRESSED && !exitedFromMenu && millis() >= lastClockCheck)
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
        smartDelay(100);
        now = rtc.now();
        displayTime(now.hour(), now.minute());
        smartDelay(100);
        if (readBattery() < BATTERY_LOW)
        {
            displayBatteryLow();
        }
    }
    goToSleep = true;
}

void menuLoop()
{

    if (MENU_BUTTON_PRESSED)
    {
        MENU_BUTTON_PRESSED = false;
        CHANGE_BUTTON_PRESSED = false;
        prevBatteryMeasure = BATTERY_NO_MEASURE;
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
                menuExit(true);
                return;
            }
        }
        goToSleep = false;
        if (currentMenuItem >= MENU_ITEMS_COUNT)
        {
            currentMenuItem = 0;
            displayClear();
            smartDelay(100);
        }
    }

    if (!IS_MENU_ACTIVE)
    {
        return;
    }

    if ((millis() - lastTimeButton[MENU_BUTTON] > MENU_TIMEOUT && millis() - lastTimeButton[CHANGE_BUTTON] > MENU_TIMEOUT) || SNOOZE_BUTTON_PRESSED)
    {
        delay(200);
        menuExit(true);
        return;
    }

    byte helpVal = 0;
    bool alarmEnabled;
    DateTime retrievedTime;

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
            smartDelay(300);
            if (!ANY_BUTTON_PRESSED)
            {
                saySample(SAMPLE_WARNING);
            }
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
                    helpVal = LETTER_D;
                    retrievedTime = rtc.getAlarmDateTime(1);
                }
                else
                {
                    helpVal = LETTER_E;
                    retrievedTime = rtc.now();
                }
                displayWriteNumbers(helpVal, LETTER_NONE, retrievedTime.hour() / 10, retrievedTime.hour() % 10);
                smartDelay(SWAP_HOURS_MINUTES_IN_MENU_TIMING);
                if (!ANY_BUTTON_PRESSED)
                {
                    displayWriteNumbers(helpVal, LETTER_NONE, retrievedTime.minute() / 10, retrievedTime.minute() % 10);
                    smartDelay(SWAP_HOURS_MINUTES_IN_MENU_TIMING);
                }
                if (!ANY_BUTTON_PRESSED)
                {
                    displayWriteNumbers(helpVal, LETTER_NONE, LETTER_NONE, LETTER_NONE);
                    smartDelay(SWAP_HOURS_MINUTES_IN_MENU_TIMING);
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
    case MENU_SILENT_MODE: // F
        if (CHANGE_BUTTON_PRESSED)
        {
            CHANGE_BUTTON_PRESSED = false;
            silentThreshhold++;
            if (silentThreshhold > SILENT_MODE_THRESHOLDS_COUNT)
            {
                if (silentModeHighPower)
                {
                    silentModeHighPower = false;
                    silentThreshhold = 0;
                }
                else
                {
                    silentModeHighPower = true;
                    silentThreshhold = 1;
                }
            }
        }
        if (!SILENT_MODE_ENABLED)
        {
            displayWriteNumbers(LETTER_F, LETTER_NONE, LETTER_O, LETTER_F);
        }
        else
        {
            displayWriteNumbers(LETTER_F, LETTER_NONE, silentModeHighPower ? LETTER_H : LETTER_L, silentThreshhold);
        }
        break;
    case MENU_ALARM_LOOPS: // G
        if (CHANGE_BUTTON_PRESSED)
        {
            CHANGE_BUTTON_PRESSED = false;
            currentAlarmLoopSetting++;
            if (currentAlarmLoopSetting >= ALARMS_LOOPS_COUNT)
            {
                currentAlarmLoopSetting = 0;
            }
        }
        if (ALARM_CURRENT_LOOPS > 0)
        {
            displayWriteNumbers(LETTER_G, LETTER_NONE, (ALARM_CURRENT_LOOPS / 10) % 10, ALARM_CURRENT_LOOPS % 10);
        }
        else
        {
            displayWriteNumbers(LETTER_G, LETTER_NONE, LETTER_O, LETTER_O);
        }
        break;
    case MENU_BATTERY: // H
        if (prevBatteryMeasure == BATTERY_NO_MEASURE)
        {
            displayWriteNumbers(LETTER_H, LETTER_NONE, LETTER_UNDERLINE, LETTER_UNDERLINE);
            smartDelay(300);
        }
        helpVal = readBattery();
        prevBatteryMeasure = helpVal;
        smartDelay(10);
        displayWriteNumbers(LETTER_H, LETTER_NONE, (helpVal / 10) % 10, helpVal % 10);
        smartDelay(BATTERY_WAIT_BETWEEN_MEASURES_TIMING);
        break;
    default:
        break;
    }
    smartDelay(100);
}

void menuExit(bool saveSettingsToFlash)
{
    MENU_BUTTON_PRESSED = false;
    CHANGE_BUTTON_PRESSED = false;
    SNOOZE_BUTTON_PRESSED = false;
    currentMenuItem = -1;
    timeSetCurrentDigit = -1;
    displayClear();
    exitedFromMenu = true;
    goToSleep = true;

    if (saveSettingsToFlash)
    {
        flashSaveSettings();
        delay(50);
    }
}

void serialLoop()
{
    byte serialMode = SERIAL_MODE_NONE;
    byte serialBuffer[64];
    byte serialBufferPos = 0;

    do
    {
        while (Serial.available() > 0)
        {
            lastTimeSerialRecieved = millis();
            byte inByte = Serial.read();
            switch (serialMode)
            {
            case SERIAL_MODE_FLASH:
                flashProcessByte(inByte);
                break;
            case SERIAL_MODE_SET_TIME:
                serialBuffer[serialBufferPos] = inByte;
                serialBufferPos++;
                break;
            default:
                serialMode = inByte;
                if (serialMode == SERIAL_MODE_FLASH)
                {
                    flashStart();
                }
                if (serialMode == SERIAL_MODE_DEVICE)
                {
                    Serial.println("Device: EH14");
                    Serial.print("Capacity: ");
                    Serial.println((flash.getCapacity() - SETTINGS_HEADER_LENGTH));
                }
                if (serialMode == SERIAL_MODE_SET_TIME)
                {
                    serialBufferPos = 0;
                }
            }
        }
    } while (millis() - lastTimeSerialRecieved <= SERIAL_TIMEOUT_MODE);

    DateTime now;
    switch (serialMode)
    {
    case SERIAL_MODE_FLASH:
        flashEnd();
        break;
    case SERIAL_MODE_SET_TIME:
        clockSet(serialBuffer[0] * 10 + serialBuffer[1], serialBuffer[2] * 10 + serialBuffer[3], serialBuffer[4] * 10 + serialBuffer[5]);
        now = rtc.now();
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
        break;
    }
}

void silentLoop()
{
    if (!SILENT_MODE_ENABLED)
    {
        return;
    }

    float lightValue = 0;
    for (byte i = 0; i < SILENT_MODE_MEASURES; i++)
    {
        lightValue += ((float)analogRead(PIN_LIGHT_SENSOR)) / SILENT_MODE_MEASURES;
        smartDelay(50);
    }
    bool currentIsSilent = lightValue <= SILENT_MODE_THRESHOLD;
    int lightDiff = lightValue - SILENT_MODE_THRESHOLD;
    if (currentIsSilent != isSilent && (!isSilent || lightDiff > 2))
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

    if (!IS_MENU_ACTIVE && isSilent)
    {
        displayWriteNumbers(LETTER_DASH, LETTER_DASH, LETTER_DASH, LETTER_DASH);
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

void testLoop()
{
    if (testModeEnabled)
    {
        testModeEnabled = false;
        testSequence();
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
        // flash.powerDown();
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
    digitalWrite(PIN_DISPLAY_ENABLE, HIGH);
    delay(150);
    digitalWrite(PIN_SPEAKER_ENABLE, HIGH);
    // flash.powerUp();
    delay(150);
    return true;
}
