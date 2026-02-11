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
        uint8_t *temperatures = decrypt(this->xxtea_, raw_data, value_len);
        this->target_temperature = temperatures[0] / 2.0f;
        this->room_temperature = temperatures[1] / 2.0f;
    }

    void pack(uint8_t *buff) override {
        buff[0] = (uint8_t)(target_temperature * 2);
        buff[1] = (uint8_t)(room_temperature * 2);
        encrypt(this->xxtea_, buff, length);
    }
};

struct SettingsData : public WritableData {
    enum DeviceMode { MANUAL = 0, SCHEDULED = 1, VACATION = 3, HOLD = 5 };

    bool get_adaptable_regulation() { return parse_bit(settings_[0], 0); }
    bool get_vertical_intallation() { return parse_bit(settings_[0], 2); }
    bool get_display_flip() { return parse_bit(settings_[0], 3); }
    bool get_slow_regulation() { return parse_bit(settings_[0], 4); }
    bool get_valve_installed() { return parse_bit(settings_[0], 6); }
    bool get_lock_control() { return parse_bit(settings_[0], 7); }

    climate::ClimateMode device_mode;
    float temperature_min;
    float temperature_max;
    float frost_protection_temperature;
    float vacation_temperature;
    time_t vacation_from; 
    time_t vacation_to;   

    SettingsData(std::shared_ptr<Xxtea> &xxtea, uint8_t *raw_data, uint16_t value_len) : WritableData(16, xxtea) {
        uint8_t *decrypted = decrypt(this->xxtea_, raw_data, value_len);
        settings_.assign(decrypted, decrypted + length);

        this->temperature_min = settings_[1] / 2.0f;
        this->temperature_max = settings_[2] / 2.0f;
        this->frost_protection_temperature = settings_[3] / 2.0f;
        this->device_mode = to_climate_mode((DeviceMode)settings_[4]);
        this->vacation_temperature = settings_[5] / 2.0f;
        this->vacation_from = parse_int(settings_.data(), 6);
        this->vacation_to = parse_int(settings_.data(), 10);
    }

    climate::ClimateMode to_climate_mode(DeviceMode mode) {
        if (mode == SCHEDULED || mode == VACATION) return climate::CLIMATE_MODE_AUTO;
        return climate::CLIMATE_MODE_HEAT;
    }

    void pack(uint8_t *buff) override {
        memcpy(buff, settings_.data(), length);
        buff[1] = (uint8_t)(this->temperature_min * 2);
        buff[2] = (uint8_t)(this->temperature_max * 2);
        buff[3] = (uint8_t)(this->frost_protection_temperature * 2);
        buff[4] = (this->device_mode == climate::CLIMATE_MODE_AUTO) ? SCHEDULED : MANUAL;
        buff[5] = (uint8_t)(this->vacation_temperature * 2);
        write_int(buff, 6, this->vacation_from);
        write_int(buff, 10, this->vacation_to);
        encrypt(this->xxtea_, buff, length);
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
        uint16_t errors = parse_short(decrypt(this->xxtea_, raw_data, value_len), 0);
        E9_VALVE_DOES_NOT_CLOSE = parse_bit(errors, 8);
        E10_INVALID_TIME = parse_bit(errors, 9);
        E14_LOW_BATTERY = parse_bit(errors, 13);
        E15_VERY_LOW_BATTERY = parse_bit(errors, 14);
    }
};

} // namespace danfoss_eco
} // namespace esphome