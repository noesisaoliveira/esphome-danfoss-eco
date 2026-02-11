#include "my_component.h"
#include "device.h"
#include "esphome/core/log.h"

namespace esphome {
namespace danfoss_eco {

static const char *const TAG = "danfoss_eco.climate";

void MyComponent::setup() {
  this->xxtea_instance_ = std::make_shared<Xxtea>();
  this->device_ = std::make_shared<Device>(this, this->xxtea_instance_);
  this->device_->setup();
}

void MyComponent::loop() {
  this->device_->loop();
}

void MyComponent::update() {
  this->device_->update();
}

void MyComponent::dump_config() {
  LOG_CLIMATE("", "Danfoss Eco", this);
}

void MyComponent::control(const climate::ClimateCall &call) {
  this->device_->control(call);
}

climate::ClimateTraits MyComponent::traits() {
  auto traits = climate::ClimateTraits();
  // Fixed feature flags for 2026
  traits.set_supports_current_temperature(true);
  traits.set_supports_action(true);
  
  traits.set_visual_min_temperature(this->visual_min_temp_);
  traits.set_visual_max_temperature(this->visual_max_temp_);
  traits.set_visual_temperature_step(0.5f);
  traits.set_supported_modes({climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_AUTO});
  return traits;
}

void MyComponent::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  this->device_->gattc_event_handler(event, gattc_if, param);
}

void MyComponent::set_pin_code(const std::string &pin) {
  this->device_->set_pin_code(pin);
}

void MyComponent::set_secret_key(const std::string &key) {
  this->device_->set_secret_key(key);
}

void MyComponent::set_secret_key(uint8_t *key, bool persist) {
  this->device_->set_secret_key(key, persist);
}

} // namespace danfoss_eco
} // namespace esphome