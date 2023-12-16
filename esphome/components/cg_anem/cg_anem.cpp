#include "cg_anem.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cg_anem {

static const char *const TAG = "cg_anem.sensor";

static const uint8_t CG_ANEM_REGISTER_VERSION = 0x04;
static const uint8_t CG_ANEM_REGISTER_WHO_I_AM = 0x05;
static const uint8_t CG_ANEM_REGISTER_STATUS = 0x06;

static const uint8_t CG_ANEM_REGISTER_WIND = 0x07;
static const uint8_t CG_ANEM_REGISTER_COLD_RAW = 0x09;
static const uint8_t CG_ANEM_REGISTER_HOT_RAW = 0x0B;

static const uint8_t CG_ANEM_REGISTER_VIN = 0x0D;
static const uint8_t CG_ANEM_REGISTER_HEAT_WT = 0x0E;

static const uint8_t CG_ANEM_REGISTER_COLD = 0x10;
static const uint8_t CG_ANEM_REGISTER_HOT = 0x12;
static const uint8_t CG_ANEM_REGISTER_DT = 0x14;

static const uint8_t CG_ANEM_REGISTER_ADDRESS = 0x20;

static const uint8_t CG_ANEM_REGISTER_WIND_MAX = 0x21;
static const uint8_t CG_ANEM_REGISTER_WIND_MIN = 0x23;
static const uint8_t CG_ANEM_REGISTER_RESET_WIND = 0x25;

static const uint8_t CG_ANEM_STATUS_INCORRECT_TARING_RANGE = 0b10000000;
static const uint8_t CG_ANEM_STATUS_INCORRECT_TARING = 0b01000000;
static const uint8_t CG_ANEM_STATUS_WATCHDOG_TIMER = 0b00100000;
static const uint8_t CG_ANEM_STATUS_OVERVOLTAGE = 0b00000010;
static const uint8_t CG_ANEM_STATUS_UNSTEADY_PROCESS = 0b00000001;

inline uint16_t combine_bytes(uint8_t msb, uint8_t lsb) { return ((msb & 0xFF) << 8) | (lsb & 0xFF); }

void CGAnemComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up CG Anem...");
  uint8_t chip_id = 0;

  // Mark as not failed before initializing. Some devices will turn off sensors to save on batteries
  // and when they come back on, the COMPONENT_STATE_FAILED bit must be unset on the component.
  if ((this->component_state_ & COMPONENT_STATE_MASK) == COMPONENT_STATE_FAILED) {
    this->component_state_ &= ~COMPONENT_STATE_MASK;
    this->component_state_ |= COMPONENT_STATE_CONSTRUCTION;
  }

  if (!this->read_byte(CG_ANEM_REGISTER_WHO_I_AM, &chip_id)) {
    this->error_code_ = COMMUNICATION_FAILED;
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "Id: %d", chip_id);

  uint8_t versionRaw = 0;

  if (!this->read_byte(CG_ANEM_REGISTER_VERSION, &versionRaw)) {
    this->error_code_ = COMMUNICATION_FAILED;
    this->mark_failed();
    return;
  }

  float version = versionRaw / 10.0;
  ESP_LOGI(TAG, "Version: %.1f", version);

  if (version >= 1.0f) {
    // Send a max wind reset soft reset.
    // if (!this->write_byte(CG_ANEM_REGISTER_RESET_WIND, 0x01)) {
    //   this->mark_failed();
    //   return;
    // }
  }

  this->read_status();
}

void CGAnemComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "CGANEM:");
  LOG_I2C_DEVICE(this);
  switch (this->error_code_) {
    case COMMUNICATION_FAILED:
      ESP_LOGE(TAG, "Communication with CG Anem failed!");
      break;
    case NONE:
    default:
      break;
  }

  LOG_UPDATE_INTERVAL(this);

  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Pressure", this->speed_sensor_);
}
float CGAnemComponent::get_setup_priority() const { return setup_priority::DATA; }

void CGAnemComponent::read_status() {
  uint8_t status;
  if (!this->read_byte(CG_ANEM_REGISTER_STATUS, &status)) {
    ESP_LOGW(TAG, "Error reading status register.");
    this->status_set_warning();
    return;
  }

  if (status & CG_ANEM_STATUS_INCORRECT_TARING_RANGE) {
    ESP_LOGW(TAG, "Incorrect taring range detected");
  }

  if (status & CG_ANEM_STATUS_INCORRECT_TARING) {
    ESP_LOGW(TAG, "Incorrect taring detected");
  }

  if ((status & CG_ANEM_STATUS_WATCHDOG_TIMER) == 0) {
    ESP_LOGW(TAG, "Watchdog disabled");
  }

  if (status & CG_ANEM_STATUS_OVERVOLTAGE) {
    ESP_LOGW(TAG, "Overvoltage detected");
  }

  if (status & CG_ANEM_STATUS_UNSTEADY_PROCESS) {
    ESP_LOGW(TAG, "Unsteady process detected");
    this->status_set_warning();
  } else {
    this->status_clear_warning();
  }
}

void CGAnemComponent::update() {
  this->read_status();

  if (this->status_has_warning()) {
    return;
  }

  uint16_t tempRaw;
  if (auto tempH = this->read_byte(CG_ANEM_REGISTER_COLD)) {
    if (auto tempL = this->read_byte(CG_ANEM_REGISTER_COLD + 1)) {
      tempRaw = (*tempH << 8) | *tempL;
    } else {
      ESP_LOGW(TAG, "Error reading cold temp.");
      this->status_set_warning();
      return;
    }
  } else {
    ESP_LOGW(TAG, "Error reading cold temp.");
    this->status_set_warning();
    return;
  }

  uint16_t speedRaw;
  if (auto speedH = this->read_byte(CG_ANEM_REGISTER_WIND)) {
    if (auto speedL = this->read_byte(CG_ANEM_REGISTER_WIND + 1)) {
      speedRaw = (*speedH << 8) | *speedL;
    } else {
      ESP_LOGW(TAG, "Error reading wind speed.");
      this->status_set_warning();
      return;
    }
  } else {
    ESP_LOGW(TAG, "Error reading wind speed.");
    this->status_set_warning();
    return;
  }

  float temp = tempRaw / 10.0f;
  float speed = speedRaw / 10.0f;

  ESP_LOGV(TAG, "Got temperature=%.1f°C pressure=%.1fm/s ", temp, speed);

  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(temp);
  if (this->speed_sensor_ != nullptr)
    this->speed_sensor_->publish_state(speed);

  this->status_clear_warning();
}

}  // namespace cg_anem
}  // namespace esphome
