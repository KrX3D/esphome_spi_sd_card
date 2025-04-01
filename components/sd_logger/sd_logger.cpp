#include "sd_logger.h"
#include "esphome/core/log.h"
#include <cstdint>  // For uint8_t and fixed-width types

namespace esphome {
namespace sd_logger {

static const char *TAG = "sd_logger";

void SDLogger::setup() {
  ESP_LOGI(TAG, "Initializing SD card...");

#if defined(SD_LOGGER_USE_ESP_IDF)
  // ESP-IDF Implementation
  ESP_LOGI(TAG, "Using ESP-IDF framework");
  
  // Initialize SPI bus
  spi_bus_config_t bus_cfg = {
      .mosi_io_num = spi::get_pin_no(this->mosi),
      .miso_io_num = spi::get_pin_no(this->miso),
      .sclk_io_num = spi::get_pin_no(this->clk),
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
  };
  
  esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
      return;
  }
  
  // Mount FAT filesystem
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };
  
  // ESP-IDF 4.4+ compatible code
  sdmmc_host_t host = {
      .flags = SDMMC_HOST_FLAG_SPI,
      .slot = SDMMC_HOST_SLOT_1,
      .max_freq_khz = 20000,
      .io_voltage = 3.3f,
      .init = &sdspi_host_init,
      .set_bus_width = NULL,
      .get_bus_width = NULL,
      .set_bus_ddr_mode = NULL,
      .set_card_clk = &sdspi_host_set_card_clk,
      .do_transaction = &sdspi_host_do_transaction,
      .deinit = &sdspi_host_deinit,
      .io_int_enable = NULL,
      .io_int_wait = NULL,
      .command_timeout_ms = 0,
  };
  
  sdspi_device_config_t slot_config = {
      .gpio_cs = static_cast<gpio_num_t>(spi::get_pin_no(this->cs)),
      .host_id = SPI2_HOST,
      .gpio_cd = SDSPI_SLOT_NO_CD,
      .gpio_wp = SDSPI_SLOT_NO_WP,
      .gpio_int = SDSPI_SLOT_NO_INT,
  };
  
  ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card_);
  
  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          ESP_LOGE(TAG, "Failed to mount filesystem.");
      } else {
          ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
      }
      this->card_mounted_ = false;
      return;
  }
  
  this->card_mounted_ = true;
  this->card_type_ = card_->ocr & SD_OCR_SDHC_CAP ? 1 : 0; // SDHC or SD card
  
  ESP_LOGI(TAG, "SD Card mounted successfully");
  
#else
  // Arduino Framework Implementation
  ESP_LOGI(TAG, "Using Arduino framework");
  
  if (!SD.begin(spi::get_pin_no(this->cs))) {
      ESP_LOGE(TAG, "Card Mount Failed");
      this->card_mounted_ = false;
      return;
  }
  
  this->card_mounted_ = true;
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
      ESP_LOGW(TAG, "No SD card attached");
      this->card_mounted_ = false;
      return;
  }
  
  this->card_type_ = cardType;
  
  ESP_LOGI(TAG, "SD Card mounted successfully");
  ESP_LOGI(TAG, "Card Type: %s", getCardType());
  ESP_LOGI(TAG, "Card Size: %lluMB", SD.cardSize() / (1024 * 1024));
#endif
}

void SDLogger::loop() {
  // Add loop functionality if required.
}

void SDLogger::dump_config() {
  ESP_LOGCONFIG(TAG, "SD Logger:");
  ESP_LOGCONFIG(TAG, "  Mounted: %s", this->card_mounted_ ? "Yes" : "No");
  // Add additional configuration dump as needed.
}

// Additional member function implementations would follow here...
// For example: appendFile, writeFile, readFile, deleteFile, etc.

}  // namespace sd_logger
}  // namespace esphome
