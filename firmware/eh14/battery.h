// returns battery current charge in range 0-99
byte readBattery()
{
    long measuredvbat = analogRead(PIN_BATSENSOR);
    measuredvbat = constrain(measuredvbat, BATTERY_MEASURE_MIN, BATTERY_MEASURE_MAX);
    byte byteval = map(measuredvbat, BATTERY_MEASURE_MIN, BATTERY_MEASURE_MAX, 0, 99);
    return byteval;
}