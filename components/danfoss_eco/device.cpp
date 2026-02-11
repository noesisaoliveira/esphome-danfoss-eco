#include "device.h"
#include "esphome/core/log.h"

namespace esphome {
namespace danfoss_eco {

static const char *const TAG = "danfoss_eco.device";

void Device::setup() {
  auto sp_this = this->parent_->shared_from_this(); // Get shared pointer to parent
  auto xxtea = this->xxtea_;

  this->p_pin_ = std::make_shared<WritableProperty>(parent_, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_PIN);
  this->p_battery_ = std::make_shared<BatteryProperty>(parent_, xxtea);
  this->p_temperature_ = std::make_shared<TemperatureProperty>(parent_, xxtea);
  this->p_settings_ = std::make_shared<SettingsProperty>(parent_, xxtea);
  this->p_errors_ = std::make_shared<ErrorsProperty>(parent_, xxtea);
  this->p_secret_key_ = std::make_shared<SecretKeyProperty>(parent_, xxtea);

  this->properties_ = { p_pin_, p_battery_, p_temperature_, p_settings_, p_errors_, p_secret_key_ };
}

void Device::loop() {
  // Logic to process the commands_ queue based on BLE connection state
}

void Device::update() {
  // Push standard read requests to the queue
  this->commands_.push(new Command(CommandType::READ, this->p_temperature_));
  this->commands_.push(new Command(CommandType::READ, this->p_battery_));
}

void Device::control(const climate::ClimateCall &call) {
  // Handle climate changes and push write commands
}

void Device::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  // Handle characteristic discovery and read/write responses
}

void Device::set_pin_code(const std::string &str) {
  this->pin_code_ = uint32_t(std::stoul(str));
}

void Device::set_secret_key(const std::string &str) {
    // Hex parsing logic here
}

void Device::set_secret_key(uint8_t *key, bool persist) {
    this->xxtea_->set_key(key, 16);
}

} // namespace danfoss_eco
} // namespace esphome