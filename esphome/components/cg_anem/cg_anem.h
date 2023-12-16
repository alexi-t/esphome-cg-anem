#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace cg_anem {

/// This class implements support for the GC Anem Temperature+Pressure+Humidity i2c sensor.
class CGAnemComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_speed_sensor(sensor::Sensor *speed_sensor) { speed_sensor_ = speed_sensor; }

  // ========== INTERNAL METHODS ==========
  // (In most use cases you won't need these)
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void update() override;

 protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *speed_sensor_{nullptr};

  enum ErrorCode {
    NONE = 0,
    COMMUNICATION_FAILED,
  } error_code_{NONE};

 private:
  void read_status();
};

}  // namespace cg_anem
}  // namespace esphome
