#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "properties.h"
#include <queue>

namespace esphome {
namespace danfoss_eco {

enum class CommandType { READ, WRITE };

struct Command {
  CommandType type;
  std::shared_ptr<DeviceProperty> property;
  Command(CommandType t, std::shared_ptr<DeviceProperty> p) : type(t), property(p) {}
  
  bool execute(ble_client::BLEClient *client) {
    if (property->handle == 0xFFFF) return false;
    if (type == CommandType::READ) {
      return property->read_request(client);
    } else {
      auto writable = std::dynamic_pointer_cast<WritableProperty>(property);
      return writable ? writable->write_request(client) : false;
    }
  }
};

class Device : public std::enable_shared_from_this<Device> {
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
  
  void on_read(esp_ble_gattc_cb_param_t::gattc_read_char_evt_param param);
  void on_write(esp_ble_gattc_cb_param_t::gattc_write_evt_param param);

  MyComponent *parent_;
  std::shared_ptr<Xxtea> xxtea_;
  std::vector<std::shared_ptr<DeviceProperty>> properties_;
  std::queue<Command*> commands_;
  
  uint32_t pin_code_{0};
  uint8_t node_state_{0};

  // Fixed variable names (adding underscores to match private members)
  std::shared_ptr<WritableProperty> p_pin_;
  std::shared_ptr<BatteryProperty> p_battery_;
  std::shared_ptr<TemperatureProperty> p_temperature_;
  std::shared_ptr<SettingsProperty> p_settings_;
  std::shared_ptr<ErrorsProperty> p_errors_;
  std::shared_ptr<SecretKeyProperty> p_secret_key_;
};

} // namespace danfoss_eco
} // namespace esphome