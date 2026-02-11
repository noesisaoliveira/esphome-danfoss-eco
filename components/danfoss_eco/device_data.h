#pragma once

#include "esphome/core/log.h"
#include "helpers.h"
#include "xxtea.h"
#include <vector>

namespace esphome {
namespace danfoss_eco {

struct DeviceData {
  uint16_t length;
  DeviceData(uint16_t l, std::shared_ptr<Xxtea> &xxtea) : length(l), xxtea_(xxtea) {}
  virtual ~DeviceData() = default;

 protected:
  std::shared_ptr<Xxtea> xxtea_;
};

struct WritableData : public DeviceData {
  WritableData(uint16_t l, std::shared_ptr<Xxtea> &xxtea) : DeviceData(l, xxtea) {}
  virtual void pack(uint8_t *) = 0;
};

struct TemperatureData : public WritableData {
  float target_temperature;
  float room_temperature;

  TemperatureData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : WritableData(8, xxtea) {
    if (this->xxtea_->decrypt(raw_data, value_len) == 0) { // 0 is Success in your xxtea.cpp
      uint8_t reversed[8];
      reverse_chunks(raw_data, 8, reversed);
      this->target_temperature = reversed[0] / 2.0f;
      this->room_temperature = reversed[1] / 2.0f;
    }
  }

  void pack(uint8_t *buff) override {
    uint8_t plain[8] = {0};
    plain[0] = (uint8_t)(target_temperature * 2);
    plain[1] = (uint8_t)(room_temperature * 2);
    uint8_t reversed[8];
    reverse_chunks(plain, 8, reversed);
    size_t out_len = 8;
    this->xxtea_->encrypt(reversed, 8, buff, &out_len);
  }
};

struct SettingsData : public WritableData {
  enum DeviceMode { MANUAL = 0, SCHEDULED = 1, VACATION = 3, HOLD = 5 };
  climate::ClimateMode device_mode;
  float temperature_min;
  float temperature_max;
  float frost_protection_temperature;
  float vacation_temperature;
  uint32_t vacation_from; 
  uint32_t vacation_to;   

  SettingsData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : WritableData(16, xxtea) {
    if (this->xxtea_->decrypt(raw_data, value_len) == 0) {
      uint8_t reversed[16];
      reverse_chunks(raw_data, 16, reversed);
      settings_.assign(reversed, reversed + 16);
      this->temperature_min = settings_[1] / 2.0f;
      this->temperature_max = settings_[2] / 2.0f;
      this->frost_protection_temperature = settings_[3] / 2.0f;
      this->device_mode = (settings_[4] == SCHEDULED || settings_[4] == VACATION) ? 
                          climate::CLIMATE_MODE_AUTO : climate::CLIMATE_MODE_HEAT;
      this->vacation_temperature = settings_[5] / 2.0f;
      this->vacation_from = parse_int(settings_.data(), 6);
      this->vacation_to = parse_int(settings_.data(), 10);
    }
  }

  void pack(uint8_t *buff) override {
    uint8_t plain[16] = {0};
    if (settings_.size() >= 16) memcpy(plain, settings_.data(), 16);
    plain[1] = (uint8_t)(this->temperature_min * 2);
    plain[2] = (uint8_t)(this->temperature_max * 2);
    plain[3] = (uint8_t)(this->frost_protection_temperature * 2);
    plain[4] = (this->device_mode == climate::CLIMATE_MODE_AUTO) ? SCHEDULED : MANUAL;
    plain[5] = (uint8_t)(this->vacation_temperature * 2);
    write_int(plain, 6, this->vacation_from);
    write_int(plain, 10, this->vacation_to);
    uint8_t reversed[16];
    reverse_chunks(plain, 16, reversed);
    size_t out_len = 16;
    this->xxtea_->encrypt(reversed, 16, buff, &out_len);
  }

 private:
  std::vector<uint8_t> settings_;
};

struct ErrorsData : public DeviceData {
  bool E9_VALVE_DOES_NOT_CLOSE;
  bool E10_INVALID_TIME;
  bool E14_LOW_BATTERY;
  bool E15_VERY_LOW_BATTERY;

  ErrorsData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : DeviceData(8, xxtea) {
    if (this->xxtea_->decrypt(raw_data, value_len) == 0) {
      uint8_t reversed[8];
      reverse_chunks(raw_data, 8, reversed);
      uint16_t errors = parse_short(reversed, 0);
      E9_VALVE_DOES_NOT_CLOSE = (errors >> 8) & 1;
      E10_INVALID_TIME = (errors >> 9) & 1;
      E14_LOW_BATTERY = (errors >> 13) & 1;
      E15_VERY_LOW_BATTERY = (errors >> 14) & 1;
    }
  }
};

} // namespace danfoss_eco
} // namespace esphome