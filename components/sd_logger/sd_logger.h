#pragma once
#include "esphome/components/spi/spi.h"
#include "esphome/core/component.h"

#if defined(SD_LOGGER_USE_ESP_IDF)
  #include "esp_vfs_fat.h"
  #include "driver/sdmmc_host.h"
  #include "driver/sdspi_host.h"
  #include "sdmmc_cmd.h"
#else // Arduino framework
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
  
  // File functions
  void appendFile(const char *filename, const char *message);
  void writeFile(const char *filename, const char *message);
  char *getFirstFileFilename(const char *dir = "/");
  char *readFile(const char *filename);
  void deleteFile(const char *filename);
  
  // SD card information
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

#if defined(SD_LOGGER_USE_ESP_IDF)
  // Setters for additional SPI pins (needed in ESP-IDF mode)
  void set_mosi_pin(GPIOPin *pin) { this->mosi_pin_ = pin; }
  void set_miso_pin(GPIOPin *pin) { this->miso_pin_ = pin; }
  void set_clk_pin(GPIOPin *pin) { this->clk_pin_ = pin; }
#endif

 protected:
  char *createFilename(const char *filename);
  bool card_mounted_{false};
  
#if defined(SD_LOGGER_USE_ESP_IDF)
  sdmmc_card_t *card_{nullptr};
#endif

  uint8_t card_type_{0};

#if defined(SD_LOGGER_USE_ESP_IDF)
  // Pointers for extra SPI bus pins
  GPIOPin *mosi_pin_{nullptr};
  GPIOPin *miso_pin_{nullptr};
  GPIOPin *clk_pin_{nullptr};
#endif
};

}  // namespace sd_logger
}  // namespace esphome
