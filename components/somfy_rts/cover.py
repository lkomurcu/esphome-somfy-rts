import esphome.codegen as cg
from esphome.components import cover
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import SomfyRTSHub, somfy_rts_ns

SomfyRTSCover = somfy_rts_ns.class_("SomfyRTSCover", cover.Cover, cg.Component)

CONF_SOMFY_RTS_ID = "somfy_rts_id"
CONF_REMOTE_CODE = "remote_code"
CONF_STORAGE_NAME = "storage_name"
CONF_STORAGE_KEY = "storage_key"
CONF_OPEN_DURATION = "open_duration"
CONF_CLOSE_DURATION = "close_duration"
CONF_INVERT_POSITION = "invert_position"
CONF_INVERT_DIRECTION = "invert_direction"

CONFIG_SCHEMA = cover.cover_schema(
    SomfyRTSCover,
    device_class="shutter",
).extend(cv.Schema(
    {
        cv.GenerateID(CONF_SOMFY_RTS_ID): cv.use_id(SomfyRTSHub),
        cv.Required(CONF_REMOTE_CODE): cv.hex_uint32_t,
        cv.Required(CONF_STORAGE_NAME): cv.string_strict,
        cv.Required(CONF_STORAGE_KEY): cv.string_strict,
        cv.Optional(CONF_INVERT_POSITION, default=False): cv.boolean,
        cv.Optional(CONF_INVERT_DIRECTION, default=False): cv.boolean,
        cv.Optional(CONF_OPEN_DURATION, default="30s"): cv.time_period_microseconds,
        cv.Optional(CONF_CLOSE_DURATION, default="30s"): cv.time_period_microseconds,
    }
))


async def to_code(config):
    hub = await cg.get_variable(config[CONF_SOMFY_RTS_ID])
    var = await cover.new_cover(
        config,
        hub,
        config[CONF_STORAGE_NAME],
        config[CONF_STORAGE_KEY],
        config[CONF_REMOTE_CODE],
    )
    await cg.register_component(var, config)
    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))
    cg.add(var.set_invert_position(config[CONF_INVERT_POSITION]))
    cg.add(var.set_invert_direction(config[CONF_INVERT_DIRECTION]))
