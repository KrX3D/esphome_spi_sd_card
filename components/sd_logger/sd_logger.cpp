#include "sd_logger.h"
#include <cstring>
#include "esphome/core/log.h"
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

namespace esphome {
namespace sd_logger {

static const char *TAG = "sd_logger.component";

#ifdef USE_ESP_IDF
  static const char *MOUNT_POINT = "/sdcard";
#endif

void SDLogger::setup() {
  ESP_LOGI(TAG, "Initializing SD card...");
  
#ifdef USE_ESP_IDF
  // ESP-IDF Implementierung
  ESP_LOGI(TAG, "Using ESP-IDF framework");
  
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_cs = static_cast<gpio_num_t>(spi::Utility::get_pin_no(this->cs_));
  slot_config.gpio_miso = static_cast<gpio_num_t>(spi::Utility::get_pin_no(this->miso_));
  slot_config.gpio_mosi = static_cast<gpio_num_t>(spi::Utility::get_pin_no(this->mosi_));
  slot_config.gpio_sck = static_cast<gpio_num_t>(spi::Utility::get_pin_no(this->clk_));

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };

  esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card_);

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
  this->card_type_ = card_->card_type;
  
  ESP_LOGI(TAG, "SD Card mounted successfully");
  ESP_LOGI(TAG, "Card info:");
  ESP_LOGI(TAG, "Name: %s", card_->cid.name);
  ESP_LOGI(TAG, "Type: %s", getCardType());
  ESP_LOGI(TAG, "Capacity: %lluMB", ((uint64_t) card_->csd.capacity) * card_->csd.sector_size / (1024 * 1024));
  
#else
  // Arduino Framework Implementierung
  ESP_LOGI(TAG, "Using Arduino framework");
  
  if (!SD.begin(spi::Utility::get_pin_no(this->cs_))) {
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
  // Nichts zu tun in der Loop
}

void SDLogger::dump_config() { 
  ESP_LOGCONFIG(TAG, "SD Logger:");
  ESP_LOGCONFIG(TAG, "  Card Mounted: %s", this->card_mounted_ ? "YES" : "NO");
  if (this->card_mounted_) {
    ESP_LOGCONFIG(TAG, "  Card Type: %s", getCardType());
    ESP_LOGCONFIG(TAG, "  Total Space: %lluMB", getTotalBytes() / (1024 * 1024));
    ESP_LOGCONFIG(TAG, "  Free Space: %lluMB", getFreeBytes() / (1024 * 1024));
  }
}

void SDLogger::writeFile(const char *filename, const char *message) {
  if (!this->card_mounted_) {
    ESP_LOGE(TAG, "SD card not mounted");
    return;
  }

  size_t message_size = strlen(message);
  if (!hasEnoughSpace(message_size)) {
    ESP_LOGE(TAG, "Not enough space on SD card for writing %d bytes", message_size);
    return;
  }

#ifdef USE_ESP_IDF
  char full_path[128];
  snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, createFilename(filename));
  
  ESP_LOGI(TAG, "Writing file: %s", full_path);
  
  FILE *f = fopen(full_path, "w");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return;
  }
  
  fprintf(f, "%s", message);
  fclose(f);
#else
  char *path = createFilename(filename);
  ESP_LOGI(TAG, "Writing file: %s", path);
  
  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return;
  }
  
  if (file.print(message)) {
    ESP_LOGI(TAG, "File written");
  } else {
    ESP_LOGE(TAG, "Write failed");
  }
  file.close();
  delete[] path;
#endif

  ESP_LOGI(TAG, "File written");
}

