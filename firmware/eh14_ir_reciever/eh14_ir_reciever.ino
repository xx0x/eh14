#include <tiny_IRremote.h>

// PIN DEFINITIONS
#define PIN_ENABLED 4
#define PIN_IR 3
#define PIN_LATCH 0
#define PIN_DATA 1
#define PIN_CLOCK 2

// IR STUFF
IRrecv irrecv(PIN_IR);
decode_results results;

#define IR_CODE_SNOOZE 0xF710EF
#define IR_CODE_MENU 0xF7906F
#define IR_CODE_CHANGE 0xF750AF
#define IR_CODE_SOUND 0xF7D02F

#define MESSAGE_SNOOZE 0xA1
#define MESSAGE_MENU 0xA2
#define MESSAGE_CHANGE 0xA3
#define MESSAGE_SOUND 0xA4

void setup()
{
    pinMode(PIN_ENABLED, INPUT);
    pinMode(PIN_LATCH, OUTPUT);
    pinMode(PIN_DATA, OUTPUT);
    pinMode(PIN_CLOCK, OUTPUT);
    digitalWrite(PIN_LATCH, HIGH);
    sendMessage(0);
    irrecv.enableIRIn();
}

void loop()
{
    if (irrecv.decode(&results))
    {
        processIrCode(results.value);
        irrecv.resume();
    }
}

void processIrCode(long code)
{
    switch (code)
    {
    case IR_CODE_MENU:
        sendMessage(MESSAGE_MENU);
        break;
    case IR_CODE_CHANGE:
        sendMessage(MESSAGE_CHANGE);
        break;
    case IR_CODE_SNOOZE:
        sendMessage(MESSAGE_SNOOZE);
        break;
    case IR_CODE_SOUND:
        sendMessage(MESSAGE_SOUND);
        break;
    }
}

void slowShiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t val)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        digitalWrite(dataPin, !!(val & (1 << (7 - i))));
        digitalWrite(clockPin, HIGH);
        delay(9);
        digitalWrite(clockPin, LOW);
        delay(1);
    }
}

void sendMessage(byte b)
{
    digitalWrite(PIN_LATCH, LOW);
    delay(5);
    slowShiftOut(PIN_DATA, PIN_CLOCK, b);
    delay(5);
    digitalWrite(PIN_LATCH, HIGH);
}
