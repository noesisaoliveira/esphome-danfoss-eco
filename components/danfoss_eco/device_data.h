#pragma once
#include <memory>
#include <vector>
#include "xxtea.h"
#include "esphome/components/climate/climate_mode.h"

namespace esphome {
namespace danfoss_eco {

struct DeviceData { virtual ~DeviceData() = default; };

struct WritableData : public DeviceData {
  uint16_t length;
  std::shared_ptr<Xxtea> xxtea;
  WritableData(uint16_t len, std::shared_ptr<Xxtea> &xt) : length(len), xxtea(xt) {}
  virtual void pack(uint8_t *data) = 0;
};

struct TemperatureData : public WritableData {
  float room_temperature{0.0f};
  float target_temperature{0.0f};

  TemperatureData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : WritableData(8, xxtea) {
    if (value_len < 8) return;
    uint8_t decrypted[8];
    memcpy(decrypted, raw_data, 8);
    this->xxtea->decrypt(decrypted, 8);
    
    // De acordo com o Python data_struct: 
    // offset 0: set_point, offset 1: room_temp
    this->target_temperature = decrypted[0] / 2.0f;
    this->room_temperature = decrypted[1] / 2.0f;
  }

  void pack(uint8_t *data) override {
    uint8_t plain[8] = {0};
    plain[0] = (uint8_t)(this->target_temperature * 2);
    this->xxtea->encrypt(plain, 8, data);
  }
};

struct SettingsData : public WritableData {
  climate::ClimateMode device_mode;
  SettingsData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : WritableData(16, xxtea) {
    if (value_len < 16) return;
    uint8_t decrypted[16];
    memcpy(decrypted, raw_data, 16);
    this->xxtea->decrypt(decrypted, 16);
    this->device_mode = (decrypted[0] <= 1) ? climate::CLIMATE_MODE_HEAT : climate::CLIMATE_MODE_OFF;
  }
  void pack(uint8_t *data) override {}
};

struct ErrorsData : public DeviceData {
  bool E9_VALVE_DOES_NOT_CLOSE{false};
  bool E14_LOW_BATTERY{false};
  ErrorsData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) {
    if (value_len < 8) return;
    uint8_t dec[8]; memcpy(dec, raw_data, 8);
    xxtea->decrypt(dec, 8);
    this->E9_VALVE_DOES_NOT_CLOSE = (dec[0] & 0x01);
    this->E14_LOW_BATTERY = (dec[1] & 0x01);
  }
};

} // namespace danfoss_eco
} // namespace esphome