void SDLogger::appendFile(const char *filename, const char *message) {
  if (!this->card_mounted_) {
    ESP_LOGE(TAG, "SD card not mounted");
    return;
  }

  size_t message_size = strlen(message);
  if (!hasEnoughSpace(message_size)) {
    ESP_LOGE(TAG, "Not enough space on SD card for appending %d bytes", message_size);
    return;
  }

#ifdef USE_ESP_IDF
  char full_path[128];
  snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, createFilename(filename));
  
  ESP_LOGI(TAG, "Appending to file: %s", full_path);
  
  FILE *f = fopen(full_path, "a");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open file for appending");
    return;
  }
  
  fprintf(f, "%s", message);
  fclose(f);
#else
  char *path = createFilename(filename);
  ESP_LOGI(TAG, "Appending to file: %s", path);
  
  File file = SD.open(path, FILE_APPEND);
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
  delete[] path;
#endif

  ESP_LOGI(TAG, "Message appended");
}

char *SDLogger::createFilename(const char *filename) {
  if (filename[0] == '/') {
    size_t len = strlen(filename);
    char *result = new char[len + 1];
    strcpy(result, filename);
    return result;
  }
  
  char prepend_char = '/';
  size_t len = strlen(filename);
  size_t new_len = len + 1;
  char *result = new char[new_len + 1];
  result[0] = prepend_char;
  strcpy(result + 1, filename);
  return result;
}

char *SDLogger::getFirstFileFilename(const char *dir) {
  if (!this->card_mounted_) {
    ESP_LOGE(TAG, "SD card not mounted");
    char *result = new char[6];
    return strcpy(result, "false");
  }

#ifdef USE_ESP_IDF
  char full_path[128];
  snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, dir[0] == '/' ? dir : createFilename(dir));
  
  DIR *dp = opendir(full_path);
  if (dp == NULL) {
    ESP_LOGE(TAG, "Failed to open directory %s", full_path);
    char *result = new char[6];
    return strcpy(result, "false");
  }

  struct dirent *ep;
  char *filename = nullptr;
  
  while ((ep = readdir(dp)) != NULL) {
    if (ep->d_type == DT_REG) {  // Regular file
      filename = new char[strlen(ep->d_name) + 2];
      sprintf(filename, "/%s", ep->d_name);
      break;
    }
  }
  
  closedir(dp);
  
  if (filename == nullptr) {
    char *result = new char[6];
    return strcpy(result, "false");
  }
  
  return filename;
#else
  char *path = createFilename(dir);
  
  File root = SD.open(path);
  if (!root || !root.isDirectory()) {
    ESP_LOGE(TAG, "Failed to open directory");
    delete[] path;
    char *result = new char[6];
    return strcpy(result, "false");
  }
  
  File entry = root.openNextFile();
  while (entry && entry.isDirectory()) {
    entry.close();
    entry = root.openNextFile();
  }
  
  if (!entry) {
    ESP_LOGE(TAG, "No files found");
    root.close();
    delete[] path;
    char *result = new char[6];
    return strcpy(result, "false");
  }
  
  const char *name = entry.name();
  char *filename = new char[strlen(name) + 2];
  sprintf(filename, "/%s", name);
  
  root.close();
  entry.close();
  delete[] path;
  
  return filename;
#endif
}

char *SDLogger::readFile(const char *filename) {
  if (!this->card_mounted_) {
    ESP_LOGE(TAG, "SD card not mounted");
    char *result = new char[6];
    return strcpy(result, "false");
  }

#ifdef USE_ESP_IDF
  char full_path[128];
  snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, createFilename(filename));
  
  ESP_LOGI(TAG, "Reading file: %s", full_path);
  
  FILE *f = fopen(full_path, "r");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    char *result = new char[6];
    return strcpy(result, "false");
  }
  
  // Get file size
  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *buffer = new char[file_size + 1];
  size_t bytes_read = fread(buffer, 1, file_size, f);
  buffer[bytes_read] = '\0';
  
  fclose(f);
  return buffer;
