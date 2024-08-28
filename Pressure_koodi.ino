/*
  Basic test of the Qwiic MicroPressure Sensor
  By: Alex Wende
  SparkFun Electronics
  Date: July 2020
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/16476
  
  This example demonstrates how to get started with the Qwiic MicroPressure Sensor board, and read pressures in various units.
*/

// Include the SparkFun MicroPressure library.
// Click here to get the library: http://librarymanager/All#SparkFun_MicroPressure

#include<Wire.h>
#include <SparkFun_MicroPressure.h>
#include <ThingSpeak.h> //thingspeak:n (IOT)-toimimiseen vaadittava kirjasto*/
#include <SPI.h> //mikrokontrollerille vaadittava sarjaväyläliitäntäkirjasto
#include <WiFiNINA.h>
#include <SD.h> //kirjasto SD-kortille tallentamista varten
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SoftwareSerial.h>

#define CLK_PIN SCK
#define DATA_PIN MOSI
#define CS_PIN  SS

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 20;

File myFile;

char ssid[] = "HAMKvisitor";   // Verkon nimi
char pass[] = "hamkvisitor";   // Verkon salasana
int keyIndex = 0;            // Index numero (vain WEP-yhteyksille)
WiFiClient  client;
WiFiUDP ntpUDP;


// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);
// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

String formattedDate;
String dayStamp;
String timeStamp;

unsigned long myChannelNumber = 1685324; //Thingspeak kanavan numero
const char * myWriteAPIKey = "OLV3HEN5U9T9L8SM";  //Thingspeak API-Key numero

/*
 * Initialize Constructor
 * Optional parameters:
 *  - EOC_PIN: End Of Conversion (defualt: -1)
 *  - RST_PIN: Reset (defualt: -1)
 *  - MIN_PSI: Minimum Pressure (default: 0 PSI)
 *  - MAX_PSI: Maximum Pressure (default: 25 PSI)
 */
//SparkFun_MicroPressure mpr(EOC_PIN, RST_PIN, MIN_PSI, MAX_PSI);
SparkFun_MicroPressure mpr; // Use default values with reset and EOC pins unused

//int SDA_PIN=A4;
//int SCL_PIN=A5;



void setup() {
  // Initalize UART, I2C bus, and connect to the micropressure sensor
  Serial.begin(115200);
  Wire.begin();

  /* The micropressure sensor uses default settings with the address 0x18 using Wire.

     The mircropressure sensor has a fixed I2C address, if another address is used it
     can be defined here. If you need to use two micropressure sensors, and your
     microcontroller has multiple I2C buses, these parameters can be changed here.

     E.g. mpr.begin(ADDRESS, Wire1)

     Will return true on success or false on failure to communicate. */
  if(!mpr.begin())
  {
    Serial.println("Cannot connect to MicroPressure sensor.");
    while(1);
  }
  { 

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print("Initializing SD card...");
  if (!SD.begin(20)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  pinMode(20, OUTPUT);

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }
  // print the type of card
  Serial.print("\nCard type: ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }


  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  Serial.println();

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize *= 512;                            // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);


  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);

  ThingSpeak.begin(client); //aloita thingspeak
}
}

void loop()
{
     pressure();
     wifi();
     timeclient();
}

void pressure() {
  /* The micropressure sensor outputs pressure readings in pounds per square inch (PSI).
     Optionally, if you prefer pressure in another unit, the library can convert the
     pressure reading to: pascals, kilopascals, bar, torr, inches of murcury, and
     atmospheres.
   */
  Serial.print(mpr.readPressure(),4);
  Serial.println(" PSI");
  Serial.print(mpr.readPressure(PA),1);
  Serial.println(" Pa");
  Serial.print(mpr.readPressure(KPA),4);
  Serial.println(" kPa");
  Serial.print(mpr.readPressure(TORR),3);
  Serial.println(" torr");
  Serial.print(mpr.readPressure(INHG),4);
  Serial.println(" inHg");
  Serial.print(mpr.readPressure(ATM),6);
  Serial.println(" atm");
  Serial.print(mpr.readPressure(BAR),6);
  Serial.println(" bar");
  Serial.println();
  delay(500);
}
void wifi(){

  //Yhdistä tai uudelleenyhdistä wifiin
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Yritetään yhdistää Wifiin: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);  // yhdistä wifiin
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nYhdistetty Wifiin.");
  }
}
void timeclient(){
  
    while(!timeClient.update()) {
  timeClient.forceUpdate();
}

{
      formattedDate = timeClient.getFormattedDate();

      // Extract date
         int splitT = formattedDate.indexOf("T");
         dayStamp = formattedDate.substring(0, splitT);
         Serial.print("DATE: ");
         Serial.println(dayStamp);
      // Extract time
         timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
         Serial.print("HOUR: ");
         Serial.println(timeStamp);
         delay(1000);
}
}
