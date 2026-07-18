# ESPHome Component DFRobot C4001

<p align="center">
    <img src="https://dfimg.dfrobot.com/enshop/SEN0609/SEN0609_Main_01.jpg" width="50%"><br />
    C4001 (SEN0609) 25m mmWave Presence Sensor
</p>

There are two variants:

* SEN0609 has a 100° horizontal and 40° vertical field of view, 16 meter presence detection range and 25 meter motion detection range.
* SEN0610 has a 100° horizontal and 80° vertical field of view, 8 meter presence detection range and 12 meter motion detection range.

> [!NOTE]
> Some settings have different ranges depending on the variant used. This component treats both variants the same, so it is your responsibility to make sure your configuration sets these values appropriately.

The sensor can operate in one of two modes, `Presence` and `Speed and Distance` . In `Presence` mode the sensor provides a singular occupancy output. The presence output once presence is detected will stay on for a period that can be configured. In `Speed and Distance` mode the occupancy binary sensor indicates if a target is being tracked or not. Each time the sensor indicates presence it also outputs target distance, target speed and target energy. In `Speed and Distance` mode all of these parameter update frequently. There are only two settings for this mode, micro_motion_enable switch and threshold_factor number.

The C4001 sensor maintains settings in flash. When powered on these settings are loaded from flash and made operational. To change the configuration of the sensor dial in the setting you need and hit the config_save button. This will tell the sensor to store the new settings in flash and make them operational. You only need to do this once.

