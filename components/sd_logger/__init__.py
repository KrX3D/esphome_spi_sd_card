import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID, CONF_FRAMEWORK

DEPENDENCIES = ["spi"]

sd_logger_ns = cg.esphome_ns.namespace("sd_logger")
SDLogger = sd_logger_ns.class_(
    "SDLogger", cg.Component, spi.SPIDevice
)

CONFIG_SCHEMA = (
    cv.Schema({cv.GenerateID(): cv.declare_id(SDLogger)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
    
    # For Arduino frameworks, we need to be more specific about which SD library to use
    if CONF_FRAMEWORK not in cg.CORE.config or cg.CORE.config[CONF_FRAMEWORK] == "arduino":
        # Specify a specific SD library with owner to avoid conflicts
        cg.add_library('arduino-libraries/SD', '1.3.0')
        # The FS library is part of the ESP32 Arduino core, not a separate library
        cg.add_define("USE_ARDUINO")
    else:
        cg.add_define("USE_ESP_IDF")
