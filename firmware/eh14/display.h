// Display config
#define DIGITS 4
#define SEGMENTS 7
#define BETWEEN_TIME 60      // YOU MAY CHANGE THIS
#define FAST_BETWEEN_TIME 10 // YOU MAY CHANGE THIS
#define HOLD_TIME 1          // DON'T CHANGE THIS AT ALL
#define SEG_ON 9             // DON'T CHANGE THIS AT ALL
#define SEG_OFF 6            // DON'T CHANGE THIS AT ALL

// Total digit definitions
#define DIGIT_DEFINITIONS 31
#define LETTER_A 10
#define LETTER_B 11
#define LETTER_C 12
#define LETTER_D 13
#define LETTER_E 14
#define LETTER_F 15
#define LETTER_G 16
#define LETTER_H 17
#define LETTER_I 18
#define LETTER_J 19
#define LETTER_K 20
#define LETTER_L 21
#define LETTER_M 22
#define LETTER_N 23
#define LETTER_O 24
#define LETTER_P 25
#define LETTER_Q 26
#define LETTER_R 27
#define LETTER_S 28
#define LETTER_T 29
#define LETTER_NONE (DIGIT_DEFINITIONS - 1)

// Digit and char defintions
byte digitDefinitions[DIGIT_DEFINITIONS] = {
    B00111111, // 0
    B00000110, // 1
    B01011011, // 2
    B01001111, // 3
    B01100110, // 4
    B01101101, // 5
    B01111101, // 6
    B00000111, // 7
    B01111111, // 8
    B01100111, // 9
    B01110111, // A
    B01111100, // b
    B00111001, // C
    B01011110, // d
    B01111001, // E
    B01110001, // F
    B01101111, // g
    B01110110, // H
    B00000110, // I
    B00011110, // J
    B01110101, // k
    B00111000, // L
    B00010101, // M
    B01010100, // n
    B01011100, // o
    B01110011, // P
    B01100111, // Q
    B00110011, // R
    B01101101, // S
    B01111000, // t
    B00000000, // None
};

// In which order to change segments
// Better for coils load than linear (and for the effect :-))
byte segmentOrder[SEGMENTS] = {0, 4, 1, 3, 6, 2, 5};

// Which segments are currently on (as defined in digitDefinitions)
byte prevSegments[DIGITS] = {0, 0, 0, 0};

// Enables boost DC-DC convertor, approx 1s delay is needed for it to work
void displayEnable()
{
    digitalWrite(PIN_DISPLAY_ENABLE, HIGH);
}

// Disables boost DC-DC conterter to save power
void displayDisable()
{
    digitalWrite(PIN_DISPLAY_ENABLE, LOW);
}

// Helper function to clear all the registers
void displayClearAllRegisters()
{
    digitalWrite(PIN_DISPLAY_LATCH, LOW);
    for (byte i = 0; i < 2 * DIGITS; i++)
    {
        shiftOut(PIN_DISPLAY_DATA, PIN_DISPLAY_CLOCK, MSBFIRST, 0);
    }
    digitalWrite(PIN_DISPLAY_LATCH, HIGH);
}

// Writes data to the display using shift registers
// Be carefull not to set '11' in one pair - you WILL damage the mosfets
// Good Example:    00000000 00001001 (turns on the first segment)
// Good Example:    00000000 00000110 (turns off the first segment)
// Bad Example:     00000000 00000011 (fries the first segment mosfets or 595)
void displayWriteData(byte displayNumber, uint16_t value)
{
    // Start writing to the registers
    digitalWrite(PIN_DISPLAY_LATCH, LOW);

    // Shift zeros to displays after the current display (if any)
    for (byte i = displayNumber + 1; i < DIGITS; i++)
    {
        shiftOut(PIN_DISPLAY_DATA, PIN_DISPLAY_CLOCK, MSBFIRST, 0);
        shiftOut(PIN_DISPLAY_DATA, PIN_DISPLAY_CLOCK, MSBFIRST, 0);
    }

    // Shift the current display
    shiftOut(PIN_DISPLAY_DATA, PIN_DISPLAY_CLOCK, MSBFIRST, value >> 8);
    shiftOut(PIN_DISPLAY_DATA, PIN_DISPLAY_CLOCK, MSBFIRST, value & 0xFF);

    // Shift zeros to the display before the current display (if any)
    for (byte i = 0; i < displayNumber; i++)
    {
        shiftOut(PIN_DISPLAY_DATA, PIN_DISPLAY_CLOCK, MSBFIRST, 0);
        shiftOut(PIN_DISPLAY_DATA, PIN_DISPLAY_CLOCK, MSBFIRST, 0);
    }

    // Pull the latch high to update the mosfets
    digitalWrite(PIN_DISPLAY_LATCH, HIGH);

    // Hold the mosfets for 1 ms
    delay(HOLD_TIME);

    // Clear the mosfets
    displayClearAllRegisters();
}

