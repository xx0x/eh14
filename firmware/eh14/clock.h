void clockSet(byte hr, byte min, byte ss)
{
    rtc.adjust(DateTime(2020, 3, 31, hr, min, ss));
}

void clockSet(byte hr, byte min)
{
    clockSet(hr, min, 0);
}

void clockAlarmSet(byte hr, byte min, byte ss)
{
    rtc.clearAlarm(1);
    rtc.setAlarm1(DateTime(2020, 3, 14, hr, min, ss), DS3231_A1_Hour);
}

void clockAlarmSet(byte hr, byte min)
{
    clockAlarmSet(hr, min, 0);
}

void clockClearAlarm()
{
    rtc.clearAlarm(1);
}

void clockEnableAlarm()
{
    rtc.enableAlarm(1);
}

void clockDisableAlarm()
{
    rtc.disableAlarm(1);
}

bool clockIsAlarmEnabled()
{
    return rtc.isAlarmEnabled(1);
}

void clockSetup()
{
    rtc.begin();
    if (CLOCK_RESET_WHEN_STARTUP || rtc.lostPower())
    {
        clockSet(CLOCK_DEFAULT_HOURS, CLOCK_DEFAULT_MINUTES); // 00:00
    }
    rtc.clearAlarm(1);
}

byte clockGetTemp()
{
    return (byte)round(rtc.getTemperature());
}