#else
  char *path = createFilename(filename);
  
  if (SD.exists(path)) {
    File file = SD.open(path);
    if (file) {
      size_t size = file.size();
      char *content = new char[size + 1];
      size_t idx = 0;
      
      while (file.available()) {
        char c = file.read();
        if (isPrintable(c)) {
          content[idx++] = c;
        }
      }
      content[idx] = '\0';
      
      file.close();
      delete[] path;
      return content;
    }
  }
  
  delete[] path;
  char *result = new char[6];
  return strcpy(result, "false");
#endif
}

void SDLogger::deleteFile(const char *filename) {
  if (!this->card_mounted_) {
    ESP_LOGE(TAG, "SD card not mounted");
    return;
  }

#ifdef USE_ESP_IDF
  char full_path[128];
  snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, createFilename(filename));
  
  ESP_LOGI(TAG, "Deleting file: %s", full_path);
  
  if (unlink(full_path) == 0) {
    ESP_LOGI(TAG, "File deleted");
  } else {
    ESP_LOGE(TAG, "Error deleting file: %d", errno);
  }
#else
  char *path = createFilename(filename);
  
  ESP_LOGI(TAG, "Deleting file: %s", path);
  
  if (SD.remove(path)) {
    ESP_LOGI(TAG, "File deleted");
  } else {
    ESP_LOGE(TAG, "Delete failed");
  }
  
  delete[] path;
#endif
}

uint64_t SDLogger::getTotalBytes() {
  if (!this->card_mounted_) {
    return 0;
  }
  
#ifdef USE_ESP_IDF
  FATFS *fs;
  DWORD fre_clust, fre_sect, tot_sect;
  uint64_t total_bytes = 0;
  
  // Get volume information and free clusters
  if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
    // Get total sectors and sectors per cluster
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    total_bytes = (uint64_t)tot_sect * 512;  // 512 bytes per sector
  }
  
  return total_bytes;
#else
  return SD.cardSize();
#endif
}

uint64_t SDLogger::getUsedBytes() {
  if (!this->card_mounted_) {
    return 0;
  }
  
  uint64_t total = getTotalBytes();
  uint64_t free = getFreeBytes();
  return total - free;
}

uint64_t SDLogger::getFreeBytes() {
  if (!this->card_mounted_) {
    return 0;
  }
  
#ifdef USE_ESP_IDF
  FATFS *fs;
  DWORD fre_clust, fre_sect;
  uint64_t free_bytes = 0;
  
  // Get volume information and free clusters
  if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
    // Get free sectors
    fre_sect = fre_clust * fs->csize;
    free_bytes = (uint64_t)fre_sect * 512;  // 512 bytes per sector
  }
  
  return free_bytes;
#else
  return SD.totalBytes() - SD.usedBytes();
#endif
}

const char* SDLogger::getCardType() {
  if (!this->card_mounted_) {
    return "NO CARD";
  }
  
  switch (this->card_type_) {
#ifdef USE_ESP_IDF
    case CARD_TYPE_SD:
      return "SD";
    case CARD_TYPE_SDHC:
      return "SDHC";
    case CARD_TYPE_SDXC:
      return "SDXC";
    case CARD_TYPE_MMC:
      return "MMC";
#else
    case CARD_SD:
      return "SD";
    case CARD_SDHC:
      return "SDHC";
    case CARD_SDXC:
      return "SDXC";
    case CARD_MMC:
      return "MMC";
#endif
    default:
      return "UNKNOWN";
  }
}

const char* SDLogger::getFileSystemType() {
  if (!this->card_mounted_) {
    return "NONE";
  }
  
#ifdef USE_ESP_IDF
  FATFS *fs;
  DWORD fre_clust;
  
  if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
    switch (fs->fs_type) {
      case FS_FAT12:
        return "FAT12";
      case FS_FAT16:
        return "FAT16";
      case FS_FAT32:
        return "FAT32";
      case FS_EXFAT:
        return "EXFAT";
      default:
        return "UNKNOWN";
    }
  }
