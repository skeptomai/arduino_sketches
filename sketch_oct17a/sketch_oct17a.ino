#include <Arduino.h>
#include <SPI.h>
//#include <SD.h>
#include <SdFat.h>
#include <NMEAGPS.h>
#include <U8x8lib.h>
#include <GPSport.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);         

// Set the pins used
const int chipSelect=10;
const long serialBaudRate = 115200;
const int gpsBaudRate = 9600;

//static File logFile;
#define FILE_BASE_NAME "Data"
constexpr uint8_t BASE_NAME_SIZE() {return sizeof(FILE_BASE_NAME) - 1;}
char fileName[13] = FILE_BASE_NAME "00.csv";

#define error(msg) sd.errorHalt(F(msg))

// File system object.
SdFat sd;

// Log file.
SdFile logFile;

static NMEAGPS  gps;
static char gpsStr[20];
static char* cc = "Couldn't create ";
static char* wt = "Writing to ";

/* Set the delay between fresh samples */
#define BNO055_SAMPLERATE_DELAY_MS (100)
Adafruit_BNO055 bno = Adafruit_BNO055();

void setupDisplay(){
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r); 
}

void setup() {
  Serial.begin(serialBaudRate);
  
  setupDisplay();
  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  if (!sd.begin(chipSelect, SD_SCK_MHZ(50))) {
    sd.initErrorHalt();
  }

  if (!openFile()){
    exit(-1);
  }

  if(!bno.begin()){
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    exit(-1);
  }

  u8x8.drawString(0,0,fileName);

  gpsPort.begin( gpsBaudRate );
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

char lineEnd[1]="\n";
static void GPSloop()
{
  while (gps.available( gpsPort )) {
    gps_fix fix = gps.read();
    logFile.write("gps,");
    dtostrf(fix.latitude(), 4, 6, gpsStr);
    u8x8.drawString(0,2,gpsStr);
    logFile.write((uint8_t *)gpsStr, strlen(gpsStr));
    logFile.write(",",1);
    dtostrf(fix.longitude(), 4, 6, gpsStr);
    u8x8.drawString(0,3,gpsStr);
    logFile.write((uint8_t *)gpsStr, strlen(gpsStr));
    logFile.write(lineEnd,1);
    logFile.flush();
    AttitudeQuatRead();
  }
}

static void AttitudeQuatRead() {
   /* Display calibration status for each sensor. */
  uint8_t system, gyro, accel, mag = 0;
  bno.getCalibration(&system, &gyro, &accel, &mag);
  Serial.print("CALIBRATION: Sys=");
  Serial.print(system, DEC);
  Serial.print(" Gyro=");
  Serial.print(gyro, DEC);
  Serial.print(" Accel=");
  Serial.print(accel, DEC);
  Serial.print(" Mag=");
  Serial.println(mag, DEC);
  
  imu::Quaternion quat = bno.getQuat();
  logFile.write("quat,");
  dtostrf(quat.w(), 1, 5, gpsStr);
  logFile.write((uint8_t *)gpsStr, strlen(gpsStr));
  u8x8.drawString(0,4,gpsStr);
  logFile.write(",",1);
  
  dtostrf(quat.x(), 1, 5, gpsStr);
  logFile.write((uint8_t *)gpsStr, strlen(gpsStr));
  u8x8.drawString(0,5,gpsStr);
  logFile.write(",",1);
  
  dtostrf(quat.y(), 1, 5, gpsStr);
  logFile.write((uint8_t *)gpsStr, strlen(gpsStr));
  u8x8.drawString(0,6,gpsStr);
  logFile.write(",",1);
  
  dtostrf(quat.z(), 1, 5, gpsStr);
  logFile.write((uint8_t *)gpsStr, strlen(gpsStr));
  u8x8.drawString(0,7,gpsStr);
  logFile.write(lineEnd,1);
  logFile.flush();
}

void loop() {
  GPSloop();
}


//  SD.begin(chipSelect);
//
//  strcpy(filename, "GPSLOG00.TXT");
//  for (uint8_t i = 0; i < 100; i++) {
//    filename[6] = '0' + i/10;
//    filename[7] = '0' + i%10;
//    u8x8.drawString(0,1, "Writing:");
//    u8x8.drawString(0,2,filename);
//
//    // create if does not exist, do not open existing, write, sync after write
//    if (! SD.exists(filename)) {
//      break;
//    }
//  }
//  
//  logFile = SD.open(filename, FILE_WRITE);
//  if( ! logFile ) {
//    logFile.write(cc); logFile.writeln(filename);
//    u8x8.drawString(0,0, cc);
//    exit(-1);
//  } else {
//    logFile.write(wt); logFile.writeln(filename);
//  }

