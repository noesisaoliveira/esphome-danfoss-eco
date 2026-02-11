#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "my_component.h"
#include "device_data.h"
#include <memory>

namespace esphome {
namespace danfoss_eco {

// Namespace fix for 2026
using BLEClient = ble_client::BLEClient;

static auto SERVICE_SETTINGS = esp32_ble_tracker::ESPBTUUID::from_raw("10020000-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_PIN = esp32_ble_tracker::ESPBTUUID::from_raw("10020001-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_SETTINGS = esp32_ble_tracker::ESPBTUUID::from_raw("10020003-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_TEMPERATURE = esp32_ble_tracker::ESPBTUUID::from_raw("10020005-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_ERRORS = esp32_ble_tracker::ESPBTUUID::from_raw("10020009-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_SECRET_KEY = esp32_ble_tracker::ESPBTUUID::from_raw("1002000b-2749-0001-0000-00805f9b042f");

static auto SERVICE_BATTERY = esp32_ble_tracker::ESPBTUUID::from_uint32(0x180F);
static auto CHARACTERISTIC_BATTERY = esp32_ble_tracker::ESPBTUUID::from_uint32(0x2A19);

const uint16_t INVALID_HANDLE_VAL = 0xFFFF;

class DeviceProperty {
 public:
  std::unique_ptr<DeviceData> data{nullptr};
  uint16_t handle{INVALID_HANDLE_VAL};

  DeviceProperty(std::shared_ptr<MyComponent> component, std::shared_ptr<Xxtea> xxtea, 
                 esp32_ble_tracker::ESPBTUUID s_uuid, esp32_ble_tracker::ESPBTUUID c_uuid) 
      : component_(component), xxtea_(xxtea), service_uuid(s_uuid), characteristic_uuid(c_uuid) {}

  virtual void update_state(uint8_t *value, uint16_t value_len){};
  virtual bool init_handle(BLEClient *client);
  bool read_request(BLEClient *client);

 protected:
  std::shared_ptr<MyComponent> component_;
  std::shared_ptr<Xxtea> xxtea_;
  esp32_ble_tracker::ESPBTUUID service_uuid;
  esp32_ble_tracker::ESPBTUUID characteristic_uuid;
};

class WritableProperty : public DeviceProperty {
 public:
  using DeviceProperty::DeviceProperty;
  bool write_request(BLEClient *client);
  bool write_request(BLEClient *client, uint8_t *data, uint16_t data_len);
};

class BatteryProperty : public DeviceProperty {
 public:
  BatteryProperty(std::shared_ptr<MyComponent> component, std::shared_ptr<Xxtea> xxtea) 
      : DeviceProperty(component, xxtea, SERVICE_BATTERY, CHARACTERISTIC_BATTERY) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
};

class TemperatureProperty : public WritableProperty {
 public:
  TemperatureProperty(std::shared_ptr<MyComponent> component, std::shared_ptr<Xxtea> xxtea) 
      : WritableProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_TEMPERATURE) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
};

class SettingsProperty : public WritableProperty {
 public:
  SettingsProperty(std::shared_ptr<MyComponent> component, std::shared_ptr<Xxtea> xxtea) 
      : WritableProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_SETTINGS) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
};

class ErrorsProperty : public DeviceProperty {
 public:
  ErrorsProperty(std::shared_ptr<MyComponent> component, std::shared_ptr<Xxtea> xxtea) 
      : DeviceProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_ERRORS) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
};

class SecretKeyProperty : public DeviceProperty {
 public:
  SecretKeyProperty(std::shared_ptr<MyComponent> component, std::shared_ptr<Xxtea> xxtea) 
      : DeviceProperty(component, xxtea, SERVICE_SETTINGS, CHARACTERISTIC_SECRET_KEY) {}
  void update_state(uint8_t *value, uint16_t value_len) override;
  bool init_handle(BLEClient *client) override;
};

} // namespace danfoss_eco
} // namespace esphome