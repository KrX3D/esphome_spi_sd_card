import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID, CONF_FRAMEWORK

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
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
    
    # Use the explicitly provided framework option
    if config[CONF_EXPLICIT_FRAMEWORK] == FRAMEWORK_ARDUINO:
        cg.add_library('arduino-libraries/SD', '1.3.0')
        cg.add_define("SD_LOGGER_USE_ARDUINO", "1")
    else:  # ESP-IDF
        cg.add_define("SD_LOGGER_USE_ESP_IDF", "1")
