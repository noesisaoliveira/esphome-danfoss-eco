#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace danfoss_eco {

class MyComponent : public climate::Climate, public PollingComponent, public std::enable_shared_from_this<MyComponent> {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }

  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    
    // Using the simplified scoped enum names for current ESPHome versions
    traits.add_feature_flags(climate::CURRENT_TEMPERATURE);
    traits.add_feature_flags(climate::ACTION);

    traits.set_supported_modes({
        climate::CLIMATE_MODE_HEAT, 
        climate::CLIMATE_MODE_AUTO
    });
    
    traits.set_visual_temperature_step(0.5);
    return traits;
  }

  void set_battery_level(sensor::Sensor *battery_level) { battery_level_ = battery_level; }
  void set_temperature(sensor::Sensor *temperature) { temperature_ = temperature; }
  void set_problems(binary_sensor::BinarySensor *problems) { problems_ = problems; }

  virtual void set_secret_key(uint8_t *key, bool persist) = 0;

 protected:
  sensor::Sensor *battery_level_{nullptr};
  sensor::Sensor *temperature_{nullptr};
  binary_sensor::BinarySensor *problems_{nullptr};
};

} // namespace danfoss_eco
} // namespace esphome