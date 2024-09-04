#include "sd_logger.h"
#include <cstring>
#include "esphome/core/log.h"
#include "FS.h"
#include "SD.h"

namespace esphome {
namespace sd_logger {

static const char *TAG = "sd_logger.component";

void SDLogger::setup() {
  LOG_PIN("CS Pin: ", this->cs_);
  /*SD.begin(spi::Utility::get_pin_no(this->cs_));*/
  if (!SD.begin(spi::Utility::get_pin_no(this->cs_))) {
    ESP_LOGE(TAG, "Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    ESP_LOGW(TAG, "No SD card attached");
    return;
  }
  ESP_LOGI(TAG, "Initializing SD card...");
  if (!SD.begin(spi::Utility::get_pin_no(this->cs_))) {
    ESP_LOGE(TAG, "ERROR - SD card initialization failed!");
    return;  // init failed
  }
}

void SDLogger::loop() {}

void SDLogger::dump_config() { ESP_LOGCONFIG(TAG, "SD Logger"); }

void SDLogger::writeFile(const char *filename, const char *message) {
  char *result = createFilename(filename);

  ESP_LOGI(TAG, "Writing file: %s", result);

  File file = SD.open(result, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for writing: %s", result);
    return;
  }
  if (file.print(message)) {
    ESP_LOGI(TAG, "File written");
  } else {
    ESP_LOGE(TAG, "Write failed");
  }
  file.close();
}

void SDLogger::appendFile(const char *filename, const char *message) {
  char *result = createFilename(filename);

  ESP_LOGI(TAG, "Appending to file: %s", result);
  File file = SD.open(result, FILE_APPEND);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    ESP_LOGI(TAG, "Message appended");
  } else {
    ESP_LOGE(TAG, "Append failed");
  }
  file.close();
}

char *SDLogger::createFilename(const char *filename) {
  char prepend_char = '/';
  size_t len = strlen(filename);
  size_t new_len = len + 1;
  char *result = new char[new_len + 1];
  result[0] = prepend_char;
  strcpy(result + 1, filename);

  return result;
}

char *SDLogger::getFirstFileFilename(const char *dir) {
  File root = SD.open(dir, FILE_READ);
  if (!root || !root.isDirectory()) {
    ESP_LOGE(TAG, "Failed to open file for read");
    char *result = new char[6];
    return strcpy(result, "false");
  }

  File entry = root.openNextFile();

  while (entry.isDirectory()) {
    entry.close();
    entry = root.openNextFile();
  }

  if (!entry) {
    ESP_LOGE(TAG, "Failed to open file for read");
    char *result = new char[6];
    return strcpy(result, "false");
  }

  const char *name = entry.name();

  root.close();
  entry.close();

  return createFilename(name);
}

char *SDLogger::readFile(const char *filename) {
  char c;
  String parameter;
  if (strncmp(filename, "/", 1) == 0) {
    File file = SD.open(filename);
    if (file) {
      while (file.available()) {
        char c = file.read();
        if (isPrintable(c)) {
          parameter.concat(c);
        }
      }
      ESP_LOGI(TAG, "%S", parameter);

      char *cString = new char[parameter.length() + 1];
      strcpy(cString, parameter.c_str());

      return cString;
    }
    file.close();
  }

  char *result = new char[6];
  return strcpy(result, "false");
}

void SDLogger::deleteFile(const char *filename) {
  if (strncmp(filename, "/", 1) == 0) {
    File file = SD.open(filename);

    if (file) {
      SD.remove(filename);
      if (!SD.exists(filename)) {
        ESP_LOGI(TAG, "File deleted.");
      }
    }
  }
}

}  // namespace sd_logger
}  // namespace esphome
