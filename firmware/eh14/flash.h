uint32_t flashCurrentLength = 0;
uint32_t flashAddress = 0;
bool flashLedOn = false;

uint32_t flashReadInt(uint32_t address)
{
    byte buff[4];
    flash.readByteArray(address, buff, 4);
    return buff[0] << 24 | buff[1] << 16 | buff[2] << 8 | buff[3];
}

bool flashSetup()
{
    Serial.print("EH14 | Flash manufacturer: ");
    Serial.println(flash.getManID());
    Serial.print("EH14 | Flash JEDEC ID: ");
    Serial.println(flash.getJEDECID());
    Serial.print("EH14 | Flash capacity: ");
    Serial.println(flash.getCapacity());
    Serial.println("");

    // Audio data starts at SETTINGS_HEADER_LENGTH addrs (after settings stored in flash memory)
    uint32_t flashHeaderLength = flashReadInt(SETTINGS_HEADER_LENGTH);
    Serial.print("EH14 | Data header length: ");
    Serial.println(flashHeaderLength);

    if (flashHeaderLength > AUDIO_MAX_HEADER_SIZE)
    {
        Serial.println("EH14 | Header too long, ignoring...");
        return false;
    }

    uint32_t prevAddress = flashHeaderLength + SETTINGS_HEADER_LENGTH;

    // clear old stuff
    for (byte i = 0; i < AUDIO_MAX_SAMPLES; i++)
    {
        samplesOffsets[i] = 0;
        samplesLenghts[i] = 0;
    }

    // clear old alarms
    alarmsCount = 0;

    // read new data
    for (uint32_t i = 4 + SETTINGS_HEADER_LENGTH; i < flashHeaderLength + SETTINGS_HEADER_LENGTH; i += 5)
    {
        byte num = flash.readByte(i);
        Serial.print("Reading sample ");
        Serial.println(num);
        if (num >= AUDIO_MAX_SAMPLES)
        {
            break;
        }
        samplesOffsets[num] = prevAddress;
        samplesLenghts[num] = flashReadInt(i + 1);
        prevAddress += samplesLenghts[num];
        if (num >= SAMPLE_ALARM_BASE)
        {
            alarmsCount++;
        }
    }

    // check alarm, whether available
    if (currentAlarm >= alarmsCount)
    {
        currentAlarm = 0;
    }

    // display new data
    for (byte i = 0; i < AUDIO_MAX_SAMPLES; i++)
    {
        if (samplesLenghts[i] != 0)
        {
            Serial.print("EH14 | Sample ");
            if (i < 10)
            {
                Serial.print(" ");
            }
            Serial.print(i);
            Serial.print(": offset ");
            Serial.print(samplesOffsets[i]);
            Serial.print(", length ");
            Serial.println(samplesLenghts[i]);
        }
    }
    Serial.print("EH14 | Found alarms: ");
    Serial.println(alarmsCount);

    return true;
}

void flashProcessByte(byte data)
{
    audioBuffer[flashCurrentLength] = data;
    if (flashCurrentLength >= AUDIO_BUFFER_SIZE)
    {
        flash.writeByteArray(flashAddress - flashCurrentLength, audioBuffer, flashCurrentLength);
        Serial.print(".");
        flashCurrentLength = 0;
        flashLedOn = !flashLedOn;
        displaySetLed(flashLedOn);
    }
    else
    {
        flashCurrentLength++;
    }
    flashAddress++;
}

void flashStart()
{
    delay(20);
    displayFlash();
    delay(20);
    flashAddress = SETTINGS_HEADER_LENGTH;
    flashCurrentLength = 0;
    Serial.print("EH14 | Flash manufacturer: ");
    Serial.println(flash.getManID());
    Serial.print("EH14 | Flash JEDEC ID: ");
    Serial.println(flash.getJEDECID());
    Serial.print("EH14 | Flash capacity: ");
    Serial.println(flash.getCapacity());
    Serial.println("");
    Serial.println("EH14 | Erasing chip.");
    flash.eraseChip();
    Serial.println("EH14 | Chip erased.");
    Serial.println("EH14 | Starting flashing.");
}

