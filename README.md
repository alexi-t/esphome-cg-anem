Example usage

```
sensor:
- platform: cg_anem
  temperature:
    name: "temp_dorm"
  wind_speed:
    name: "speed_dorm"
  address: 0x11
  update_interval: 10s
  i2c_id: bus_a
- platform: cg_anem
  temperature:
    name: "temp_main"
  wind_speed:
    name: "speed_main"
  address: 0x11
  update_interval: 10s
  i2c_id: bus_b
```
