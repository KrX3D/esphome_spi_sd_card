#pragma once
#include "esphome/components/spi/spi.h"
#include "esphome/core/component.h"

// Determine which includes to use based on the defined framework
#ifdef USE_ESP_IDF
  #include "esp_vfs_fat.h"
  #include "sdmmc_cmd.h"
#else
  // Arduino framework
  #include <SD.h>
  #include <FS.h>
#endif

namespace esphome {
namespace sd_logger {

class SDLogger : public Component,
                 public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                                       spi::DATA_RATE_1KHZ> {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  
  // Dateifunktionen
  void appendFile(const char *filename, const char *message);
  void writeFile(const char *filename, const char *message);
  char *getFirstFileFilename(const char *dir = "/");
  char *readFile(const char *filename);
  void deleteFile(const char *filename);
  
  // SD-Karten-Informationen
  bool isCardMounted() { return this->card_mounted_; }
  uint64_t getTotalBytes();
  uint64_t getUsedBytes();
  uint64_t getFreeBytes();
  const char* getCardType();
  const char* getFileSystemType();
  bool formatCard(const char* type = "FAT");
  bool createDirectory(const char *path);
  bool removeDirectory(const char *path);
  bool fileExists(const char *filename);
  bool hasEnoughSpace(size_t size);

 protected:
  char *createFilename(const char *filename);
  bool card_mounted_{false};
  
#ifdef USE_ESP_IDF
  sdmmc_card_t *card_{nullptr};
#endif

  uint8_t card_type_{0};
};

}  // namespace sd_logger
}  // namespace esphome
