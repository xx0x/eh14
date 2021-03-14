#include <SPIMemory.h> // modified
#include "Adafruit_ZeroI2S.h"
#include "pins.h"
#include "RTClib.h" // modified

#define IS_SNOOZE_BUTTON_PRESSED (digitalRead(PIN_SNOOZE_BUTTON) == LOW)
#define IS_MENU_BUTTON_PRESSED (digitalRead(PIN_MENU_BUTTON) == LOW)
#define IS_CHANGE_BUTTON_PRESSED (digitalRead(PIN_CHANGE_BUTTON) == LOW)
#define IS_RC_SWITCH_ON (digitalRead(PIN_RC_SWITCH) == LOW)

#define SAMPLERATE_HZ 22050
#define MAX_SAMPLES 160
#define Serial SERIAL_PORT_USBVIRTUAL

SPIFlash flash(PIN_FLASH_CS);
RTC_DS3231 rtc;
Adafruit_ZeroI2S i2s(PIN_I2S_FS, PIN_I2S_SCK, PIN_I2S_TX, PIN_I2S_RX);

// Volume
#define VOLUMES_COUNT 8
byte currentVolume = 3;
uint32_t volumes[VOLUMES_COUNT] = {0, 500, 1000, 3000, 6000, 12000, 16000, 22000};

// Alarm stuff
#define ALARMS_COUNT alarmsCount
#define ALARM_MAX_LOOPS 20
byte currentAlarm = 0;
byte alarmsCount = 0;
byte alarmTriggered = false;

// Flow
bool goToSleep = true;
bool menuActive = false;
bool stopPlaying = false;
bool isPlaying = false;

// Buttons and timings
#define DEBOUNCE_TIME 250
bool snoozeButtonPressed = false;
bool menuButtonPressed = false;
bool changeButtonPressed = false;
unsigned long lastTimeSnoozeButton = 0;
unsigned long lastTimeMenuButton = 0;
unsigned long lastTimeChangeButton = 0;
#define MENU_TIMEOUT 30000

// Include parts
#include "./display.h"
#include "./clock.h"
#include "./samples.h"
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

    for (byte i = 0; i < 10; i++)
    {
        displaySetLed(1);
        delay(20);
        displaySetLed(0);
        delay(80);
    }

    flash.begin();

    while (!flashSetup())
    {
        displayEmpty();
        delay(1000);
        serialLoop();
        delay(1000);
    }

    saySetup();

    Serial.println("EH14 ready\n");
    // clockAlarmSet(12, 10);
}

void loop()
{
    alarmLoop();
    serialLoop();
    timeLoop();
}

bool stopAlarm()
{
    if (alarmTriggered)
    {
        stopPlaying = true;
        return true;
    }
    return false;
}

void callbackSnoozeButton()
{
    if (millis() - lastTimeSnoozeButton < DEBOUNCE_TIME)
    {
        return;
    }
    lastTimeSnoozeButton = millis();
    if (!stopAlarm())
    {
        snoozeButtonPressed = true;
    }
}

void callbackMenuButton()
{
    if (millis() - lastTimeMenuButton < DEBOUNCE_TIME)
    {
        return;
    }
    lastTimeMenuButton = millis();
    if (!stopAlarm())
    {
        menuButtonPressed = true;
    }
}

void callbackChangeButton()
{
    if (millis() - lastTimeChangeButton < DEBOUNCE_TIME)
    {
        return;
    }
    lastTimeChangeButton = millis();
    if (!stopAlarm())
    {
        changeButtonPressed = true;
    }
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
    if (snoozeButtonPressed)
    {
        sayTime(now.hour(), now.minute(), 0);
        snoozeButtonPressed = false;
    }
    delay(100);
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