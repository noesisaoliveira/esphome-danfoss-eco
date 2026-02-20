#pragma once

#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "my_component.h"
#include "device_data.h"
#include <memory>

namespace esphome {
namespace danfoss_eco {

using BLEClient = ble_client::BLEClient;

// UUIDs
static auto SERVICE_BATTERY = esp32_ble_tracker::ESPBTUUID::from_raw("0000180f-0000-1000-8000-00805f9b34fb");
static auto CHARACTERISTIC_BATTERY = esp32_ble_tracker::ESPBTUUID::from_raw("00002a19-0000-1000-8000-00805f9b34fb");
static auto SERVICE_SETTINGS = esp32_ble_tracker::ESPBTUUID::from_raw("10020000-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_PIN = esp32_ble_tracker::ESPBTUUID::from_raw("10020001-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_SETTINGS = esp32_ble_tracker::ESPBTUUID::from_raw("10020003-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_TEMPERATURE = esp32_ble_tracker::ESPBTUUID::from_raw("10020005-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_ERRORS = esp32_ble_tracker::ESPBTUUID::from_raw("10020009-2749-0001-0000-00805f9b042f");
static auto CHARACTERISTIC_SECRET_KEY = esp32_ble_tracker::ESPBTUUID::from_raw("1002000b-2749-0001-0000-00805f9b042f");

class DeviceProperty {
 public:
  uint16_t handle{0};
  esp32_ble_tracker::ESPBTUUID service_uuid;
  esp32_ble_tracker::ESPBTUUID characteristic_uuid;
  std::unique_ptr<DeviceData> data;

  DeviceProperty(MyComponent *component, std::shared_ptr<Xxtea> xxtea, 
                 esp32_ble_tracker::ESPBTUUID service_uuid, 
                 esp32_ble_tracker::ESPBTUUID characteristic_uuid)
      : component_(component), xxtea_(xxtea), service_uuid(service_uuid), characteristic_uuid(characteristic_uuid) {}

  virtual bool init_handle(BLEClient *client);
  virtual bool read_request(BLEClient *client);
  virtual void update_state(uint8_t *value, uint16_t value_len) = 0;

 protected:
  MyComponent *component_;
  std::shared_ptr<Xxtea> xxtea_;
};

class WritableProperty : public DeviceProperty {
 public:
  using DeviceProperty::DeviceProperty;
  virtual bool write_request(BLEClient *client);
  virtual bool write_request(BLEClient *client, uint8_t *data, uint16_t data_len);
  // Implementação vazia para permitir instanciar a classe para o PIN
  void update_state(uint8_t *value, uint16_t value_len) override {}
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
};

} // namespace danfoss_eco
} // namespace esphome