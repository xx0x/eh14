void testSequence()
{
    displayWriteNumbers(LETTER_S, LETTER_E, LETTER_L, LETTER_F, true);
    delay(1000);
    displayWriteNumbers(LETTER_T, LETTER_E, LETTER_S, LETTER_T);
    delay(1000);
    // test display
    for (byte i = 0; i < 10; i++)
    {
        for (byte d = 0; d < DIGITS; d++)
        {
            displayWriteNumber(d, i);
            delay(100);
        }
    }
    delay(100);
    displayClear();

    // test all alarms
    // for (byte i = 0; i < alarmsCount; i++)
    // {
    //     byte n = i + 1;
    //     displayWriteNumber(2, n / 10);
    //     displayWriteNumber(3, n % 10);
    //     saySample(i + SAMPLE_ALARM_BASE);
    //     delay(100);
    // }

    // test last alarm
    byte i = alarmsCount - 1;
    displayWriteNumber(2, alarmsCount / 10);
    displayWriteNumber(3, alarmsCount % 10);
    saySample(i + SAMPLE_ALARM_BASE);
    delay(100);
    displayClear();

    // test alarm
    displayWriteNumber(0, LETTER_A);
    clockSet(6, 59, 58);
    clockAlarmSet(7, 0, 0);
    delay(3000);
    if (alarmTriggered)
    {
        displayWriteNumbers(LETTER_A, LETTER_NONE, LETTER_O, LETTER_K);
    }
    else
    {
        displayWriteNumbers(LETTER_A, LETTER_B, LETTER_A, LETTER_D);
    }
    alarmTriggered = false;
    delay(1000);
    displayClear();

    // test light sensor
    unsigned int lightValue = 0;
    lightValue = analogRead(PIN_LIGHT_SENSOR);
    displayWriteBigNumber(lightValue);
    delay(1000);
    displaySetLed(true);
    delay(100);
    lightValue = analogRead(PIN_LIGHT_SENSOR);
    displayWriteBigNumber(lightValue);
    delay(1000);
    displaySetLed(false);
    displayClear();
}