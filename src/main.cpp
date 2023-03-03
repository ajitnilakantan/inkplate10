// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#if !defined(ARDUINO_INKPLATE10) && !defined(ARDUINO_INKPLATE10V2)
#error "Wrong board selection for this example, please select Inkplate 10 in the boards menu."
#endif

#include "Inkplate.h" //Include Inkplate library to the sketch
#include "EEPROM.h"

Inkplate display(INKPLATE_3BIT); // Create an object on Inkplate library and also set library into 3-bit mode (Greyscale)

char sbuf[1024]; // sprintf buffer

// Forwards
extern void buildIndex();
extern void shuffleIndex();

/*
** Settings
*/
#define BATTERY_WARNING_LEVEL 3.7

const char *PHOTOS_FOLDER = "/images";
// #define uS_TO_SLEEP 10800000000 // 3h
// #define uS_TO_SLEEP 5400000000 //1.5h
#define uS_TO_SLEEP 2700000000 // 45m
// #define uS_TO_SLEEP 20000000 // 10s
//  #define uS_TO_SLEEP 10000000 // 5s
//  #define uS_TO_SLEEP 3600000000 // 1h

/*
** Perist this to EEPROM
*/
#define EEPROM_SIZE 512      // Init EEPROM library with 512 of EEPROM size. Do not change this value, it can wipe waveform data!
#define EEPROM_START_ADDR 76 // Start EEPROM address for user data. Addresses below address 76 are waveform data!
#define MAX_PHOTOS 512
#define EEPROM_MAGIC (EEPROM_START_ADDR + 0)
const char eepromMagic[20] = "INKPLATE PHOTOFRAME";
#define EEPROM_PHOTO_COUNT (EEPROM_MAGIC + sizeof(eepromMagic))
uint16_t photoCount;
#define EEPROM_NEXT_PHOTO_INDEX (EEPROM_PHOTO_COUNT + sizeof(photoCount))
uint16_t nextPhotoIndex;
#define EEPROM_PHOTO_INDEX_LIST (EEPROM_NEXT_PHOTO_INDEX + sizeof(nextPhotoIndex))
uint16_t photoIndexList[MAX_PHOTOS];

/*
** Deep sleep
*/
void gotoSleep()
{
    Serial.println("Waiting 2.5s for everything to settle...");
    delay(2500);
    Serial.println("Going to sleep");

    // Turn off the power supply for the SD card
    display.sdCardSleep();
    // Isolate/disable GPIO12 on ESP32 (only to reduce power consumption in sleep)
    esp_sleep_enable_timer_wakeup(uS_TO_SLEEP); // Activate wake-up timer
    // Enable wakeup from deep sleep on gpio 36 (wake button)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW);
    // Put ESP32 into deep sleep. Program stops here.
    esp_deep_sleep_start();
}

/*
** Persist data: Read/write from the EEPROM
*/
void invalidateEEPROM()
{
    EEPROM.put(EEPROM_MAGIC, "INVALID");
    EEPROM.commit();
}

void updateEEPROM()
{
    // Truncate the photoCount, size the EEPROM size is limited
    uint16_t count = (EEPROM_SIZE - EEPROM_PHOTO_INDEX_LIST) / sizeof(photoIndexList[0]);
    if (photoCount > count)
    {
        photoCount = count;
        Serial.println((sprintf(sbuf, "Truncated photoCount to %d", photoCount), sbuf));
    }
    EEPROM.put(EEPROM_MAGIC, eepromMagic);
    EEPROM.put(EEPROM_PHOTO_COUNT, photoCount);
    EEPROM.put(EEPROM_NEXT_PHOTO_INDEX, nextPhotoIndex);
    u_char *bytes = (u_char *)photoIndexList;
    for (int i = 0; i < photoCount * sizeof(photoIndexList[0]); i++)
    {
        EEPROM.put(EEPROM_PHOTO_INDEX_LIST + i, bytes[i]);
    }

    EEPROM.commit();
}

void readEEPROM()
{
    Serial.println("Reading EEPROM...");
    EEPROM.get(EEPROM_PHOTO_COUNT, photoCount);
    EEPROM.get(EEPROM_NEXT_PHOTO_INDEX, nextPhotoIndex);
    u_char *bytes = (u_char *)photoIndexList;
    for (int i = 0; i < photoCount * sizeof(photoIndexList[0]); i++)
    {
        EEPROM.get(EEPROM_PHOTO_INDEX_LIST + i, bytes[i]);
    }

    Serial.println((sprintf(sbuf, "EEPROM get photoCount=%d nextPhotoIndex=%d", photoCount, nextPhotoIndex), sbuf));
}

void initEEPROM()
{
    char magic[20];

    // Init EEPROM library with 512 of EEPROM size. Do not change this value, it can wipe waveform data!
    if (!EEPROM.begin(EEPROM_SIZE))
    {
        display.println("EEPROM initialization error!");
        Serial.println("EEPROM initialization error!");
        display.display();
        gotoSleep();
    }

    EEPROM.get(EEPROM_MAGIC, magic);
    if (strncmp(magic, eepromMagic, 20) != 0)
    {
        Serial.println("No valid config found formatting EEPROM.");
        buildIndex();
        shuffleIndex();
        updateEEPROM();
    }
}

