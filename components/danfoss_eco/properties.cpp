#include "esphome/components/climate/climate.h"
#include "esphome/core/log.h"

#include "properties.h"
#include "helpers.h"

#ifdef USE_ESP32

namespace esphome
{
    namespace danfoss_eco
    {
        bool DeviceProperty::init_handle(BLEClient *client)
        {
            ESP_LOGV(TAG, "[%s] resolving handler for service=%s, characteristic=%s", this->component_->get_name().c_str(), this->service_uuid.to_string().c_str(), this->characteristic_uuid.to_string().c_str());
            auto chr = client->get_characteristic(this->service_uuid, this->characteristic_uuid);
            if (chr == nullptr)
            {
                ESP_LOGW(TAG, "[%s] characteristic uuid=%s not found", this->component_->get_name().c_str(), this->characteristic_uuid.to_string().c_str());
                return false;
            }

            this->handle = chr->handle;
            return true;
        }

        bool DeviceProperty::read_request(BLEClient *client)
        {
            auto status = esp_ble_gattc_read_char(client->get_gattc_if(),
                                                  client->get_conn_id(),
                                                  this->handle,
                                                  ESP_GATT_AUTH_REQ_NONE);
            if (status != ESP_OK)
                ESP_LOGW(TAG, "[%s] esp_ble_gattc_read_char failed, handle=%#04x, status=%01x", this->component_->get_name().c_str(), this->handle, status);

            return status == ESP_OK;
        }

        bool WritableProperty::write_request(BLEClient *client, uint8_t *data, uint16_t data_len)
        {
            ESP_LOGD(TAG, "[%s] write_request: handle=%#04x, data=%s", this->component_->get_name().c_str(), this->handle, format_hex_pretty(data, data_len).c_str());

            auto status = esp_ble_gattc_write_char(client->get_gattc_if(),
                                                   client->get_conn_id(),
                                                   this->handle,
                                                   data_len,
                                                   data,
                                                   ESP_GATT_WRITE_TYPE_RSP,
                                                   ESP_GATT_AUTH_REQ_NONE);
            if (status != ESP_OK)
                ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, handle=%#04x, status=%01x", this->component_->get_name().c_str(), this->handle, status);

            return status == ESP_OK;
        }

        bool WritableProperty::write_request(BLEClient *client)
        {
            WritableData *writableData = static_cast<WritableData *>(this->data.get());
            uint8_t buff[this->data->length]{0};
            writableData->pack(buff);
            return this->write_request(client, buff, sizeof(buff));
        }

        void BatteryProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            uint8_t battery_level = value[0];
            ESP_LOGD(TAG, "[%s] battery level: %d %%", this->component_->get_name().c_str(), battery_level);
            if (this->component_->battery_level() != nullptr)
                this->component_->battery_level()->publish_state(battery_level);
        }

        void TemperatureProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            auto t_data = new TemperatureData(this->xxtea_, value, value_len);
            this->data.reset(t_data);

            ESP_LOGD(TAG, "[%s] Current room temperature: %2.1f°C, Set point temperature: %2.1f°C", this->component_->get_name().c_str(), t_data->room_temperature, t_data->target_temperature);
            if (this->component_->temperature() != nullptr)
                this->component_->temperature()->publish_state(t_data->room_temperature);

            // FIX: Modern climate action logic for 2026
            this->component_->action = (t_data->room_temperature >= t_data->target_temperature) ? climate::CLIMATE_ACTION_IDLE : climate::CLIMATE_ACTION_HEATING;
            this->component_->target_temperature = t_data->target_temperature;
            this->component_->current_temperature = t_data->room_temperature;
            this->component_->publish_state();
        }

        void SettingsProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            auto s_data = new SettingsData(this->xxtea_, value, value_len);
            this->data.reset(s_data);

            const char *name = this->component_->get_name().c_str();
            ESP_LOGD(TAG, "[%s] temperature_min: %2.1f°C, temperature_max: %2.1f°C", name, s_data->temperature_min, s_data->temperature_max);

            // apply read configuration to the component
            this->component_->mode = s_data->device_mode;
            
            // These methods now exist because we added them to MyComponent in my_component.h
            this->component_->set_visual_min_temperature_override(s_data->temperature_min);
            this->component_->set_visual_max_temperature_override(s_data->temperature_max);
            
            this->component_->publish_state();
        }

        void ErrorsProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            auto e_data = new ErrorsData(this->xxtea_, value, value_len);
            this->data.reset(e_data);

            if (this->component_->problems() != nullptr)
                this->component_->problems()->publish_state(e_data->E9_VALVE_DOES_NOT_CLOSE || e_data->E10_INVALID_TIME || e_data->E14_LOW_BATTERY || e_data->E15_VERY_LOW_BATTERY);
        }

        bool SecretKeyProperty::init_handle(BLEClient *client)
        {
            if (this->xxtea_->status() != XXTEA_STATUS_NOT_INITIALIZED)
            {
                ESP_LOGD(TAG, "[%s] xxtea is initialized", this->component_->get_name().c_str());
                return true;
            }

            auto chr = client->get_characteristic(this->service_uuid, this->characteristic_uuid);
            if (chr != nullptr)
            {
                this->handle = chr->handle;
                return true;
            }

            ESP_LOGW(TAG, "[%s] Hardware button not pressed, cannot read secret key", this->component_->get_name().c_str());
            this->handle = INVALID_HANDLE;
            return false;
        }

        void SecretKeyProperty::update_state(uint8_t *value, uint16_t value_len)
        {
            if (value_len != SECRET_KEY_LENGTH) return;

            char key_str[SECRET_KEY_LENGTH * 2 + 1];
            encode_hex(value, value_len, key_str);

            ESP_LOGI(TAG, "[%s] secret_key found: %s", this->component_->get_name().c_str(), key_str);
            this->component_->set_secret_key(value, true);
        }

    } // namespace danfoss_eco
} // namespace esphome

#endif // USE_ESP32