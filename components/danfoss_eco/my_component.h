#pragma once

#include <set>  // FIX: Critical for std::set
#include <memory>
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/climate/climate.h"

#include "helpers.h"

namespace esphome
{
    namespace danfoss_eco
    {
        using namespace esphome::climate;
        using namespace esphome::sensor;
        using namespace esphome::binary_sensor;

        class MyComponent : public Climate, public PollingComponent, public std::enable_shared_from_this<MyComponent>
        {
        public:
            float get_setup_priority() const override { return setup_priority::DATA; }

            ClimateTraits traits() override
            {
                auto traits = ClimateTraits();
                
                // FIX: Modern feature flag system for 2026
                traits.add_feature_flags(CLIMATE_FEAT_CURRENT_TEMPERATURE);
                traits.add_feature_flags(CLIMATE_FEAT_ACTION);

                // FIX: Explicit std::set namespace
                traits.set_supported_modes(std::set<ClimateMode>({
                    ClimateMode::CLIMATE_MODE_HEAT, 
                    ClimateMode::CLIMATE_MODE_AUTO
                }));
                
                traits.set_visual_temperature_step(0.5);
                
                return traits;
            }

            // FIX: Added these methods to resolve the "no member named" errors in properties.cpp
            void set_visual_min_temperature_override(float temp) {
                this->visual_min_temperature_override_ = temp;
            }
            void set_visual_max_temperature_override(float temp) {
                this->visual_max_temperature_override_ = temp;
            }

            void set_battery_level(Sensor *battery_level) { battery_level_ = battery_level; }
            void set_temperature(Sensor *temperature) { temperature_ = temperature; }
            void set_problems(BinarySensor *problems) { problems_ = problems; }

            Sensor *battery_level() { return this->battery_level_; }
            Sensor *temperature() { return this->temperature_; }
            BinarySensor *problems() { return this->problems_; }

            virtual void set_secret_key(uint8_t *, bool) = 0;

        protected:
            Sensor *battery_level_{nullptr};
            Sensor *temperature_{nullptr};
            BinarySensor *problems_{nullptr};

            // Helpers for the temperature overrides
            optional<float> visual_min_temperature_override_{};
            optional<float> visual_max_temperature_override_{};
        };

    } // namespace danfoss_eco
} // namespace esphome