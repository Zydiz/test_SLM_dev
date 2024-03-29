#include <Arduino.h>
#include <WiFi.h>

#include "FS.h"
#include "PSRamFS.h"
#include "SD.h"

#define BUFSIZE 1024

const char *ssid = "SSID";
const char *password = "PASSWORD";

const char *httpHost = "c12edaf5-27c4-441a-9bbf-8587e5c8471c.mock.pstmn.io";
const char *httpPath = "/post_test";
const char *deviceId = "123456";

uint8_t initSDcard() {
  if (!SD.begin(5)) {
    Serial.println("Card Mount Failed");
    return 0;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return 0;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
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

void writeFile(fs::FS &fs, const char *path, const char *message) {
  delay(100);

  File file = fs.open(path, FILE_WRITE);

  if (!file) {
    Serial.printf("Failed to open file %s for writing\n", path);
    return;
  } else {
    Serial.printf("Truncated file: %s\n", path);
  }

  if (file.write((const uint8_t *)message, strlen(message) + 1)) {
    Serial.println("- data written");
  } else {
    Serial.println("- write failed\n");
  }
  file.close();
  Serial.printf("Write done: %s\n", path);
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  delay(100);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.write((const uint8_t *)message, strlen(message) + 1)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed\n");
  }
  file.close();
}

void readFile(fs::FS &fs, const char *path) {
  delay(100);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.printf("Failed to open file for reading : %s", path);
    return;
  }

  if (!file.available()) {
    Serial.printf("CANNOT read from file: %s", path);
  } else {
    Serial.printf("Will read from file: %s", path);

    delay(100);
    Serial.println();

    int32_t lastPosition = -1;
    while (file.available()) {
      size_t position = file.position();
      char a = file.read();
      Serial.write(a);
      if (lastPosition == position) {  // uh-oh
        Serial.println("Halting");
        while (1);
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

bool readPost(fs::FS &fs, const char* path) {
  bool bFlag;
  log_i("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    log_e("Failed to open file for reading");
    return false;
  }

  size_t fLen = file.available();
  log_d("%d", fLen);
  if (!fLen) return false; // HACK TESTING

  char* fileBuffer = new char[BUFSIZE + 1];
  size_t lines = 0;
  char response[64];

  while (file.available()) {
    char c = file.read();
    if (c == '\n') {
      lines++;
    }
  }
  file.seek(0);

  size_t contLen =  fLen + 31 + 15 + strlen(file.name()) - lines + 1 ; // TODO some gibberish at the end?

  WiFiClient client;

  log_i("\nStarting connection to server...");
  int8_t retry_c = 0;
  while (!client.connect(httpHost, 80)) {
    if (++retry_c > 10) {
      log_e("Connection failed!");
      return false;
    }
  }
  // if (!client.connect(httpHost, 80)) {
  //   log_e("Connection failed!");
  //   return false;
  // } else {
  log_i("Connected to server!");
  client.println();
  client.println();
  client.println(String("POST ") + httpPath + " HTTP/1.1");
  // client.printf("POST %s HTTP/1.1\r\n", httpPath);

  // Make a HTTP request
  client.println(String("Host: ") + httpHost);
  // client.printf("Host: %s\r\n", httpHost);
  client.println("Connection: close");
  client.println("Content-Type: application/json");
  // client.printf("Content-Length: %d", contLen);
  client.println(String("Content-Length: ") + contLen);
  client.println();

  client.print(String("{\"DeviceId\":\"") + deviceId + "\",");
  client.print(String("\"FileName\":\"") + file.name() + "\",");
  // client.printf("{\"DeviceId\":\"%s\",", deviceId);
  // client.printf("\"FileName\":\"%s\",", file.name());
  client.print("\"Data\":\"");

  // TODO check for 0 size writes
  uint8_t skip = 0;
  size_t written = 0;
  while (file.available()) {
    size_t toRead = file.available();
    if (toRead > BUFSIZE) {
      toRead = BUFSIZE;
    }
    for (uint16_t i = 0; i < toRead; i++) {
      if (skip == 0) {
        fileBuffer[i] = file.read();
        if (fileBuffer[i] == '\r') {
          if (file.peek() == '\n') {
            skip = 1;
            fileBuffer[i] = '*';
          }
        }
      } else {
        skip = 0;
        i -= 1;
        file.read();
      }
    }
    written = client.write((const uint8_t*)fileBuffer, toRead);
    log_d("%d", written);
    delay(100);
  }
  client.print("\"}");
  client.print("\r\n");

  for (uint8_t i = 0; (!client.available()) && (i < 1000); i++) {
    delay(1);
  }

  uint8_t i = 0;
  while (client.available()) {
    char c = client.read();
    response[i] = c;
    if (c == '\r') {
      response[i] = '\0';
      i = 0;
      Serial.println(response);
      if (strcmp(response, "HTTP/1.1 200 OK")) {
        if (strcmp(response, "HTTP/1.1 409 Conflict")) {
          Serial.println("Response status is NOT \"200 OK\" OR \"409 Conflict\"");
          bFlag = false;
          break;
        } else {
        // TODO check for new config
          bFlag = true;
          break;
        }
      } else {
        // TODO check for new config
        bFlag = true;
        break;
      }
    }
    i++;
  }
  Serial.println();

  client.stop();
  delete[] fileBuffer;
  file.close();
  return bFlag;
}
void copyFile(fs::FS& fs1, fs::FS& fs2, const char* path) {
  // delay(100);
  File file1 = fs1.open(path, "r+");

  if (!file1 || file1.isDirectory()) {
    log_e("FAILED to open file 1: %s \n", path);
    return;
  } else if (!file1.available()) {
    log_e("CANNOT read from file 1: %s \n", path);
    return;
  } else {
    // log_i("Will read from file 1: %s \n", path);

    delay(100);
    Serial.println();

    File file2 = fs2.open(path, "a");

    if (!file2 || file2.isDirectory()) {
      log_e("FAILED to open file 2 for appending: %s \n", path);
      return;
    } else {
      const size_t bufferSize = 512;
      uint8_t buffer[bufferSize]; // create a buffer array of size 512
      while (file1.available()) {
        size_t n = file1.read(buffer, bufferSize); // read up to bytesToRead bytes from file1 into buffer
        if (!file2.write(buffer, n)) { // write the bytes from buffer to file2
          return;
        }
      }
      log_i("Copying finished");
    }

    file1.close();
    file2.close();
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  delay(100);
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
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
const char *oldFile = "/Frekans_0.txt";

void mainLoopTask(fs::FS &fs, const char *path) {
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


void setup() {
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  Serial.println();

  if(!initSDcard()) return; // sd card vers

  if(!PSRamFS.begin()){
    Serial.println("PSRamFS Mount Failed");
    return;
  }
  delay(500);

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();

  Serial.println(FileName);
  File file = SD.open(FileName, "r+");

  if (!file) {
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
    String header = "DateAndTime,LA,LC,LZ,LAmax,LAmin,LCmax,LCmin,LZmax,LZmin,LApeak,LCpeak,LZpeak,runnA,runnC,runnZ,20,25,32,40,50,63,80,100,125,160,200,250,316,400,500,630,800,1000,1250,1584,2000,2500,3162,4000,5000,6300,8000,10000,12500,15840,20000 \r\n";
    Serial.println(header);
    writeFile(SD, FileName.c_str(), header.c_str());
  } else {
    Serial.println("File already exists");
  }
  file.close();

  // mainLoopTask(SD, FileName.c_str());

  Serial.print("\r\n\r\n------------------------\r\n\r\n-------main file-------\r\n\r\n------------------------\r\n\r\n");
  // readFile(SD, FileName.c_str());
  readFile(SD, "/test.txt");

  Serial.print("\r\n\r\n---------------------------\r\n\r\n--------post file--------\r\n\r\n---------------------------\r\n\r\n"); 
  // readPost(SD, FileName.c_str());
  readPost(SD, "/test.txt");

}

void loop() {
  // ahmet
}
