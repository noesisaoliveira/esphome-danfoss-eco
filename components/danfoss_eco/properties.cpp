#include "esphome/components/climate/climate.h"
#include "esphome/core/log.h"
#include "properties.h"
#include "helpers.h"

#ifdef USE_ESP32

namespace esphome {
namespace danfoss_eco {

static const char *const TAG = "danfoss_eco.prop";

bool DeviceProperty::init_handle(BLEClient *client) {
    auto chr = client->get_characteristic(this->service_uuid, this->characteristic_uuid);
    if (chr == nullptr) {
        ESP_LOGW(TAG, "[%s] characteristic %s not found", 
                 this->component_->get_name().c_str(), 
                 this->characteristic_uuid.to_string().c_str());
        return false;
    }
    this->handle = chr->handle;
    return true;
}

bool DeviceProperty::read_request(BLEClient *client) {
    auto status = esp_ble_gattc_read_char(client->get_gattc_if(),
                                          client->get_conn_id(),
                                          this->handle,
                                          ESP_GATT_AUTH_REQ_NONE);
    return status == ESP_OK;
}

bool WritableProperty::write_request(BLEClient *client, uint8_t *data, uint16_t data_len) {
    auto status = esp_ble_gattc_write_char(client->get_gattc_if(),
                                           client->get_conn_id(),
                                           this->handle,
                                           data_len,
                                           data,
                                           ESP_GATT_WRITE_TYPE_RSP,
                                           ESP_GATT_AUTH_REQ_NONE);
    return status == ESP_OK;
}

bool WritableProperty::write_request(BLEClient *client) {
    if (this->data == nullptr) return false;
    WritableData *writableData = static_cast<WritableData *>(this->data.get());
    uint8_t buff[16] = {0}; // Max buffer size for Danfoss properties
    writableData->pack(buff);
    return this->write_request(client, buff, writableData->length);
}

void BatteryProperty::update_state(uint8_t *value, uint16_t value_len) {
    if (value_len > 0 && this->component_->battery_level() != nullptr)
        this->component_->battery_level()->publish_state(value[0]);
}

void TemperatureProperty::update_state(uint8_t *value, uint16_t value_len) {
    auto t_data = new TemperatureData(this->xxtea_, value, value_len);
    this->data.reset(t_data);

    this->component_->current_temperature = t_data->room_temperature;
    this->component_->target_temperature = t_data->target_temperature;
    
    this->component_->action = (t_data->room_temperature >= t_data->target_temperature) ? 
                               climate::CLIMATE_ACTION_IDLE : climate::CLIMATE_ACTION_HEATING;
    
    if (this->component_->temperature() != nullptr)
        this->component_->temperature()->publish_state(t_data->room_temperature);
        
    this->component_->publish_state();
}

void SettingsProperty::update_state(uint8_t *value, uint16_t value_len) {
    auto s_data = new SettingsData(this->xxtea_, value, value_len);
    this->data.reset(s_data);

    this->component_->mode = s_data->device_mode;
    this->component_->set_visual_min_temperature_override(s_data->temperature_min);
    this->component_->set_visual_max_temperature_override(s_data->temperature_max);
    this->component_->publish_state();
}

void ErrorsProperty::update_state(uint8_t *value, uint16_t value_len) {
    auto e_data = new ErrorsData(this->xxtea_, value, value_len);
    this->data.reset(e_data);
    if (this->component_->problems() != nullptr) {
        bool has_error = e_data->E9_VALVE_DOES_NOT_CLOSE || e_data->E10_INVALID_TIME || 
                         e_data->E14_LOW_BATTERY || e_data->E15_VERY_LOW_BATTERY;
        this->component_->problems()->publish_state(has_error);
    }
}

bool SecretKeyProperty::init_handle(BLEClient *client) {
    if (this->xxtea_->status() != XXTEA_STATUS_NOT_INITIALIZED) return true;
    return DeviceProperty::init_handle(client);
}

void SecretKeyProperty::update_state(uint8_t *value, uint16_t value_len) {
    if (value_len == 16) this->component_->set_secret_key(value, true);
}

} // namespace danfoss_eco
} // namespace esphome

#endif