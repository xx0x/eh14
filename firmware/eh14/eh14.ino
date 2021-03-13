#include <SPIMemory.h> // modified
#include "Adafruit_ZeroI2S.h"
#include "pins.h"

#define IS_SNOOZE_BUTTON_PRESSED (digitalRead(PIN_SNOOZE_BUTTON) == LOW)
#define IS_MENU_BUTTON_PRESSED (digitalRead(PIN_MENU_BUTTON) == LOW)
#define IS_CHANGE_BUTTON_PRESSED (digitalRead(PIN_CHANGE_BUTTON) == LOW)
#define IS_RC_SWITCH_ON (digitalRead(PIN_RC_SWITCH) == LOW)

#define SAMPLERATE_HZ 22050
#define MAX_SAMPLES 160
#define Serial SERIAL_PORT_USBVIRTUAL

SPIFlash flash(PIN_FLASH_CS);
Adafruit_ZeroI2S i2s(PIN_I2S_FS, PIN_I2S_SCK, PIN_I2S_TX, PIN_I2S_RX);

// Volume
#define VOLUMES_COUNT 8
byte currentVolume = 7;
uint32_t volumes[VOLUMES_COUNT] = {0, 2000, 4000, 8000, 12000, 16000, 26000, 34000};

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

// Include parts
#include "display.h"
#include "samples.h"
#include "flash.h"
#include "say.h"

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
}

void loop()
{
    serialLoop();

    if (IS_SNOOZE_BUTTON_PRESSED)
    {
        displaySetLed(true);
        displayTime(16, 25);
        sayTime(16, 25, 0);
        displaySetLed(false);
    }

    if (IS_MENU_BUTTON_PRESSED)
    {
        displaySetLed(true);
        saySample(SAMPLE_ALARM_BASE + 9);
        displaySetLed(false);
    }
    
    if (IS_CHANGE_BUTTON_PRESSED)
    {
        displaySetLed(true);
        saySample(SAMPLE_ALARM_BASE + 19);
        displaySetLed(false);
    }
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