#include "somfy_rts.h"

#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <algorithm>
#include <nvs.h>

namespace esphome {
namespace somfy_rts {

static const char *const TAG = "somfy_rts";

void SomfyRTSHub::setup() {
  ESP_LOGI(TAG, "setup: begin");

  ESP_LOGI(TAG, "setup: configuring emitter GPIO %u", this->emitter_pin_);
  pinMode(this->emitter_pin_, OUTPUT);
  digitalWrite(this->emitter_pin_, LOW);
  yield();
  ESP_LOGI(TAG, "setup: emitter GPIO ready");

  // Explicitly select the NodeMCU-32S VSPI pins. Without this, the
  // SmartRC driver may select the alternate HSPI mapping (GPIO 13/12/14),
  // which can hang during Init() with newer Arduino-ESP32 releases.
  ESP_LOGI(TAG, "setup: selecting SPI SCK=18 MISO=19 MOSI=23 CS=5");
  ELECHOUSE_cc1101.setSpiPin(18, 19, 23, 5);

  ESP_LOGI(TAG, "setup: CC1101 Init begin");
  ELECHOUSE_cc1101.Init();
  ESP_LOGI(TAG, "setup: CC1101 Init complete");
  yield();

  ESP_LOGI(TAG, "setup: setMHZ(%.2f) begin", this->frequency_);
  ELECHOUSE_cc1101.setMHZ(this->frequency_);
  ESP_LOGI(TAG, "setup: setMHZ complete");
}

void SomfyRTSHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Somfy RTS:");
  ESP_LOGCONFIG(TAG, "  CC1101 frequency: %.2f MHz", this->frequency_);
  ESP_LOGCONFIG(TAG, "  Emitter GPIO: %u", this->emitter_pin_);
}

void SomfyRTSHub::send_command(SomfyRemote *remote, Command command) {
  ELECHOUSE_cc1101.SetTx();
  remote->sendCommand(command);
  ELECHOUSE_cc1101.setSidle();
}

SomfyRTSCover::SomfyRTSCover(
    SomfyRTSHub *hub,
    const char *storage_name,
    const char *storage_key,
    uint32_t remote_code
)
    : hub_(hub), storage_name_(storage_name), storage_key_(storage_key), remote_code_(remote_code) {
  this->storage_ =
      new NVSRollingCodeStorage(storage_name, storage_key);

  this->remote_ =
      new SomfyRemote(
          this->hub_->get_emitter_pin(),
          remote_code,
          this->storage_
      );
}

void SomfyRTSCover::setup() {
  this->publish_position_();

  nvs_handle_t handle;
  esp_err_t err = nvs_open(this->storage_name_, NVS_READONLY, &handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Remote 0x%06lX storage '%s/%s': NVS open failed: %s",
             static_cast<unsigned long>(this->remote_code_),
             this->storage_name_, this->storage_key_, esp_err_to_name(err));
    return;
  }

  uint16_t rolling_code = 0;
  err = nvs_get_u16(handle, this->storage_key_, &rolling_code);
  nvs_close(handle);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "Remote 0x%06lX storage '%s/%s' stored rolling code: %u",
             static_cast<unsigned long>(this->remote_code_),
             this->storage_name_, this->storage_key_, rolling_code);
  } else {
    ESP_LOGW(TAG, "Remote 0x%06lX storage '%s/%s': rolling code not found: %s",
             static_cast<unsigned long>(this->remote_code_),
             this->storage_name_, this->storage_key_, esp_err_to_name(err));
  }
}

cover::CoverTraits SomfyRTSCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(true);
  traits.set_supports_position(true);
  traits.set_supports_tilt(false);
  traits.set_supports_stop(true);
  return traits;
}

void SomfyRTSCover::send_command_(Command command) {
  ESP_LOGI(TAG, "TX command %u remote 0x%06lX", static_cast<unsigned>(command),
           static_cast<unsigned long>(this->remote_code_));
  this->hub_->send_command(this->remote_, command);
}

void SomfyRTSCover::control(const cover::CoverCall &call) {
  this->update_position_();

  if (call.get_stop()) {
    ESP_LOGI(TAG, "STOP / MY at %.0f%%", this->position * 100.0f);
    this->stop_motion_(true);
    return;
  }

  if (call.get_position().has_value()) {
    const float requested_position = *call.get_position();
    const float pos = this->invert_direction_ ? 1.0f - requested_position : requested_position;
    if (pos <= this->physical_position_ + 0.001f && pos >= this->physical_position_ - 0.001f) {
      return;
    }
    const bool opening = pos > this->physical_position_;
    ESP_LOGI(TAG, "%s to %.0f%% from %.0f%%", opening ? "OPEN" : "CLOSE",
             requested_position * 100.0f, this->position * 100.0f);
    this->start_motion_(pos, opening);
  }
}

void SomfyRTSCover::loop() {
  if (!this->moving_)
    return;

  this->update_position_();
  if ((this->opening_ && this->physical_position_ >= this->target_position_) ||
      (!this->opening_ && this->physical_position_ <= this->target_position_)) {
    this->physical_position_ = this->target_position_;
    // The motor is already commanded to run to its physical end stop for
    // 0%/100% requests. Do not send MY there, because MY would interrupt it.
    // Intermediate targets still need MY to stop the motor at that position.
    const bool at_physical_end =
        this->target_position_ <= cover::COVER_CLOSED ||
        this->target_position_ >= cover::COVER_OPEN;
    this->stop_motion_(!at_physical_end);
  }
}

void SomfyRTSCover::update_position_() {
  if (!this->moving_)
    return;

  const uint32_t duration = this->opening_ ? this->open_duration_ : this->close_duration_;
  if (duration == 0)
    return;

  const float elapsed = static_cast<float>(millis() - this->motion_started_);
  const float progress = elapsed / static_cast<float>(duration);
  if (this->opening_)
    this->physical_position_ = this->motion_start_position_ + progress;
  else
    this->physical_position_ = this->motion_start_position_ - progress;

  this->physical_position_ = std::max(cover::COVER_CLOSED,
                                      std::min(cover::COVER_OPEN, this->physical_position_));
  this->publish_position_();
}

void SomfyRTSCover::start_motion_(float target, bool opening) {
  this->motion_start_position_ = this->physical_position_;
  this->target_position_ = target;
  this->opening_ = opening;
  this->motion_started_ = millis();
  this->moving_ = true;
  this->send_command_(opening ? Command::Up : Command::Down);
}

void SomfyRTSCover::stop_motion_(bool send_command) {
  this->update_position_();
  this->moving_ = false;
  if (send_command)
    this->send_command_(Command::My);
  this->publish_position_();
}

void SomfyRTSCover::publish_position_() {
  this->position = this->invert_position_ ?
      1.0f - this->physical_position_ : this->physical_position_;
  this->publish_state();
}

void SomfyRTSCover::program() {
  ESP_LOGI(TAG, "PROG remote 0x%06lX storage '%s/%s'",
           static_cast<unsigned long>(this->remote_code_),
           this->storage_name_, this->storage_key_);
  this->send_command_(Command::Prog);
}

void SomfyRTSCover::send_my() {
  ESP_LOGI(TAG, "MY");
  this->send_command_(Command::My);
}

void SomfyRTSButton::press_action() {
  this->cover_->program();
}

}  // namespace somfy_rts
}  // namespace esphome