More information on the C4001 (SEN0609) sensor is available [here](https://www.dfrobot.com/product-2793.html). Information on the C4001 (SEN0610) sensor is available [here](https://www.dfrobot.com/product-2795.html).

```yaml
# Sample configuration entry example
external_components:
  - source:
      type: git
      url: https://github.com/mikelawrence/esphome-components-dfrobot-c4001

uart:
  - id: c4001_uart
    tx_pin: GPIO18
    rx_pin: GPIO17
    baud_rate: 9600
    
dfrobot_c4001:
  id: mmwave_sensor
  uart_id: mmwave_uart
  mode: PRESENCE
```

## Configuration Variables

* **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the `dfrobot_c4001` component. Required if there are multiple DFRobot C4001s configured.

* **uart_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID of the UART Component to use. Required if you have multiple UARTs configured.

* **mode** (*Required*, enumeration): This sets the operation mode of the sensor. Options are `PRESENCE` and `SPEED_AND_DISTANCE`.

## Buttons

The `dfrobot_c4001` button allows you to perform `config save` actions on your DFRobot C4001.

```yaml
button:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    config_save:
      name: Config Save
      entity_category: CONFIG
```

### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **config_save** (*Optional*): When you click this button the current configuration will be saved. Keep in mind that these are writes to flash and there is a limited number of time you can do this before the flash wears out. All Options from [Button Component](https://esphome.io/components/button/#base-button-configuration).
* **factory_reset** (*Optional*): Clicking this button will perform a factory reset of the module and all configuration values will go back to default. All Options from [Button Component](https://esphome.io/components/button/#base-button-configuration).
* **restart** (*Optional*): When you click this button the module will restart and all configuration values will remains as previously set. All Options from [Button Component](https://esphome.io/components/button/#base-button-configuration).

## Binary Sensors

The `dfrobot_c4001` binary sensor report `config changed` and `occupancy` state information about your DFRobot C4001.

```yaml
binary_sensor:
  - platform: dfrobot_c4001
    config_changed:
      name: Config Changed
    occupancy:
      id: occupancy
      name: Occupancy
```

### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **config_changed** (*Optional*): When `true` the current sensor configuration has been changed but not saved to the sensor. All Options from [Binary Sensor Component](https://esphome.io/components/binary_sensor/#base-binary-sensor-configuration).
* **occupancy** (*Optional*): In `PRESENCE` mode this indicates presence. In `SPEED_AND_DISTANCE` mode this indicates a target is being tracked. All Options from [Binary Sensor Component](https://esphome.io/components/binary_sensor/#base-binary-sensor-configuration).

## Numbers

The `dfrobot_c4001` button allows you to perform `config save` actions on your DFRobot C4001.

```yaml
number:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    max_range:
      name: Range Max
    min_range:
      name: Range Min
    trigger_range:
      name: Range Trigger
    hold_sensitivity:
      name: Sensitivity Hold
    trigger_sensitivity:
      name: Sensitivity Trigger
    on_latency:
      name: Latency On
    off_latency:
      name: Latency Off
    inhibit_time:
      name: Inhibit Time
```

### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **min_range** (*Optional*): This is the minimum detection range. Default is 0.6 meters (m) with a range of 0.6 to 25.0 m. The manual recommends not changing this value. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **max_range** (*Optional*): This is the maximum detection range. Default is 6 meters (m) with a range of 0.6 to 25.0 m. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **trigger_range** (*Optional*): Sets the maximum range at which occupancy can switch to present. The range between max detection range and trigger detection range can NOT cause occupancy to switch to present. Default is 0.6 meters (m) with a range of 0.6 to 25.0 m. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **hold_sensitivity** (*Optional*): The number represents the ease in which the sensor switches to the present state when someone enters the sensing range of the sensor. Default is 7 (no units) with a range of 0 to 9, higher is more sensitive. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **trigger_sensitivity** (*Optional*): This number represents ease of continued presence detection after the sensor switched to the present state. Default is 5 (no units) with a range of 0 to 9, higher is more sensitive. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **on_latency** (*Optional*): This time value is how long presence is detected before switching to the present state. Default is 0.050 (seconds) with a range of 0.0 to 100.0. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **off_latency** (*Optional*): This time value is how long the after the sensor no longer detects presence before switching to the not present state. Default is 15 (seconds) with a range of 0 to 1500. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **inhibit_time** (*Optional*): The dead-time after switching to the not present state before presence can be detected again. Default is 1 (seconds) with a range of 0.1 to 255.0. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `PRESENCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).
* **threshold_factor** (*Optional*): The larger the number the larger the object and more motion is required to trigger the sensor to switch to target tracked state. Default is 5 with a range of 0 to 65535. The `config_save` button must be clicked to save the sensor configuration to flash and make operational. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Number Component](https://esphome.io/components/number/#base-number-configuration).

## Sensors

The `dfrobot_c4001`  sensors report `software version` and `firmware version` from your DFRobot C4001.

```yaml
sensor:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    target_distance:
      name: Target Distance
    target_speed:
      name: Target Speed
    target_energy:
      name: Target Energy
```

### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **target_distance** (*Optional*): When **occupancy** binary sensor is `true` this sensor indicates distance to target in meters (m). When **occupancy** binary sensor is `false` this sensor switches to 0.0 indicating invalid data. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/#base-sensor-configuration).
* **target_speed** (*Optional*): When **occupancy** binary sensor is `true` this sensor indicates target speed in meters per second (m/s). When **occupancy** binary sensor is `false` this sensor switches to 0.0 indicating invalid data. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/#base-sensor-configuration).
* **target_energy** (*Optional*): When **occupancy** binary sensor is `true` this sensor indicates target energy in no units. When **occupancy** binary sensor is `false` this sensor switches to 0.0 indicating invalid data. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Sensor Component](https://esphome.io/components/sensor/#base-sensor-configuration).

## Switches

The `dfrobot_c4001` switch allows you to enable the output LED on your DFRobot C4001.

```yaml
switch:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    led_enable:
      name: Enable LED
```

### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **led_enable** (*Optional*): When turned on the green LED will flash when the sensor has been started. The blue LED cannot be disabled with this command. All Options from [Switch Component](https://esphome.io/components/switch/#base-switch-configuration).
* **micro_motion_enable** (*Optional*): Turns on micro motion mode. Available only in `SPEED_AND_DISTANCE` mode. All Options from [Switch Component](https://esphome.io/components/switch/#base-switch-configuration).

## Text Sensors

The `dfrobot_c4001` text sensors report `software version` and `firmware version` from your DFRobot C4001.

```yaml
text_sensor:
  - platform: dfrobot_c4001
    dfrobot_c4001_id: mmwave_sensor
    software_version:
      name: Software Version
    hardware_version:
      name: Hardware Version
```

### Configuration Variables

* **dfrobot_c4001_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types/#id)): Manually specify the ID for the DFRobot C4001 component. Required if there are multiple DFRobot C4001s configured.
* **software_version** (*Optional*): Software Version as reported by the module. All Options from [Text Sensor Component](https://esphome.io/components/sensor/#base-text-sensor-configuration).
* **firmware_version** (*Optional*): Firmware Version as reported by the module. All Options from [Text Sensor Component](https://esphome.io/components/text_sensor/#base-text-sensor-configuration).

## Actions

* **dfrobot_c4001.factory_reset** Will perform a factory reset of the module and all configuration values will go back to default. The module will restart with these defaults. Keep in mind that these are writes to flash and there is a limited number of time you can do this before the flash wears out. This is much easier to do with a lambda that accidentally performs a factory reset every second.

Example using automations...

```yaml
button:
  - platform: template
    name: Factory Reset
    on_press:
      - dfrobot_c4001.factory_reset: mmwave_sensor
    entity_category: CONFIG
```

Example in lambdas...

```yaml
- lambda: |-
  id(mmwave_sensor).factory_reset();
```

* **dfrobot_c4001.restart** Will restart the module and all configuration values will remains as previously set.

Example using automations...

```yaml
button:
  - platform: template
    name: Restart
    on_press:
      - dfrobot_c4001.restart: mmwave_sensor
    entity_category: CONFIG
```

Example in lambdas...

```yaml
- lambda: |-
  id(mmwave_sensor).restart();
```
