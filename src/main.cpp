#include <Arduino.h>

#include "FS.h"
#include "SD.h"
#include "PSRamFS.h"


uint8_t initSDcard(){
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return 0;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return 0;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);


  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
  return 1;
}


void writeFile(fs::FS &fs, const char * path, const char * message){
  delay(100);

  File file = fs.open(path, FILE_WRITE);

  if(!file){
    Serial.printf("Failed to open file %s for writing\n", path);
    return;
  } else {
    Serial.printf("Truncated file: %s\n", path);
  }

  if( file.write( (const uint8_t*)message, strlen(message)+1 ) ) {
    Serial.println("- data written");
  } else {
    Serial.println("- write failed\n");
  }
  file.close();
  Serial.printf("Write done: %s\n", path);

}


// append data to SD card
void appendFile(fs::FS &fs, const char * path, const char * message){
  delay(100);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("- failed to open file for appending");
    return;
  }
  if( file.write( (const uint8_t*)message, strlen(message)+1 ) ){
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed\n");
  }
  file.close();
}


void readFile(fs::FS &fs, const char * path){
  delay(100);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.printf("Failed to open file for reading : %s", path);
    return;
  }

  if( !file.available() ) {
    Serial.printf("CANNOT read from file: %s", path);
  } else {
    Serial.printf("Will read from file: %s", path);

    delay(100);
    Serial.println();

    int32_t lastPosition = -1;
    while( file.available() ) {
      size_t position = file.position();
      char a = file.read();
      Serial.write( a );
      if( lastPosition == position ) { // uh-oh
        Serial.println("Halting");
        while(1);
        break;
      } else {
        lastPosition = position;
      }
    }
  }
  Serial.println("\n");
  file.close();
  Serial.printf("Read done: %s", path);
}


void copyFile(fs::FS &fsPsram,fs::FS &fsSd, const char * path) {
  delay(100);

  File filePsram = PSRamFS.open(path, "r+");

  if(!filePsram || filePsram.isDirectory()){
    Serial.printf("FAILED to open psram file: %s \n", path);
    return;
  }

  if( !filePsram.available() ) {
    Serial.printf("CANNOT read from psram file: %s \n", path);
  } else {
    Serial.printf("Will read from psram file: %s \n", path);

    delay(100);
    Serial.println();

    File fileSd = SD.open(path, "a"); // TODO take a look

    if(!fileSd || fileSd.isDirectory()){
      Serial.printf("FAILED to open SD file for appending: %s \n", path);
      return;
    } else if (!fileSd.available()) {
      Serial.printf("CANNOT open SD file: %s \n", path);
    } else {
      int32_t lastPosition = -1;
      while(filePsram.available()) {
        size_t position = filePsram.position();
        char a = filePsram.read();
        if(!fileSd.write(a)){
          return;
        }
        if( lastPosition == position ) { // uh-oh
          Serial.println("Halting");
          while(1);
          break;
        } else {
          lastPosition = position;
        }
      }
      Serial.println("Copying finished");
      filePsram.close();
      fileSd.close();
    }
  }
}


void logMemory() {
  log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());
}


//
//
/* TASK */ /* TASK */ /* TASK */
/* TASK */ /* TASK */ /* TASK */
/* TASK */ /* TASK */ /* TASK */
//
//


String FileName = String("/Frekans_") + random(30) + ".txt";

void mainLoopTask(fs::FS &fs){
  String dataMessage;
  for(int i = 0; i<60; i++){
    dataMessage = String(random(100, 500)) + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "||" + random(100, 500) + "||" + random(100, 500)
                                           + "||" + random(100, 500) + "\r\n";
    appendFile(PSRamFS, FileName.c_str(), dataMessage.c_str()); // PSRamFS instead of SD
    delay(100);
  }
}


//
//
/* SETUP */ /* SETUP */ /* SETUP */
/* SETUP */ /* SETUP */ /* SETUP */
/* SETUP */ /* SETUP */ /* SETUP */
//
//


void setup(){
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  Serial.println();

  if(!initSDcard()) return; // sd card vers

  if(!PSRamFS.begin()){
    Serial.println("PSRamFS Mount Failed");
    return;
  }

  Serial.println(FileName);
  File file = PSRamFS.open(FileName, "r+"); // PSRamFS instead of SD
  if(!file){
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
    String header = "DateAndTime,LA,LC,LZ,LAmax,LAmin,LCmax,LCmin,LZmax,LZmin,LApeak,LCpeak,LZpeak,runnA,runnC,runnZ,20,25,32,40,50,63,80,100,125,160,200,250,316,400,100, 500,630,800,1000,1250,1584,2000,2100, 500,3162,4000,100, 5000,6300,8000,10000,12100, 500,15840,20000 \r\n";
    Serial.println(header);
    writeFile(PSRamFS, FileName.c_str(), header.c_str()); // PSRamFS instead of SD
  } else{
    Serial.println("File already exists");
  }
  file.close();

  mainLoopTask(PSRamFS);
  logMemory();

  Serial.print("\r\n\r\n------------------------\r\n\r\n-------main file-------\r\n\r\n------------------------\r\n\r\n");
  readFile(PSRamFS, FileName.c_str());

  Serial.print("\r\n\r\n-----------------------\r\n\r\n--------copying--------\r\n\r\n-----------------------\r\n\r\n");
  copyFile(PSRamFS, SD, FileName.c_str());

  Serial.print("\r\n\r\n---------------------------\r\n\r\n--------copied file--------\r\n\r\n---------------------------\r\n\r\n");
  readFile(SD, FileName.c_str());
}

void loop(){
  // ahmet
}