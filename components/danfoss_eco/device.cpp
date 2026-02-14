#include "device.h"
#include "esphome/core/log.h"
#include "helpers.h"

namespace esphome {
namespace danfoss_eco {

static const char *const TAG = "danfoss_eco.device";

void Device::setup() {
  auto xxtea = this->xxtea_;

  // Apply secret key if it was set before setup
  if (!this->pending_secret_key_.empty()) {
    ESP_LOGD(TAG, "Applying pending secret key: %s", this->pending_secret_key_.c_str());
    uint8_t key[16];
    parse_hex_str(this->pending_secret_key_.c_str(), this->pending_secret_key_.length(), key);
    xxtea->set_key(key, 16);
    ESP_LOGD(TAG, "Secret key applied successfully");
  } else {
    ESP_LOGW(TAG, "No secret key set! Communication will fail.");
  }

  this->p_pin_ = std::make_shared<WritableProperty>(this->parent_, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_PIN);
  this->p_battery_ = std::make_shared<BatteryProperty>(this->parent_, xxtea);
  this->p_temperature_ = std::make_shared<TemperatureProperty>(this->parent_, xxtea);
  this->p_settings_ = std::make_shared<SettingsProperty>(this->parent_, xxtea);
  this->p_errors_ = std::make_shared<ErrorsProperty>(this->parent_, xxtea);
  this->p_secret_key_ = std::make_shared<SecretKeyProperty>(this->parent_, xxtea);

  this->properties_ = {
    this->p_pin_, this->p_battery_, this->p_temperature_, 
    this->p_settings_, this->p_errors_, this->p_secret_key_
  };
}

void Device::loop() {
  if (this->parent_->node_state != esp32_ble_tracker::ClientState::ESTABLISHED) {
    while (!this->commands_.empty()) {
      delete this->commands_.front();
      this->commands_.pop();
    }
    return;
  }

  if (!this->commands_.empty()) {
    Command *cmd = this->commands_.front();
    if (cmd->execute(this->parent_->parent())) {
      delete cmd;
      this->commands_.pop();
    }
  }
}

void Device::update() {
  ESP_LOGD(TAG, "Device::update() called");
  if (this->parent_->node_state != esp32_ble_tracker::ClientState::ESTABLISHED) {
    ESP_LOGD(TAG, "BLE not connected, skipping update");
    return;
  }

  ESP_LOGD(TAG, "Reading temperature and battery");
  this->commands_.push(new Command(CommandType::READ, this->p_temperature_));
  this->commands_.push(new Command(CommandType::READ, this->p_battery_));
}

void Device::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    auto data = std::make_unique<TemperatureData>(this->xxtea_);
    data->target_temperature = temp;
    data->room_temperature = this->parent_->current_temperature; 
    
    this->p_temperature_->data = std::move(data);
    this->commands_.push(new Command(CommandType::WRITE, this->p_temperature_));
  }
}

void Device::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_SEARCH_CMPL_EVT:
      for (auto &prop : this->properties_) {
        prop->init_handle(this->parent_->parent());
      }
      this->write_pin();
      break;
    case ESP_GATTC_READ_CHAR_EVT:
      if (param->read.status == ESP_GATT_OK) {
        for (auto &prop : this->properties_) {
          if (prop->handle == param->read.handle) {
            prop->update_state(param->read.value, param->read.value_len);
            break;
          }
        }
      }
      break;
    default:
      break;
  }
}

void Device::write_pin() {
  if (this->pin_code_ == 0) return;
  uint8_t pin_data[4];
  pin_data[0] = (this->pin_code_ >> 0) & 0xFF;
  pin_data[1] = (this->pin_code_ >> 8) & 0xFF;
  pin_data[2] = (this->pin_code_ >> 16) & 0xFF;
  pin_data[3] = (this->pin_code_ >> 24) & 0xFF;
  this->p_pin_->write_request(this->parent_->parent(), pin_data, 4);
}

void Device::set_pin_code(const std::string &str) {
  this->pin_code_ = (uint32_t) strtoul(str.c_str(), nullptr, 10);
}

void Device::set_secret_key(const std::string &str) {
  ESP_LOGD(TAG, "Device::set_secret_key called with: %s", str.c_str());
  // Store the key to be applied during setup()
  this->pending_secret_key_ = str;
  ESP_LOGD(TAG, "Stored as pending_secret_key_");
}

void Device::set_secret_key(uint8_t *key, bool persist) {
  if (!this->xxtea_) {
    ESP_LOGE(TAG, "XXTEA not initialized yet");
    return;
  }
  this->xxtea_->set_key(key, 16);
}

} // namespace danfoss_eco
} // namespace esphome
