#pragma once
#include <memory>
#include <vector>
#include "xxtea.h"
#include "esphome/components/climate/climate_mode.h"

namespace esphome {
namespace danfoss_eco {

/**
 * Base structure for data received from or sent to the valve.
 */
struct DeviceData {
  virtual ~DeviceData() = default;
};

/**
 * Interface for properties that can be written to (requires encryption).
 */
struct WritableData : public DeviceData {
  uint16_t length;
  std::shared_ptr<Xxtea> xxtea;
  WritableData(uint16_t len, std::shared_ptr<Xxtea> &xt) : length(len), xxtea(xt) {}
  virtual void pack(uint8_t *data) = 0;
};

/**
 * Handles Temperature and Target Setpoint (UUID 0005)
 */
struct TemperatureData : public WritableData {
  float room_temperature{0.0f};
  float target_temperature{0.0f};

  // Constructor for decoding data read from the device
  TemperatureData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : WritableData(8, xxtea) {
    if (value_len < 8) return;
    uint8_t decrypted[8];
    xxtea->decrypt(raw_data, 8, decrypted);
    
    // Danfoss uses 0.5°C units (value / 2)
    this->room_temperature = (float)decrypted[0] / 2.0f;
    this->target_temperature = (float)decrypted[1] / 2.0f;
  }

  // Constructor for creating a command to send to the device
  TemperatureData(std::shared_ptr<Xxtea> &xxtea) : WritableData(8, xxtea) {}

  void pack(uint8_t *data) override {
    uint8_t plain[8] = {0};
    plain[0] = (uint8_t)(this->room_temperature * 2);
    plain[1] = (uint8_t)(this->target_temperature * 2);
    // bytes 2-7 are usually padding or timestamp related in Danfoss protocol
    this->xxtea->encrypt(plain, 8, data);
  }
};

/**
 * Handles Device Settings and Min/Max limits (UUID 0003)
 */
struct SettingsData : public WritableData {
  float temperature_min{5.0f};
  float temperature_max{30.0f};
  climate::ClimateMode device_mode{climate::CLIMATE_MODE_HEAT};

  SettingsData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : WritableData(16, xxtea) {
    if (value_len < 16) return;
    uint8_t decrypted[16];
    xxtea->decrypt(raw_data, 16, decrypted);

    this->temperature_min = (float)decrypted[3] / 2.0f;
    this->temperature_max = (float)decrypted[4] / 2.0f;
    
    // Mode mapping: 0 = Manual (Heat), 1 = At Home (Auto), 2 = Vacation (Off/Eco)
    uint8_t mode = decrypted[0];
    if (mode == 0) this->device_mode = climate::CLIMATE_MODE_HEAT;
    else if (mode == 1) this->device_mode = climate::CLIMATE_MODE_AUTO;
    else this->device_mode = climate::CLIMATE_MODE_OFF;
  }

  SettingsData(std::shared_ptr<Xxtea> &xxtea) : WritableData(16, xxtea) {}

  void pack(uint8_t *data) override {
    uint8_t plain[16] = {0};
    plain[0] = (this->device_mode == climate::CLIMATE_MODE_AUTO) ? 1 : 0;
    plain[3] = (uint8_t)(this->temperature_min * 2);
    plain[4] = (uint8_t)(this->temperature_max * 2);
    this->xxtea->encrypt(plain, 16, data);
  }
};

/**
 * Decodes Error codes from the valve (UUID 0009)
 */
struct ErrorsData : public DeviceData {
  bool E9_VALVE_DOES_NOT_CLOSE{false};
  bool E10_INVALID_TIME{false};
  bool E14_LOW_BATTERY{false};
  bool E15_VERY_LOW_BATTERY{false};

  ErrorsData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) {
    if (value_len < 8) return;
    uint8_t decrypted[8];
    xxtea->decrypt(raw_data, 8, decrypted);

    // Bitwise error mapping for Danfoss Eco
    this->E9_VALVE_DOES_NOT_CLOSE = (decrypted[0] & 0x01);
    this->E10_INVALID_TIME = (decrypted[0] & 0x02);
    this->E14_LOW_BATTERY = (decrypted[1] & 0x01);
    this->E15_VERY_LOW_BATTERY = (decrypted[1] & 0x02);
  }
};

} // namespace danfoss_eco
} // namespace esphome