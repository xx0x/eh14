#include <tiny_IRremote.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// PIN DEFINITIONS
#define PIN_SNOOZE_BUTTON 0
#define PIN_MENU_BUTTON 1
#define PIN_CHANGE_BUTTON 2
#define PIN_SOUND_BUTTON 3
#define PIN_IR 4 // just for reference (specified in library)

// Buttons
#define BUTTON_COUNT 4
byte buttons[BUTTON_COUNT] = {PIN_SNOOZE_BUTTON, PIN_MENU_BUTTON, PIN_CHANGE_BUTTON, PIN_SOUND_BUTTON};
bool buttonsPressed[BUTTON_COUNT] = {false, false, false, false};

// IR STUFF
IRsend irsend;
#define IR_CODE_SNOOZE 0xAF710EFA
#define IR_CODE_MENU 0xAF7906FA
#define IR_CODE_CHANGE 0xAF750AFA
#define IR_CODE_SOUND 0xAF7D02FA

unsigned long irCodes[BUTTON_COUNT] = {IR_CODE_SNOOZE, IR_CODE_MENU, IR_CODE_CHANGE, IR_CODE_SOUND};

void setup()
{
    for (byte i = 0; i < BUTTON_COUNT; i++)
    {
        pinMode(buttons[i], INPUT_PULLUP);
    }
}

void loop()
{
    for (byte i = 0; i < BUTTON_COUNT; i++)
    {
        if (digitalRead(buttons[i]) == LOW && !buttonsPressed[i])
        {
            irsend.sendNEC(irCodes[i], 64); // 8x8 = 64
            delay(100);
        }
        if (digitalRead(buttons[i]) == HIGH)
        {
            buttonsPressed[i] = false;
        }
    }
    delay(50);
    doSleep();
}

ISR(PCINT0_vect)
{
}

// Falling to sleep and waking up when button presses
void doSleep()
{
    GIMSK |= _BV(PCIE);
    PCMSK |= _BV(PCINT0);
    PCMSK |= _BV(PCINT1);
    PCMSK |= _BV(PCINT2);
    PCMSK |= _BV(PCINT3);
    ADCSRA &= ~_BV(ADEN); // ADC off
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sei();
    sleep_cpu();

    // Now waking up
    cli();
    PCMSK &= ~_BV(PCINT0);
    PCMSK &= ~_BV(PCINT1);
    PCMSK &= ~_BV(PCINT2);
    PCMSK &= ~_BV(PCINT3);
    sleep_disable();
    ADCSRA |= _BV(ADEN); // ADC on
    sei();
}
