#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "xxtea.h"

namespace esphome {
namespace danfoss_eco {

class Device; // Forward declaration

class MyComponent : public climate::Climate, public esphome::ble_client::BLEClientNode, public Component {
 public:
  void setup() override;
  void loop() override;
  void update();
  void dump_config() override;

  // Climate overrides
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;

  // Visual Temperature Overrides (Driven by SettingsProperty)
  void set_visual_min_temperature_override(float temp) { visual_min_temp_ = temp; }
  void set_visual_max_temperature_override(float temp) { visual_max_temp_ = temp; }

  // Component Links
  void set_battery_level(sensor::Sensor *s) { battery_level_ = s; }
  void set_temperature(sensor::Sensor *s) { temperature_ = s; }
  void set_problems(binary_sensor::BinarySensor *s) { problems_ = s; }
  
  sensor::Sensor *battery_level() { return battery_level_; }
  sensor::Sensor *temperature() { return temperature_; }
  binary_sensor::BinarySensor *problems() { return problems_; }

  void set_pin_code(const std::string &pin);
  void set_secret_key(const std::string &key);
  void set_secret_key(uint8_t *key, bool persist);

  // GATT Event Bridge
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;

 protected:
  std::shared_ptr<Device> device_;
  std::shared_ptr<Xxtea> xxtea_instance_;

  sensor::Sensor *battery_level_{nullptr};
  sensor::Sensor *temperature_{nullptr};
  binary_sensor::BinarySensor *problems_{nullptr};

  float visual_min_temp_{5.0f};
  float visual_max_temp_{35.0f};
  
  uint32_t last_update_{0};
  std::string pending_secret_key_;
};

} // namespace danfoss_eco
} // namespace esphome