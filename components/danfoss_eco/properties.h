#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "my_component.h"
#include "device_data.h"
#include <memory>

namespace esphome {
namespace danfoss_eco {

using BLEClient = ble_client::BLEClient;

// Use a simple enum for manual type checking since RTTI is disabled
enum PropertyType { TYPE_READ_ONLY, TYPE_WRITABLE };

class DeviceProperty {
 public:
  std::unique_ptr<DeviceData> data{nullptr};
  uint16_t handle{0xFFFF};
  PropertyType prop_type{TYPE_READ_ONLY};

  DeviceProperty(MyComponent *component, std::shared_ptr<Xxtea> xxtea, 
                 esp32_ble_tracker::ESPBTUUID s_uuid, esp32_ble_tracker::ESPBTUUID c_uuid) 
      : component_(component), xxtea_(xxtea), service_uuid(s_uuid), characteristic_uuid(c_uuid) {}

  virtual void update_state(uint8_t *value, uint16_t value_len){};
  virtual bool init_handle(BLEClient *client);
  bool read_request(BLEClient *client);

 protected:
  MyComponent *component_;
  std::shared_ptr<Xxtea> xxtea_;
  esp32_ble_tracker::ESPBTUUID service_uuid;
  esp32_ble_tracker::ESPBTUUID characteristic_uuid;
};

class WritableProperty : public DeviceProperty {
 public:
  WritableProperty(MyComponent *component, std::shared_ptr<Xxtea> xxtea, 
                   esp32_ble_tracker::ESPBTUUID s_uuid, esp32_ble_tracker::ESPBTUUID c_uuid) 
      : DeviceProperty(component, xxtea, s_uuid, c_uuid) {
      this->prop_type = TYPE_WRITABLE;
  }
  bool write_request(BLEClient *client);
  bool write_request(BLEClient *client, uint8_t *data, uint16_t data_len);
};

class BatteryProperty : public DeviceProperty {
 public:
  BatteryProperty(MyComponent *component, std::shared_ptr<Xxtea> xxtea) 
      : DeviceProperty(component, xxtea, SERVICE_BATTERY, CHARACTERISTIC_BATTERY) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
};

class TemperatureProperty : public WritableProperty {
 public:
  TemperatureProperty(MyComponent *component, std::shared_ptr<Xxtea> xxtea) 
      : WritableProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_TEMPERATURE) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
};

class SettingsProperty : public WritableProperty {
 public:
  SettingsProperty(MyComponent *component, std::shared_ptr<Xxtea> xxtea) 
      : WritableProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_SETTINGS) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
};

class ErrorsProperty : public DeviceProperty {
 public:
  ErrorsProperty(MyComponent *component, std::shared_ptr<Xxtea> xxtea) 
      : DeviceProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_ERRORS) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
};

class SecretKeyProperty : public DeviceProperty {
 public:
  SecretKeyProperty(MyComponent *component, std::shared_ptr<Xxtea> xxtea) 
      : DeviceProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_SECRET_KEY) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
  bool init_handle(BLEClient *client) override;
};

} // namespace danfoss_eco
} // namespace esphome