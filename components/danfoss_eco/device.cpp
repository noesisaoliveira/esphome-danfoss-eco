#include "device.h"

#ifdef USE_ESP32

namespace esphome {
namespace danfoss_eco {

// 2026 Namespace Alias to solve the deep nesting error
using BLEState = esp32_ble_client::ble_client::ClientState;

void Device::setup() {
  std::shared_ptr<MyComponent> sp_this = std::static_pointer_cast<MyComponent>(shared_from_this());

  this->p_pin = std::make_shared<WritableProperty>(sp_this, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_PIN);
  this->p_battery = std::make_shared<BatteryProperty>(sp_this, xxtea);
  this->p_temperature = std::make_shared<TemperatureProperty>(sp_this, xxtea);
  this->p_settings = std::make_shared<SettingsProperty>(sp_this, xxtea);
  this->p_errors = std::make_shared<ErrorsProperty>(sp_this, xxtea);
  this->p_secret_key = std::make_shared<SecretKeyProperty>(sp_this, xxtea);

  this->properties = {this->p_pin, this->p_battery, this->p_temperature, this->p_settings, this->p_errors, this->p_secret_key};
  
  copy_address(this->parent()->get_address(), this->parent()->get_remote_bda());
  
  this->parent()->set_state(BLEState::IDLE);
}

void Device::loop() {
  if (this->status_has_error()) {
    this->disconnect();
    this->status_clear_error();
  }

  if (this->node_state != (uint8_t)BLEState::ESTABLISHED)
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
    this->commands_.push(new Command(CommandType::READ, this->p_battery));
    this->commands_.push(new Command(CommandType::READ, this->p_temperature));
    this->commands_.push(new Command(CommandType::READ, this->p_settings));
    this->commands_.push(new Command(CommandType::READ, this->p_errors));
  }
}

void Device::control(const climate::ClimateCall &call) {
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
  this->p_pin->write_request(this->parent(), pin_bytes, sizeof(pin_bytes));
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
    return;
  }
  this->node_state = (uint8_t)BLEState::ESTABLISHED;
}

void Device::connect() {
  if (this->node_state == (uint8_t)BLEState::ESTABLISHED) return;
  this->parent()->set_state(BLEState::CONNECTING_DISCOVERED);
}

void Device::disconnect() {
  this->parent()->set_enabled(false);
  this->node_state = (uint8_t)BLEState::IDLE;
}

// ... (remaining helper setters)

} // namespace danfoss_eco
} // namespace esphome
#endif