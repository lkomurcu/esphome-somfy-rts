import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = []
MULTI_CONF = False

somfy_rts_ns = cg.esphome_ns.namespace("somfy_rts")
SomfyRTSHub = somfy_rts_ns.class_("SomfyRTSHub", cg.Component)

CONF_FREQUENCY = "frequency"
CONF_EMITTER_PIN = "emitter_pin"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SomfyRTSHub),
            cv.Optional(CONF_FREQUENCY, default=433.42): cv.float_,
            cv.Optional(CONF_EMITTER_PIN, default=2): cv.int_range(min=0, max=39),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_emitter_pin(config[CONF_EMITTER_PIN]))