#else
  // Arduino SD-Bibliothek bietet keine direkte Methode zum Abrufen des Dateisystemtyps
  uint64_t card_size = SD.cardSize();
  if (card_size < 4ULL * 1024 * 1024 * 1024) {  // < 4GB
    return "FAT16/FAT32";
  } else {
    return "EXFAT";
  }
#endif
  
  return "UNKNOWN";
}

bool SDLogger::formatCard(const char* type) {
  if (!this->card_mounted_) {
    ESP_LOGE(TAG, "SD card not mounted");
    return false;
  }
  
#ifdef USE_ESP_IDF
  // Unmount before formatting
  esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card_);
  this->card_mounted_ = false;
  
  // Remount with format option
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_cs = static_cast<gpio_num_t>(spi::Utility::get_pin_no(this->cs_));
  slot_config.gpio_miso = static_cast<gpio_num_t>(spi::Utility::get_pin_no(this->miso_));
  slot_config.gpio_mosi = static_cast<gpio_num_t>(spi::Utility::get_pin_no(this->mosi_));
  slot_config.gpio_sck = static_cast<gpio_num_t>(spi::Utility::get_pin_no(this->clk_));
  
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = true,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };
  
  esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card_);
  
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to format and mount filesystem (%s)", esp_err_to_name(ret));
    return false;
  }
  
  this->card_mounted_ = true;
  this->card_type_ = card_->card_type;
  
  ESP_LOGI(TAG, "SD Card formatted and mounted successfully");
  return true;
#else
  // Arduino Framework bietet keine direkte Methode zum Formatieren an
  ESP_LOGE(TAG, "Card formatting not supported in Arduino framework");
  return false;
#endif
}

bool SDLogger::createDirectory(const char *path) {
  if (!this->card_mounted_) {
    ESP_LOGE(TAG, "SD card not mounted");
    return false;
  }
  
#ifdef USE_ESP_IDF
  char full_path[128];
  snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, createFilename(path));
  
  if (mkdir(full_path, 0755) == 0) {
    ESP_LOGI(TAG, "Directory created: %s", full_path);
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to create directory: %s (error %d)", full_path, errno);
    return false;
  }
#else
  char *dir_path = createFilename(path);
  
  if (SD.mkdir(dir_path)) {
    ESP_LOGI(TAG, "Directory created: %s", dir_path);
    delete[] dir_path;
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to create directory: %s", dir_path);
    delete[] dir_path;
    return false;
  }
#endif
}

bool SDLogger::removeDirectory(const char *path) {
  if (!this->card_mounted_) {
    ESP_LOGE(TAG, "SD card not mounted");
    return false;
  }
  
#ifdef USE_ESP_IDF
  char full_path[128];
  snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, createFilename(path));
  
  if (rmdir(full_path) == 0) {
    ESP_LOGI(TAG, "Directory removed: %s", full_path);
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to remove directory: %s (error %d)", full_path, errno);
    return false;
  }
#else
  char *dir_path = createFilename(path);
  
  if (SD.rmdir(dir_path)) {
    ESP_LOGI(TAG, "Directory removed: %s", dir_path);
    delete[] dir_path;
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to remove directory: %s", dir_path);
    delete[] dir_path;
    return false;
  }
#endif
}

bool SDLogger::fileExists(const char *filename) {
  if (!this->card_mounted_) {
    return false;
  }
  
#ifdef USE_ESP_IDF
  char full_path[128];
  snprintf(full_path, sizeof(full_path), "%s%s", MOUNT_POINT, createFilename(filename));
  
  struct stat st;
  return stat(full_path, &st) == 0;
#else
  char *file_path = createFilename(filename);
  bool exists = SD.exists(file_path);
  delete[] file_path;
  return exists;
#endif
}

bool SDLogger::hasEnoughSpace(size_t size) {
  if (!this->card_mounted_) {
    return false;
  }
  
  return getFreeBytes() >= size;
}

}  // namespace sd_logger
}  // namespace esphome
