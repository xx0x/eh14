void digitsPrepare(bool prepareAlarm)
{
    DateTime now;
    if (!prepareAlarm)
    {
        now = rtc.now();
    }
    else
    {
        now = rtc.getAlarmDateTime(1);
    }
    int hh = now.hour();
    int mm = now.minute();
    timeSetDigits[0] = (byte)(hh / 10);
    timeSetDigits[1] = (byte)(hh % 10);
    timeSetDigits[2] = (byte)(mm / 10);
    timeSetDigits[3] = (byte)(mm % 10);
}

void digitsSaveAsClock()
{
    clockSet(timeSetDigits[0] * 10 + timeSetDigits[1], timeSetDigits[2] * 10 + timeSetDigits[3]);
}

void digitsSaveAsAlarm()
{
    clockAlarmSet(timeSetDigits[0] * 10 + timeSetDigits[1], timeSetDigits[2] * 10 + timeSetDigits[3]);
}

void digitsNext()
{
    timeSetCurrentDigit++;
    if (timeSetDigits[0] > 1 && timeSetDigits[1] > 3)
    {
        timeSetDigits[1] = 0;
    }
}

void digitsIncrease()
{
    timeSetDigits[timeSetCurrentDigit]++;
    if (timeSetDigits[timeSetCurrentDigit] > 9)
    {
        timeSetDigits[timeSetCurrentDigit] = 0;
    }
    if (timeSetCurrentDigit == 0 && timeSetDigits[0] > 2)
    {
        timeSetDigits[0] = 0;
    }
    if (timeSetCurrentDigit == 1 && timeSetDigits[0] == 2 && timeSetDigits[1] > 3)
    {
        timeSetDigits[1] = 0;
    }
    if (timeSetCurrentDigit == 2 && timeSetDigits[2] > 5)
    {
        timeSetDigits[2] = 0;
    }
}