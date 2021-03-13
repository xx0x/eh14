#define BUFFER_SIZE 8600
#define SERIAL_MODE_NONE 0
#define SERIAL_MODE_FLASH '@'
#define MAX_HEADER_SIZE 1279 // 4 + 255*5

byte buffer[BUFFER_SIZE];
byte serialMode = SERIAL_MODE_NONE;

uint32_t currentFlashCount = 0;
uint32_t flashAddress = 0;
bool flashDisplayActive = false;

// Samples
uint32_t samplesLenghts[MAX_SAMPLES];
uint32_t samplesOffsets[MAX_SAMPLES];

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

    uint32_t flashHeaderLength = flashReadInt(0);
    Serial.print("EH14 | Data header length: ");
    Serial.println(flashHeaderLength);

    if (flashHeaderLength > MAX_HEADER_SIZE)
    {
        Serial.println("EH14 | Header too long, ignoring...");
        return false;
    }

    uint32_t prevAddress = flashHeaderLength;

    // clear old stuff
    for (byte i = 0; i < MAX_SAMPLES; i++)
    {
        samplesOffsets[i] = 0;
        samplesLenghts[i] = 0;
    }

    // clear old alarms
    alarmsCount = 0;

    // read new data
    for (uint16_t i = 4; i < flashHeaderLength; i += 5)
    {
        byte num = flash.readByte(i);
        Serial.print("Reading sample ");
        Serial.println(num);
        if (num >= MAX_SAMPLES)
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
    for (byte i = 0; i < MAX_SAMPLES; i++)
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
    buffer[currentFlashCount] = data;
    if (currentFlashCount >= BUFFER_SIZE)
    {
        flash.writeByteArray(flashAddress - currentFlashCount, buffer, currentFlashCount);
        Serial.print(".");
        currentFlashCount = 0;
        flashDisplayActive = !flashDisplayActive;
        displaySetLed(flashDisplayActive);
    }
    else
    {
        currentFlashCount++;
    }
    flashAddress++;
}

void flashStart()
{
    delay(20);
    displayFlash();
    delay(20);
    flashAddress = 0;
    currentFlashCount = 0;
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
    if (flashAddress > 0)
    {
        flash.writeByteArray(flashAddress - currentFlashCount, buffer, currentFlashCount); // write remainings
        Serial.print("EH14 | ");
        Serial.print(flashAddress);
        Serial.println(" bytes written.");
        flashSetup();
        displayClear();
    }
}