// Sets the segment on/off
// Forcing is made to set the display in the beginning, when the prevSegments values are not known
void displayWriteSegment(byte displayNumber, byte segmentNumber, bool state, bool force)
{
    if (force || bitRead(prevSegments[displayNumber], segmentNumber) != state)
    {
        displayWriteData(displayNumber, (state ? SEG_ON : SEG_OFF) << (segmentNumber * 2));
        delay(BETWEEN_TIME);
        bitWrite(prevSegments[displayNumber], segmentNumber, state);
    }
}

// Sets the segment on/off, not forcing if same
void displayWriteSegment(byte displayNumber, byte segmentNumber, bool state)
{
    displayWriteSegment(displayNumber, segmentNumber, state, false);
}

// Writes a number to a display
void displayWriteNumber(byte displayNumber, byte value)
{
    // non-linear segments changing (as defined in segmentOrder)
    for (byte i = 0; i < SEGMENTS; i++)
    {
        displayWriteSegment(displayNumber, segmentOrder[i], bitRead(digitDefinitions[value], segmentOrder[i]));
    }
}

// Clears all the displays
void displayClear(bool force)
{
    for (byte d = 0; d < DIGITS; d++)
    {
        for (byte s = 0; s < SEGMENTS; s++)
        {
            displayWriteSegment(d, s, false, force);
        }
    }
}

// Clears all the displays
void displayClear()
{
    displayClear(false);
}

// Displays time
void displayTime(byte hours, byte minutes)
{
    displayWriteNumber(0, hours / 10);
    displayWriteNumber(1, hours % 10);
    displayWriteNumber(2, minutes / 10);
    displayWriteNumber(3, minutes % 10);
}

// Display write 4 numbers
void displayWriteNumbers(byte a, byte b, byte c, byte d)
{
    displayWriteNumber(0, a);
    displayWriteNumber(1, b);
    displayWriteNumber(2, c);
    displayWriteNumber(3, d);
}

// Display preparation, clears it, takes approx 1s
void displaySetup()
{
    pinMode(PIN_DISPLAY_LATCH, OUTPUT);
    pinMode(PIN_DISPLAY_DATA, OUTPUT);
    pinMode(PIN_DISPLAY_CLOCK, OUTPUT);
    pinMode(PIN_DISPLAY_LED, OUTPUT);
    pinMode(PIN_DISPLAY_ENABLE, OUTPUT);
    digitalWrite(PIN_DISPLAY_ENABLE, LOW);

    displayClearAllRegisters();

    digitalWrite(PIN_DISPLAY_ENABLE, HIGH);
    digitalWrite(PIN_DISPLAY_LED, LOW);

    delay(1000);
    displayClear(true);
}

// Tests all the digits/letters
void displayTest()
{
    for (byte i = 0; i < DIGIT_DEFINITIONS; i++)
    {
        for (byte d = 0; d < DIGITS; d++)
        {
            displayWriteNumber(d, i);
            delay(200);
        }
    }
}

// Turns on/off the status LED
void displaySetLed(bool ledState)
{
    digitalWrite(PIN_DISPLAY_LED, ledState);
}

// Display "flash" message
void displayFlash()
{
    displayWriteNumber(0, LETTER_F);
    displayWriteNumber(1, LETTER_L);
    displayWriteNumber(2, LETTER_A);
    displayWriteNumber(3, 5);
}

// Display "empty" message
void displayEmpty()
{
    displayWriteNumber(0, LETTER_E);
    displayWriteNumber(1, LETTER_E);
    displayWriteNumber(2, LETTER_E);
    displayWriteNumber(3, LETTER_E);
}

// Flashes LED to signalize clock ready
void displayClockReady()
{
    for (byte i = 0; i < 10; i++)
    {
        displaySetLed(1);
        delay(20);
        displaySetLed(0);
        delay(80);
    }
}