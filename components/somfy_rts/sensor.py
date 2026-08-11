import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import somfy_rts_ns

SomfyRTSCover = somfy_rts_ns.class_("SomfyRTSCover")
SomfyRTSRollingCodeSensor = somfy_rts_ns.class_(
    "SomfyRTSRollingCodeSensor", sensor.Sensor, cg.Component
)

CONF_COVER_ID = "cover_id"

CONFIG_SCHEMA = sensor.sensor_schema(
    SomfyRTSRollingCodeSensor,
    unit_of_measurement="",
    accuracy_decimals=0,
).extend(
    {
        cv.Required(CONF_COVER_ID): cv.use_id(SomfyRTSCover),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    cover = await cg.get_variable(config[CONF_COVER_ID])
    var = cg.new_Pvariable(config[CONF_ID], cover)
    await sensor.register_sensor(var, config)
    await cg.register_component(var, config)
