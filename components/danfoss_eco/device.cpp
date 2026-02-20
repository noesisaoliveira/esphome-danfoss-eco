#include "device.h"
#include "esphome/core/log.h"
#include "helpers.h"

namespace esphome {
namespace danfoss_eco {

static const char *const TAG = "danfoss_eco.device";

void Device::setup() {
  auto xxtea = this->xxtea_;
  if (!this->pending_secret_key_.empty()) {
    uint8_t key[16];
    parse_hex_str(this->pending_secret_key_.c_str(), this->pending_secret_key_.length(), key);
    xxtea->set_key(key, 16);
  }

  this->p_pin_ = std::make_shared<WritableProperty>(this->parent_, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_PIN);
  this->p_battery_ = std::make_shared<BatteryProperty>(this->parent_, xxtea);
  this->p_temperature_ = std::make_shared<TemperatureProperty>(this->parent_, xxtea);
  this->p_settings_ = std::make_shared<SettingsProperty>(this->parent_, xxtea);
  this->p_errors_ = std::make_shared<ErrorsProperty>(this->parent_, xxtea);

  this->properties_ = {this->p_battery_, this->p_temperature_, this->p_settings_, this->p_errors_};
}

void Device::write_pin() {
  if (this->pin_code_ == 0) return;
  uint8_t pin_data[4];
  // BIG ENDIAN como no Python
  pin_data[0] = (this->pin_code_ >> 24) & 0xFF;
  pin_data[1] = (this->pin_code_ >> 16) & 0xFF;
  pin_data[2] = (this->pin_code_ >> 8) & 0xFF;
  pin_data[3] = (this->pin_code_ >> 0) & 0xFF;
  this->p_pin_->write_request(this->parent_->parent(), pin_data, 4);
}

void Device::update() {
  for (auto &prop : this->properties_) prop->read_request(this->parent_->parent());
}

void Device::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  if (event == ESP_GATTC_SEARCH_CMPL_EVT) {
    for (auto &prop : this->properties_) prop->init_handle(this->parent_->parent());
    this->write_pin();
  } else if (event == ESP_GATTC_READ_CHAR_EVT && param->read.status == ESP_GATT_OK) {
    for (auto &prop : this->properties_) {
      if (prop->handle == param->read.handle) {
        prop->update_state(param->read.value, param->read.value_len);
        break;
      }
    }
  }
}

void Device::set_pin_code(const std::string &str) { this->pin_code_ = (uint32_t)strtoul(str.c_str(), nullptr, 10); }
void Device::set_secret_key(const std::string &str) { this->pending_secret_key_ = str; }

} // namespace danfoss_eco
} // namespace esphome