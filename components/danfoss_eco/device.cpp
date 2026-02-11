#include "device.h"

#ifdef USE_ESP32

namespace esphome {
namespace danfoss_eco {

void Device::setup() {
  std::shared_ptr<MyComponent> sp_this(this);

  this->p_pin = std::make_shared<WritableProperty>(sp_this, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_PIN);
  this->p_battery = std::make_shared<BatteryProperty>(sp_this, xxtea);
  this->p_temperature = std::make_shared<TemperatureProperty>(sp_this, xxtea);
  this->p_settings = std::make_shared<SettingsProperty>(sp_this, xxtea);
  this->p_errors = std::make_shared<ErrorsProperty>(sp_this, xxtea);
  this->p_secret_key = std::make_shared<SecretKeyProperty>(sp_this, xxtea);

  this->properties = {this->p_pin, this->p_battery, this->p_temperature, this->p_settings, this->p_errors, this->p_secret_key};
  
  // Initialize remote address
  copy_address(this->parent()->get_address(), this->parent()->get_remote_bda());
  
  // 2026 Fix: ClientState::INIT is now IDLE
  this->parent()->set_state(ble_client::BLEClientState::IDLE);
}

void Device::loop() {
  if (this->status_has_error()) {
    this->disconnect();
    this->status_clear_error();
  }

  // 2026 Fix: Full namespace for state comparison
  if (this->node_state != ble_client::BLEClientState::ESTABLISHED)
    return;

  Command *cmd = this->commands_.pop();
  while (cmd != nullptr) {
    if (cmd->execute(this->parent()))
      this->request_counter_++;

    delete cmd;
    cmd = this->commands_.pop();
  }

  if (this->request_counter_ == 0)
    this->disconnect();
}

void Device::update() {
  this->connect();

  if (this->xxtea->status() == XXTEA_STATUS_SUCCESS) {
    ESP_LOGI(TAG, "[%s] requesting device state", this->get_name().c_str());

    this->commands_.push(new Command(CommandType::READ, this->p_battery));
    this->commands_.push(new Command(CommandType::READ, this->p_temperature));
    this->commands_.push(new Command(CommandType::READ, this->p_settings));
    this->commands_.push(new Command(CommandType::READ, this->p_errors));
  }
}

void Device::control(const ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    TemperatureData &t_data = (TemperatureData &)(*this->p_temperature->data);
    t_data.target_temperature = *call.get_target_temperature();

    this->commands_.push(new Command(CommandType::WRITE, this->p_temperature));
    this->connect();
  }

  if (call.get_mode().has_value()) {
    SettingsData &s_data = (SettingsData &)(*this->p_settings->data);
    s_data.device_mode = *call.get_mode();

    this->mode = s_data.device_mode;
    this->publish_state();

    this->commands_.push(new Command(CommandType::WRITE, this->p_settings));
    this->connect();
  }
}

void Device::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_CONNECT_EVT:
      if (memcmp(param->connect.remote_bda, this->parent()->get_remote_bda(), 6) != 0)
        return; 
      ESP_LOGD(TAG, "[%s] connect, conn_id=%d", this->get_name().c_str(), param->connect.conn_id);
      break;

    case ESP_GATTC_OPEN_EVT:
      if (param->open.status == ESP_GATT_OK)
        ESP_LOGV(TAG, "[%s] open, conn_id=%d", this->get_name().c_str(), param->open.conn_id);
      else
        ESP_LOGW(TAG, "[%s] failed to open, status=%#04x", this->get_name().c_str(), param->open.status);
      break;

    case ESP_GATTC_SEARCH_CMPL_EVT:
      for (auto p : this->properties)
        p->init_handle(this->parent());
      write_pin();
      break;

    case ESP_GATTC_WRITE_CHAR_EVT:
      if (param->write.handle == this->p_pin->handle)
        this->on_write_pin(param->write);
      else
        this->on_write(param->write);
      break;

    case ESP_GATTC_READ_CHAR_EVT:
      this->on_read(param->read);
      break;

    default:
      break;
  }
}

void Device::write_pin() {
  uint8_t pin_bytes[4];
  write_int(pin_bytes, 0, this->pin_code_);
  if (!this->p_pin->write_request(this->parent(), pin_bytes, sizeof(pin_bytes)))
    this->status_set_error();
}

void Device::on_read(esp_ble_gattc_cb_param_t::gattc_read_char_evt_param param) {
  this->request_counter_--;
  auto it = std::find_if(properties.begin(), properties.end(),
                         [&param](std::shared_ptr<DeviceProperty> p) { return p->handle == param.handle; });

  if (it != properties.end())
    (*it)->update_state(param.value, param.value_len);
}

void Device::on_write(esp_ble_gattc_cb_param_t::gattc_write_evt_param param) {
  this->request_counter_--;
  if (param.status == ESP_GATT_OK) update();
}

void Device::on_write_pin(esp_ble_gattc_cb_param_t::gattc_write_evt_param param) {
  if (param.status != ESP_GATT_OK) {
    this->disconnect();
    this->mark_failed();
    return;
  }
  this->node_state = ble_client::BLEClientState::ESTABLISHED;
  if (this->xxtea->status() == XXTEA_STATUS_NOT_INITIALIZED && this->p_secret_key->handle != INVALID_HANDLE) {
    this->commands_.push(new Command(CommandType::READ, this->p_secret_key));
  }
}

void Device::connect() {
  if (this->node_state == ble_client::BLEClientState::ESTABLISHED) return;

  if (!parent()->enabled) {
    parent()->set_enabled(true);
  }
  esp_ble_gap_stop_scanning();
  // 2026 Fix: READY_TO_CONNECT is now DISCOVERED
  this->parent()->set_state(ble_client::BLEClientState::DISCOVERED);
}

void Device::disconnect() {
  this->parent()->set_enabled(false);
  this->node_state = ble_client::BLEClientState::IDLE;
}

void Device::set_pin_code(const std::string &str) {
  if (!str.empty()) this->pin_code_ = atoi(str.c_str());
}

void Device::set_secret_key(const std::string &str) {
  uint32_t hash = fnv1_hash("danfoss_eco_secret__" + this->get_name());
  this->secret_pref_ = global_preferences->make_preference<SecretKeyValue>(hash, true);

  if (!str.empty()) {
    uint8_t buff[16];
    parse_hex_str(str.c_str(), 32, buff);
    this->set_secret_key(buff, false);
  } else {
    SecretKeyValue key_buff;
    if (this->secret_pref_.load(&key_buff)) {
      this->set_secret_key(key_buff.value, false);
    }
  }
}

void Device::set_secret_key(uint8_t *key, bool persist) {
  if (this->xxtea->set_key(key, 16) != XXTEA_STATUS_SUCCESS) {
    this->mark_failed();
  } else if (persist) {
    SecretKeyValue key_buff(key);
    this->secret_pref_.save(&key_buff);
    global_preferences->sync();
  }
}

} // namespace danfoss_eco
} // namespace esphome

#endif