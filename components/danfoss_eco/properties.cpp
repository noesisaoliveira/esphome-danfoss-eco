#include "properties.h"
#include "esphome/core/log.h"
#include "helpers.h"

namespace esphome {
namespace danfoss_eco {

static const char *const TAG = "danfoss_eco.prop";

bool DeviceProperty::init_handle(BLEClient *client) {
  auto chr = client->get_characteristic(this->service_uuid, this->characteristic_uuid);
  if (chr == nullptr) return false;
  this->handle = chr->handle;
  return true;
}

bool DeviceProperty::read_request(BLEClient *client) {
  if (this->handle == 0) return false;
  return esp_ble_gattc_read_char(client->get_gattc_if(), client->get_conn_id(), 
                                 this->handle, ESP_GATT_AUTH_REQ_NONE) == ESP_OK;
}

bool WritableProperty::write_request(BLEClient *client) {
  if (this->handle == 0 || this->data == nullptr) return false;
  auto *writable_data = static_cast<WritableData *>(this->data.get());
  uint8_t buffer[16];
  writable_data->pack(buffer);
  return this->write_request(client, buffer, writable_data->length);
}

bool WritableProperty::write_request(BLEClient *client, uint8_t *data, uint16_t data_len) {
  if (this->handle == 0) return false;
  return esp_ble_gattc_write_char(client->get_gattc_if(), client->get_conn_id(), 
                                  this->handle, data_len, data, 
                                  ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE) == ESP_OK;
}

void BatteryProperty::update_state(uint8_t *value, uint16_t value_len) {
  if (value_len > 0 && this->component_->battery_level() != nullptr) {
    this->component_->battery_level()->publish_state((float)value[0]);
  }
}

void TemperatureProperty::update_state(uint8_t *value, uint16_t value_len) {
  auto t_data = std::make_unique<TemperatureData>(this->xxtea_, value, value_len);
  if (this->component_->temperature() != nullptr) {
    this->component_->temperature()->publish_state(t_data->room_temperature);
  }
  this->component_->target_temperature = t_data->target_temperature;
  this->data = std::move(t_data);
  this->component_->publish_state();
}

void SettingsProperty::update_state(uint8_t *value, uint16_t value_len) {
  auto s_data = std::make_unique<SettingsData>(this->xxtea_, value, value_len);
  this->component_->mode = s_data->device_mode;
  // Corrigido para os nomes do teu device_data.h
  this->component_->set_visual_min_temperature_override(s_data->frost_protection_temperature);
  this->component_->set_visual_max_temperature_override(s_data->vacation_temperature);
  this->data = std::move(s_data);
  this->component_->publish_state();
}

void ErrorsProperty::update_state(uint8_t *value, uint16_t value_len) {
  auto e_data = std::make_unique<ErrorsData>(this->xxtea_, value, value_len);
  bool has_problem = e_data->E9_VALVE_DOES_NOT_CLOSE || e_data->E14_LOW_BATTERY;
  if (this->component_->problems() != nullptr) {
    this->component_->problems()->publish_state(has_problem);
  }
  this->data = std::move(e_data);
}

void SecretKeyProperty::update_state(uint8_t *value, uint16_t value_len) {}

} // namespace danfoss_eco
} // namespace esphome