import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID, CONF_FRAMEWORK, CONF_MOSI, CONF_MISO, CONF_CLK

DEPENDENCIES = ["spi"]

# Define the framework option
CONF_EXPLICIT_FRAMEWORK = "explicit_framework"
FRAMEWORK_ARDUINO = "arduino"
FRAMEWORK_ESP_IDF = "esp_idf"

sd_logger_ns = cg.esphome_ns.namespace("sd_logger")
SDLogger = sd_logger_ns.class_(
    "SDLogger", cg.Component, spi.SPIDevice
)

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(SDLogger),
        cv.Optional(CONF_EXPLICIT_FRAMEWORK, default=FRAMEWORK_ARDUINO): cv.one_of(
            FRAMEWORK_ARDUINO, FRAMEWORK_ESP_IDF, lower=True
        ),
        # Extend the schema to accept additional SPI pins for ESP-IDF:
        cv.Optional(CONF_MOSI): cv.gpio_output_pin_schema,
        cv.Optional(CONF_MISO): cv.gpio_input_pin_schema,
        cv.Optional(CONF_CLK): cv.gpio_output_pin_schema,
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
    
    # For ESP-IDF, store the additional SPI pins if provided.
    if config[CONF_EXPLICIT_FRAMEWORK] == FRAMEWORK_ESP_IDF:
        if CONF_MOSI in config:
            cg.add(var.set_mosi_pin(cg.new_Pin(config[CONF_MOSI])))
        if CONF_MISO in config:
            cg.add(var.set_miso_pin(cg.new_Pin(config[CONF_MISO])))
        if CONF_CLK in config:
            cg.add(var.set_clk_pin(cg.new_Pin(config[CONF_CLK])))
    
    if config[CONF_EXPLICIT_FRAMEWORK] == FRAMEWORK_ARDUINO:
        cg.add_library('arduino-libraries/SD', '1.3.0')
        cg.add_define("SD_LOGGER_USE_ARDUINO", "1")
    else:  # ESP-IDF
        cg.add_define("SD_LOGGER_USE_ESP_IDF", "1")
