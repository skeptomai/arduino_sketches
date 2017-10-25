
#include <Adafruit_GPS.h>

#ifdef __AVR__
  #include <SoftwareSerial.h>
  #include <avr/sleep.h>
#endif

#include <SPI.h>
#include <SdFat.h>

// Ladyada's logger modified by Bill Greiman to use the SdFat library
//
// This code shows how to listen to the GPS module in an interrupt
// which allows the program to have more 'freedom' - just parse
// when a new NMEA sentence is available! Then access data when
// desired.
//
// Tested and works great with the Adafruit Ultimate GPS Shield
// using MTK33x9 chipset
//    ------> http://www.adafruit.com/products/
// Pick one up today at the Adafruit electronics shop 
// and help support open source hardware & software! -ada

#ifdef __AVR__
SoftwareSerial mySerial(8, 7);
#else
#define mySerial Serial1
#endif

Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  false
/* set to true to only log to SD when GPS has a fix, for debugging, keep it false */
#define LOG_FIXONLY true  

// Set the pins used
#define chipSelect 10
#define ledPin 13

// File system object.
SdFat sd;

// Log file.
SdFile logFile;

//static File logFile;
#define FILE_BASE_NAME "Data"
constexpr uint8_t BASE_NAME_SIZE() {return sizeof(FILE_BASE_NAME) - 1;}
char fileName[13] = FILE_BASE_NAME "00.csv";

// read a Hex value and return the decimal equivalent
uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

// blink out an error code
void error(uint8_t errno) {
/*
  if (SD.errorCode()) {
    putstring("SD error: ");
    Serial.print(card.errorCode(), HEX);
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
  }
  */
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

static bool openFile() {
    // Find an unused file name.
  if (BASE_NAME_SIZE() > 6) {
    error("FILE_BASE_NAME too long");
  }
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE() + 1] != '9') {
      fileName[BASE_NAME_SIZE() + 1]++;
    } else if (fileName[BASE_NAME_SIZE()] != '9') {
      fileName[BASE_NAME_SIZE() + 1] = '0';
      fileName[BASE_NAME_SIZE()]++;
    } else {
      error("Can't create file name");
    }
  }
  if (!logFile.open(fileName, O_CREAT | O_WRITE | O_EXCL)) {
    error("file.open");
    return false;
  }
  Serial.print("Writing "); Serial.println(fileName);
  return true;
}

void setup() {
  // for Leonardos, if you want to debug SD issues, uncomment this line
  // to see serial output
  //while (!Serial);
  
  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  Serial.println("\r\nUltimate GPSlogger Shield");
  pinMode(ledPin, OUTPUT);

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

 // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  if (!sd.begin(chipSelect, SD_SCK_MHZ(50))) {
    sd.initErrorHalt();
  }

  if (!openFile()){
    exit(-1);
  }

  // connect to the GPS at the desired rate
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);

  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For logging data, we don't suggest using anything but either RMC only or RMC+GGA
  // to keep the log files at a reasonable size
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 or 5 Hz update rate

  // Turn off updates on antenna status, if the firmware permits it
  //GPS.sendCommand(PGCMD_NOANTENNA);
  
  Serial.println("Ready!");
}

void loop() {
  char c = GPS.read();
  if (GPSECHO)
     if (c)   Serial.print(c);

  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
      // this also sets the newNMEAreceived() flag to false
    char *stringptr = GPS.lastNMEA();
    uint8_t stringsize = strlen(stringptr);
    if (!GPS.parse(stringptr))   
      return;  // we can fail to parse a sentence in which case we should just wait for another
    //Serial.println(stringptr); 
    // Sentence parsed! 
//    if (LOG_FIXONLY && !GPS.fix) {
//        Serial.print("No Fix");
//        return;
//    } 
 
    if (stringsize != logFile.write((uint8_t *)stringptr, stringsize))    //write the string to the SD file
      error(4);
    if (strstr(stringptr, "RMC"))   logFile.flush();
    //Serial.println();
  }
}


/* End code */