void flashEnd()
{
    Serial.println("");
    Serial.println("EH14 | Flashing ended.");
    if (flashAddress > SETTINGS_HEADER_LENGTH)
    {
        flash.writeByteArray(flashAddress - flashCurrentLength, audioBuffer, flashCurrentLength); // write remainings
        Serial.print("EH14 | ");
        Serial.print(flashAddress);
        Serial.println(" bytes written.");
        flashSetup();
        displayClear();
    }
}

void flashLoadSettings()
{
    byte settings[SETTINGS_HEADER_LENGTH_REAL];
    flash.readByteArray(0, settings, SETTINGS_HEADER_LENGTH_REAL, false);
    currentVolume = settings[SETTINGS_HEADER_VOLUME] < VOLUMES_COUNT ? settings[SETTINGS_HEADER_VOLUME] : currentVolume;
    currentAlarm = settings[SETTINGS_HEADER_ALARM] < alarmsCount ? settings[SETTINGS_HEADER_ALARM] : currentAlarm;
    silentThreshhold = settings[SETTINGS_HEADER_SILENT_THRESHOLD] < SILENT_MODE_THRESHOLDS_COUNT ? settings[SETTINGS_HEADER_SILENT_THRESHOLD] : silentThreshhold;
    silentModeHighPower = settings[SETTINGS_HEADER_SILENT_HIGH_POWER] == 1;
    currentAlarmLoops = settings[SETTINGS_HEADER_ALARM_LOOPS] < ALARMS_LOOPS ? settings[SETTINGS_HEADER_ALARM_LOOPS] : currentAlarmLoops;
    Serial.println("Reading settings from flash: ");
    for (byte i = 0; i < SETTINGS_HEADER_LENGTH_REAL; i++)
    {
        Serial.print(settings[i], HEX);
        Serial.print(" ");
    }
}

bool flashSettingsHasChanged()
{
    byte settings[SETTINGS_HEADER_LENGTH_REAL];
    flash.readByteArray(0, settings, SETTINGS_HEADER_LENGTH_REAL, false);
    return !(
        settings[SETTINGS_HEADER_VOLUME] == currentVolume &&
        settings[SETTINGS_HEADER_ALARM] == currentAlarm &&
        settings[SETTINGS_HEADER_SILENT_THRESHOLD] == silentThreshhold &&
        settings[SETTINGS_HEADER_SILENT_HIGH_POWER] == (silentModeHighPower ? 1 : 0) &&
        settings[SETTINGS_HEADER_ALARM_LOOPS] == currentAlarmLoops);
}

void flashSaveSettings()
{
    if (!flashSettingsHasChanged())
    {
        Serial.println("Settings same, not saving to flash.");
        return;
    }

    byte settings[SETTINGS_HEADER_LENGTH_REAL];
    settings[SETTINGS_HEADER_VOLUME] = currentVolume;
    settings[SETTINGS_HEADER_ALARM] = currentAlarm;
    settings[SETTINGS_HEADER_SILENT_THRESHOLD] = silentThreshhold;
    settings[SETTINGS_HEADER_SILENT_HIGH_POWER] = silentModeHighPower ? 1 : 0;
    settings[SETTINGS_HEADER_ALARM_LOOPS] = currentAlarmLoops;

    Serial.print("Flash erase sector 0 ");
    if (flash.eraseSector(0))
    {
        Serial.println(" ... OK");
    }
    else
    {
        Serial.println(" ... not OK");
    }

    delay(10);

    Serial.println("Saving settings to flash: ");
    for (byte i = 0; i < SETTINGS_HEADER_LENGTH_REAL; i++)
    {
        Serial.print(settings[i], HEX);
        Serial.print(" ");
    }

    if (flash.writeByteArray(0, settings, SETTINGS_HEADER_LENGTH_REAL, true))
    {
        Serial.println(" ... OK");
    }
    else
    {
        Serial.println(" ... not OK");
    }
}