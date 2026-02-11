#include "device.h"
#include "esphome/core/log.h"
#include "helpers.h"

namespace esphome {
namespace danfoss_eco {

static const char *const TAG = "danfoss_eco.device";

void Device::setup() {
  auto xxtea = this->xxtea_;

  // Initialize all properties with raw parent pointer
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
  if (this->parent_->node_state != ble_client::ClientState::ESTABLISHED) {
    // Clear queue if connection is lost
    while (!this->commands_.empty()) {
      delete this->commands_.front();
      this->commands_.pop();
    }
    return;
  }

  // Process the command queue one by one
  if (!this->commands_.empty()) {
    Command *cmd = this->commands_.front();
    if (cmd->execute(this->parent_->parent())) { // parent() gets the BLEClient
      delete cmd;
      this->commands_.pop();
    }
  }
}

void Device::update() {
  if (this->parent_->node_state != ble_client::ClientState::ESTABLISHED) return;

  ESP_LOGD(TAG, "Requesting status update from valve...");
  this->commands_.push(new Command(CommandType::READ, this->p_temperature_));
  this->commands_.push(new Command(CommandType::READ, this->p_battery_));
  this->commands_.push(new Command(CommandType::READ, this->p_settings_));
}

void Device::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    float temp = *call.get_target_temperature();
    ESP_LOGD(TAG, "Setting target temperature to %.1f", temp);
    
    // Create new TemperatureData to pack into the property
    auto data = std::make_unique<TemperatureData>(this->xxtea_);
    data->target_temperature = temp;
    this->p_temperature_->data = std::move(data);
    
    this->commands_.push(new Command(CommandType::WRITE, this->p_temperature_));
  }
}

void Device::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      ESP_LOGD(TAG, "Service search complete. Initializing property handles...");
      for (auto &prop : this->properties_) {
        prop->init_handle(this->parent_->parent());
      }
      // After discovery, immediately try to authorize if we have a pin
      this->write_pin();
      break;
    }
    case ESP_GATTC_READ_CHAR_EVT: {
      if (param->read.status == ESP_GATT_OK) {
        for (auto &prop : this->properties_) {
          if (prop->handle == param->read.handle) {
            prop->update_state(param->read.value, param->read.value_len);
            break;
          }
        }
      }
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
      if (param->write.status == ESP_GATT_OK) {
        ESP_LOGV(TAG, "Write successful to handle 0x%X", param->write.handle);
      } else {
        ESP_LOGW(TAG, "Write failed to handle 0x%X, status=%d", param->write.handle, param->write.status);
      }
      break;
    }
    default:
      break;
  }
}

void Device::write_pin() {
  if (this->pin_code_ == 0) return;
  
  ESP_LOGD(TAG, "Authorizing with PIN...");
  uint8_t pin_data[4];
  pin_data[0] = (this->pin_code_ >> 0) & 0xFF;
  pin_data[1] = (this->pin_code_ >> 8) & 0xFF;
  pin_data[2] = (this->pin_code_ >> 16) & 0xFF;
  pin_data[3] = (this->pin_code_ >> 24) & 0xFF;

  this->p_pin_->write_request(this->parent_->parent(), pin_data, 4);
}

void Device::set_pin_code(const std::string &str) {
  try {
    this->pin_code_ = std::stoul(str);
  } catch (...) {
    ESP_LOGE(TAG, "Invalid PIN code format: %s", str.c_str());
  }
}

void Device::set_secret_key(const std::string &str) {
  uint8_t key[16];
  if (parse_hex(str, key, 16)) {
    this->set_secret_key(key, false);
  } else {
    ESP_LOGE(TAG, "Invalid Secret Key format (must be 32 hex chars)");
  }
}

void Device::set_secret_key(uint8_t *key, bool persist) {
  ESP_LOGI(TAG, "Secret key updated.");
  this->xxtea_->set_key(key, 16);
  // Implementation for persistence (NVS) could go here
}

} // namespace danfoss_eco
} // namespace esphome