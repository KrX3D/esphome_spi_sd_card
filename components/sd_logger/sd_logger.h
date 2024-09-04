#pragma once

#include "esphome/components/spi/spi.h"
#include "esphome/core/component.h"

namespace esphome {
namespace sd_logger {

class SDLogger : public Component,
                 public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                                       spi::DATA_RATE_1KHZ> {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void appendFile(const char *filename, const char *message);
  void writeFile(const char *filename, const char *message);
  char *getFirstFileFilename(const char *dir = "/");
  char *readFile(const char *filename);
  void deleteFile(const char *filename);

 protected:
  char *createFilename(const char *filename);
};

}  // namespace sd_logger
}  // namespace esphome
