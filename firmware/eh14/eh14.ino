#include "pins.h"
#include "Adafruit_ZeroI2S.h"
#include <SPIMemory.h> // modified
#include "RTClib.h"    // modified

// Include parts
#include "./globals.h"
#include "./display.h"
#include "./clock.h"
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
    alarmLoop();
    serialLoop();
    timeLoop();
    menuLoop();
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
        currentAlarm++;
        if (currentAlarm >= alarmsCount)
        {
            currentAlarm = 0;
        }
        displayWriteNumbers(LETTER_B, LETTER_NONE, currentAlarm / 10, currentAlarm % 10);
        saySample(currentAlarm + SAMPLE_ALARM_BASE);
    }
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