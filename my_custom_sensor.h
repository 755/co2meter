#include "esphome.h"

class MyCustomSensor : public Component, public UARTDevice {
 public:
  MyCustomSensor(UARTComponent *parent) : UARTDevice(parent) {}

  Sensor *formaldehyde_sensor = new Sensor();

  void setup() override {
    // nothing to do here
  }
  void loop() override {
    const uint32_t now = millis();

  	if (now - last_transmission_ >= 500) {
      // last transmission too long ago. Reset RX index.
      data_index_ = 0;
    }

    if (available() == 0)
      return;
    
    last_transmission_ = now;

    while (available() != 0) {
      read_byte(&data_[data_index_]);
      auto check = check_byte_();
      if (!check.has_value()) {
        // finished
        parse_data_();
        data_index_ = 0;
      } else if (!*check) {
        // wrong data
        data_index_ = 0;
      } else {
        // next byte
        data_index_++;
      }
    }
  }

protected:
  uint8_t data_[64];
  uint8_t data_index_{0};
  uint32_t last_transmission_{0};
  
  void parse_data_() {
    const float ppm = ((uint16_t(data_[4]) << 8) | data_[5]) / 1000.0;
    ESP_LOGD("ze08", "ZE08-CH2O Received CH₂O=%.2fppm", ppm);
    formaldehyde_sensor->publish_state(ppm);
  }
  optional<bool> check_byte_() {
    uint8_t index = data_index_;
    uint8_t byte = data_[index];
    const uint8_t response_length = 9;


    if (index == 0)
      return byte == 0xFF;

    if (index == 1)
      return byte == 0x17;

    if (index < response_length - 1)
      return true;

    // checksum
    uint8_t sum = 0;
    for (uint8_t i = 1; i < response_length - 1; i++) {
      sum += data_[i];
    }
    sum = 0xFF - sum + 0x01;
    if (byte != sum) {
      ESP_LOGW("ze08", "ZE08-CH2O checksum mismatch! 0x%02X!=0x%02X", byte, sum);
      return false;
    }
    return {};
  }
};
