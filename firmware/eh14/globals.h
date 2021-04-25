// Include libraries
SPIFlash flash(PIN_FLASH_CS);
RTC_DS3231 rtc;
Adafruit_ZeroI2S i2s(PIN_I2S_FS, PIN_I2S_SCK, PIN_I2S_TX, PIN_I2S_RX);

// Serial communication stuff
#define Serial SERIAL_PORT_USBVIRTUAL
#define SERIAL_MODE_NONE 0
#define SERIAL_MODE_FLASH '@'
byte serialMode = SERIAL_MODE_NONE;

// Audio stuff
#define AUDIO_SAMPLERATE_HZ 22050
#define AUDIO_BUFFER_SIZE 8600
#define AUDIO_MAX_HEADER_SIZE 1279 // 4 + 255*5
#define VOLUMES_COUNT 12

byte audioBuffer[AUDIO_BUFFER_SIZE];
uint32_t volumes[VOLUMES_COUNT] = {250, 500, 1000, 3000, 6000, 12000, 16000, 22000, 28000, 36000, 48000, 62000};
byte currentVolume = 3;

// Samples
#define AUDIO_MAX_SAMPLES 160
uint32_t samplesLenghts[AUDIO_MAX_SAMPLES];
uint32_t samplesOffsets[AUDIO_MAX_SAMPLES];
#define SAMPLE_INTRO 60
#define SAMPLE_OUTRO 61
#define SAMPLE_HOURS 70
#define SAMPLE_HOURS1 71
#define SAMPLE_HOURS2_4 72
#define SAMPLE_MINUTES 80
#define SAMPLE_MINUTES1 81
#define SAMPLE_MINUTES2_4 82
#define SAMPLE_ALARM1 100
#define SAMPLE_ALARM_BASE 100
#define SAMPLE_WARNING 90
#define SAMPLE_TIME_SET 91
#define SAMPLE_TIME_CURRENT 92
#define SAMPLE_ALARM_SET 93
#define SAMPLE_ALARM_CURRENT 94

// Alarm stuff
#define ALARM_MAX_LOOPS 20
byte currentAlarm = 0;
byte alarmsCount = 0;
byte alarmTriggered = false;

// Flow
bool goToSleep = true;
bool menuActive = false;
bool stopPlaying = false;
bool isPlaying = false;

// Silent Mode
#define SILENT_MODE_THRESHOLD 14
#define SILENT_MODE_MEASURES 15
bool silentMode = false;
bool isSilent = false;

// Buttons and timings
#define DEBOUNCE_TIME 250
#define MENU_TIMEOUT 60000

#define BUTTONS_COUNT 3
unsigned long lastTimeButton[BUTTONS_COUNT] = {0, 0, 0};
bool buttonPressed[BUTTONS_COUNT] = {false, false, false};
#define SNOOZE_BUTTON 0
#define MENU_BUTTON 1
#define CHANGE_BUTTON 2

// #define IS_SNOOZE_BUTTON_PRESSED (digitalRead(PIN_SNOOZE_BUTTON) == LOW)
// #define IS_MENU_BUTTON_PRESSED (digitalRead(PIN_MENU_BUTTON) == LOW)
// #define IS_CHANGE_BUTTON_PRESSED (digitalRead(PIN_CHANGE_BUTTON) == LOW)
// #define IS_RC_SWITCH_ON (digitalRead(PIN_RC_SWITCH) == LOW)
#define SNOOZE_BUTTON_PRESSED buttonPressed[SNOOZE_BUTTON]
#define MENU_BUTTON_PRESSED buttonPressed[MENU_BUTTON]
#define CHANGE_BUTTON_PRESSED buttonPressed[CHANGE_BUTTON]

// Menu stuff
int8_t currentMenuItem = -1;
#define IS_MENU_ACTIVE (currentMenuItem != -1)
#define MENU_ITEMS_COUNT 7
#define MENU_VOLUME 0      // a
#define MENU_ALARM_SOUND 1 // b
#define MENU_ALARM_ON 2    // c
#define MENU_ALARM_SET 3   // d
#define MENU_TIME_SET 4    // e
#define MENU_BATTERY 5     // f
#define MENU_SILENT_MODE 6     // g
bool menuSoundHasPlayed = false;

// Time setting
int8_t timeSetCurrentDigit = -1;
byte timeSetDigits[4] = {0, 0, 0, 0};
#define TIME_SET_DIGITS_ACTIVE (timeSetCurrentDigit != -1)

// Clock stuff
#define CLOCK_DEFAULT_HOURS 12
#define CLOCK_DEFAULT_MINUTES 0
#define CLOCK_RESET_WHEN_STARTUP false
#define CLOCK_CHECK_INTERVAL 1000
unsigned long lastClockCheck = 0;

// IR Commands
#define MESSAGE_SNOOZE 0xA1
#define MESSAGE_MENU 0xA2
#define MESSAGE_CHANGE 0xA3
#define MESSAGE_SOUND 0xA4
byte irRecieved = 0;
byte irRecievedCount = 0;

// Functions
void smartDelay(unsigned int d)
{
    for (unsigned int j = 0; j < d / 10; j++)
    {
        delay(10);
        for (byte i = 0; i < BUTTONS_COUNT; i++)
        {
            if (buttonPressed[i])
            {
                return;
            }
        }
    }
}