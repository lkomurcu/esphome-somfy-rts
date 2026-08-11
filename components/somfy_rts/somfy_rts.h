#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/button/button.h"

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <NVSRollingCodeStorage.h>
#include <SomfyRemote.h>

namespace esphome {
namespace somfy_rts {

class SomfyRTSCover;

class SomfyRTSHub : public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_frequency(float frequency) { this->frequency_ = frequency; }
  void set_emitter_pin(uint8_t pin) { this->emitter_pin_ = pin; }

  uint8_t get_emitter_pin() const { return this->emitter_pin_; }

  void send_command(SomfyRemote *remote, Command command);

 protected:
  float frequency_{433.42f};
  uint8_t emitter_pin_{2};
};

class SomfyRTSCover : public cover::Cover, public Component {
 public:
  SomfyRTSCover(
      SomfyRTSHub *hub,
      const char *storage_name,
      const char *storage_key,
      uint32_t remote_code
  );

  void setup() override;
  cover::CoverTraits get_traits() override;
  void control(const cover::CoverCall &call) override;
  void loop() override;

  // ESPHome validates time periods in microseconds; the runtime calculation uses milliseconds.
  void set_open_duration(uint32_t duration) { this->open_duration_ = duration / 1000; }
  void set_close_duration(uint32_t duration) { this->close_duration_ = duration / 1000; }
  void set_invert_position(bool invert) { this->invert_position_ = invert; }
  void set_invert_direction(bool invert) { this->invert_direction_ = invert; }

  void program();
  void send_my();

 protected:
  void send_command_(Command command);
  void update_position_();
  void stop_motion_(bool send_command);
  void start_motion_(float target, bool opening);
  void publish_position_();

  SomfyRTSHub *hub_;
  const char *storage_name_;
  const char *storage_key_;
  uint32_t remote_code_;
  NVSRollingCodeStorage *storage_;
  SomfyRemote *remote_;

  uint32_t open_duration_{30000};
  uint32_t close_duration_{30000};
  uint32_t motion_started_{0};
  float motion_start_position_{cover::COVER_CLOSED};
  float target_position_{cover::COVER_CLOSED};
  bool moving_{false};
  bool opening_{false};
  bool invert_position_{false};
  bool invert_direction_{false};
  float physical_position_{cover::COVER_CLOSED};
};

class SomfyRTSButton : public button::Button, public Component {
 public:
  explicit SomfyRTSButton(SomfyRTSCover *cover) : cover_(cover) {}

 protected:
  void press_action() override;

  SomfyRTSCover *cover_;
};

}  // namespace somfy_rts
}  // namespace esphome
