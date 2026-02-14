#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "properties.h"
#include "command.h"
#include <queue>

namespace esphome {
namespace danfoss_eco {

class Device {
 public:
  Device(MyComponent *parent, std::shared_ptr<Xxtea> xxtea) : parent_(parent), xxtea_(xxtea) {}
  
  void setup();
  void loop();
  void update();
  void control(const climate::ClimateCall &call);
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

  void set_pin_code(const std::string &str);
  void set_secret_key(const std::string &str);
  void set_secret_key(uint8_t *key, bool persist);

 protected:
  void write_pin();

  MyComponent *parent_;
  std::shared_ptr<Xxtea> xxtea_;
  std::vector<std::shared_ptr<DeviceProperty>> properties_;
  std::queue<Command*> commands_;
  
  uint32_t pin_code_{0};
  std::string pending_secret_key_;

  std::shared_ptr<WritableProperty> p_pin_;
  std::shared_ptr<BatteryProperty> p_battery_;
  std::shared_ptr<TemperatureProperty> p_temperature_;
  std::shared_ptr<SettingsProperty> p_settings_;
  std::shared_ptr<ErrorsProperty> p_errors_;
  std::shared_ptr<SecretKeyProperty> p_secret_key_;
};

} // namespace danfoss_eco
} // namespace esphome
