#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/button/button.h"
#include "esphome/components/sensor/sensor.h"

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
  void set_default_position(float position) { this->default_position_ = position; }
  void set_save_position(bool save) { this->save_position_ = save; }
  void set_position_save_interval(uint32_t interval) { this->position_save_interval_ = interval / 1000; }
  void set_rolling_code_sensor(sensor::Sensor *sensor) { this->rolling_code_sensor_ = sensor; }
  void update_rolling_code_sensor();

  void program();
  void send_my();

 protected:
  void send_command_(Command command);
  void update_position_();
  void stop_motion_(bool send_command);
  void start_motion_(float target, bool opening);
  void publish_position_();
  void persist_position_();
  bool load_position_();

  SomfyRTSHub *hub_;
  const char *storage_name_;
  const char *storage_key_;
  char position_key_[16];
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
  float default_position_{cover::COVER_OPEN};
  bool save_position_{true};
  uint32_t position_save_interval_{300000};
  uint32_t position_save_started_{0};
  bool position_save_pending_{false};
  sensor::Sensor *rolling_code_sensor_{nullptr};
};

class SomfyRTSButton : public button::Button, public Component {
 public:
  explicit SomfyRTSButton(SomfyRTSCover *cover) : cover_(cover) {}

 protected:
  void press_action() override;

  SomfyRTSCover *cover_;
};

class SomfyRTSRollingCodeSensor : public sensor::Sensor, public Component {
 public:
  explicit SomfyRTSRollingCodeSensor(SomfyRTSCover *cover) : cover_(cover) {}
  void setup() override;

 protected:
  SomfyRTSCover *cover_;
};

}  // namespace somfy_rts
}  // namespace esphome
