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
    
    # Framework-spezifische Optionen hinzufügen
    # Für Arduino
    if CONF_FRAMEWORK not in cg.CORE.config or cg.CORE.config[CONF_FRAMEWORK] == "arduino":
        cg.add_library('SD', None)
        cg.add_define("USE_ARDUINO")
    # Für ESP-IDF
    else:
        cg.add_define("USE_ESP_IDF")
