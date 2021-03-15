
#define DECODE_NEC 1

// #define MARK_EXCESS_MICROS 20 // 20 is recommended for the cheap VS1838 modules

#include <IRremote.h>

void irSetup()
{
    IrReceiver.begin(PIN_IR, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
}

byte irRecieve()
{
    /*
     * Check if received data is available and if yes, try to decode it.
     * Decoded result is in the IrReceiver.decodedIRData structure.
     *
     * E.g. command is in IrReceiver.decodedIRData.command
     * address is in command is in IrReceiver.decodedIRData.address
     * and up to 32 bit raw data in IrReceiver.decodedIRData.decodedRawData
     */
    if (IrReceiver.decode())
    {
        // Print a short summary of received data
        IrReceiver.printIRResultShort(&Serial);
        if (IrReceiver.decodedIRData.protocol == UNKNOWN)
        {
            // We have an unknown protocol here, print more info
            IrReceiver.printIRResultRawFormatted(&Serial, true);
        }
        IrReceiver.resume(); // Enable receiving of the next value
        if (
            IrReceiver.decodedIRData.address == 0xEF00 &&
            !(IrReceiver.decodedIRData.flags & (IRDATA_FLAGS_IS_AUTO_REPEAT | IRDATA_FLAGS_IS_REPEAT)))
        {
            switch (IrReceiver.decodedIRData.command)
            {
            case 0x04:
                return IR_SNOOZE;
            case 0x05:
                return IR_MENU;
            case 0x06:
                return IR_CHANGE;
            case 0x07:
                return IR_SOUND;
            }
        }
    }

    return IR_EMPTY;
}