/*
** Functions to read the "images" folder
*/
SdFile file;
SdFile dir;

void buildIndex()
{
    nextPhotoIndex = 0;
    photoCount = 0;

    dir.rewind();
    while (true)
    {
        // Serial.print("Scanning file: ");
        if (!file.openNext(&dir, O_RDONLY))
        {
            Serial.println((sprintf(sbuf, "Scanned %d photos", photoCount), sbuf));
            return;
        }

        // Serial.print(file.dirIndex());
        // Serial.print(", ");
        // file.printName(&Serial);
        // Serial.println();

        if (file.isDir())
        {
            Serial.println("Skipping directory");
            file.close();
            continue;
        }

        if (file.isHidden())
        {
            Serial.println("Skipping hidden file");
            file.close();
            continue;
        }

        photoIndexList[photoCount++] = file.dirIndex();
        file.close();

        if (photoCount >= MAX_PHOTOS)
        {
            Serial.println((sprintf(sbuf, "Max photo count of %d reached. Stopping scan.", MAX_PHOTOS), sbuf));
            break;
        }
    }
}

void shuffleIndex()
{
    // Fisher Yates
    randomSeed(millis());
    for (uint16_t i = 0; i < photoCount - 2; i++)
    {
        // i <= j < n
        uint16_t j = random(i, photoCount);
        // Swap i,j
        uint16_t tmp = photoIndexList[i];
        photoIndexList[i] = photoIndexList[j];
        photoIndexList[j] = tmp;
    }
}

void initSd()
{
    uint8_t retries = 5;
    uint8_t retry_delay = 100;
    while (!display.sdCardInit() && retries > 0)
    {
        Serial.println("SD initialization error, retrying!");
        retries--;
        delay(retry_delay);
    }
    if (retries == 0)
    {
        display.println("SD initialization error!");
        Serial.println("SD initialization error!");
        display.display();
        invalidateEEPROM();
        gotoSleep();
        return;
    }
    else
    {
        Serial.println("SD Initialized.");
    }
}

void openPhotoDirectory()
{
    if (dir.open(PHOTOS_FOLDER) == 0)
    {
        sprintf(sbuf, "Could not open folder: '%s'", PHOTOS_FOLDER);
        display.println(sbuf);
        Serial.println(sbuf);
        display.display();
        invalidateEEPROM();
        gotoSleep();
        return;
    }
    else
    {
        Serial.println("Directory opened.");
    }
}

void readAndDisplayPhoto()
{
    if (!file.open(&dir, photoIndexList[nextPhotoIndex], O_RDONLY))
    {
        sprintf(sbuf, "Could not open photoIndex=%d nextPhotoIndex=%d", photoIndexList[nextPhotoIndex], nextPhotoIndex);
        Serial.println(sbuf);
        display.println(sbuf);
        display.display();
        invalidateEEPROM();
        gotoSleep();
        return;
    }
    char name[1024] = {0};
    file.getName(name, sizeof(name));
    file.close();
    String strName = name;
    String slash = "/";
    String fileName = PHOTOS_FOLDER + slash + name;

    if (!display.drawImage(fileName, 0, 0, false, false))
    {
        sprintf(sbuf, "Could not draw picture file: '%s'", fileName);
        Serial.println(sbuf);
        display.println(sbuf);
        display.display();
        invalidateEEPROM();
        gotoSleep();
        return;
    }
}

/*
** Battery level indicator
*/
void checkBattery()
{
    double batteryLevel = display.readBattery();
    Serial.print("Battery level: ");
    Serial.println(batteryLevel);
    if (batteryLevel < (double)BATTERY_WARNING_LEVEL)
    {
        display.setTextColor(7, 0);
        display.setCursor(0, E_INK_HEIGHT - 26);
        display.print("Battery level low! (");
        display.print(batteryLevel);
        display.println(")");
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(1);
    }

    display.begin();             // Init Inkplate library (you should call this function ONLY ONCE)
    display.rtcClearAlarmFlag(); // Clear alarm flag from any previous alarm

    // display.display(true);      // Put clear image on display
    display.setTextSize(2); // Scale text to be two times bigger then original (5x7 px)
    // display.setTextColor(BLACK, WHITE); // Set text color to black and background color to white
    display.setTextColor(0, 7);
    display.setTextWrap(true);

    // Display wake up reason
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        Serial.println("Wakeup caused by WakeUp button");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("Wakeup caused by timer");
        break;
    default:
        Serial.println("Wakeup was not caused by deep sleep");
        break;
    }

    initSd();
    openPhotoDirectory();
    initEEPROM();
    readEEPROM();

    readAndDisplayPhoto();
    if (nextPhotoIndex >= photoCount - 1)
    {
        // Reshuffle and reset for next run needed
        Serial.println("End of Photos reached. Reindexing and Reshuffling...");
        buildIndex();
        shuffleIndex();
    }
    else
    {
        nextPhotoIndex++;
    }
    updateEEPROM();
    checkBattery();

    display.display();
    gotoSleep();
}

void loop()
{
    // Nothing here due to deep sleep.
}
