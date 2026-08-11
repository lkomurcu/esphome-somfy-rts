import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import somfy_rts_ns, SomfyRTSHub

SomfyRTSCover = somfy_rts_ns.class_("SomfyRTSCover")
SomfyRTSButton = somfy_rts_ns.class_("SomfyRTSButton", button.Button, cg.Component)

CONF_COVER_ID = "cover_id"

CONFIG_SCHEMA = button.button_schema(SomfyRTSButton).extend(
    {
        cv.Required(CONF_COVER_ID): cv.use_id(SomfyRTSCover),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    cover = await cg.get_variable(config[CONF_COVER_ID])
    var = cg.new_Pvariable(config[CONF_ID], cover)
    await button.register_button(var, config)
    await cg.register_component(var, config)
