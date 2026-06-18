# BMP280 SPI Temperature Reading Application – Test Cases

## Positive Test Cases

| Test Case ID | Description | Test Steps | Expected Result | Status |
|-------------|-------------|-------------|-----------------|---------|
| TC_POS_01 | Verify BMP280 Sensor ID Read | Power up STM32 and BMP280. Execute application. | Sensor ID read from register `0xD0` should be `0x58`. | Pass |
| TC_POS_02 | Verify Calibration Data Read | Read calibration registers `0x88–0x8D`. | Valid non-zero values obtained for `dig_t1`, `dig_t2`, and `dig_t3`. | Pass |
| TC_POS_03 | Verify Sensor Configuration | Write `0x27` to control register `0xF4` and read back the register. | Register contains `0x27` indicating successful configuration. | Pass |
| TC_POS_04 | Verify Raw ADC Temperature Read | Read registers `0xFA–0xFC` continuously. | ADC value should vary with ambient temperature and remain within valid 20-bit range (`0–1048575`). | Pass |
| TC_POS_05 | Verify Temperature Compensation | Apply BMP280 compensation formula using calibration constants and ADC value. | Temperature displayed on USART matches room temperature within expected tolerance. | Pass |

---

## Negative Test Cases

| Test Case ID | Description | Test Steps | Expected Result | Status |
|-------------|-------------|-------------|-----------------|---------|
| TC_NEG_01 | Sensor Disconnected | Disconnect BMP280 and execute application. | Sensor ID is invalid or communication fails. Application should indicate incorrect sensor response. | Pass |
| TC_NEG_02 | Incorrect Register Address | Attempt to read from an invalid register address. | Returned data should not match expected sensor values. | Pass |
| TC_NEG_03 | SPI Clock Too High | Configure SPI with unsupported clock speed and read sensor data. | Corrupted or inconsistent data may be observed. Communication validation should fail. | Pass |
| TC_NEG_04 | Incorrect Slave Selection | Do not assert chip select (CS) before SPI transaction. | BMP280 does not respond and data read is invalid. | Pass |
| TC_NEG_05 | Corrupted Calibration Data | Force calibration constants to zero or invalid values in software. | Temperature calculation produces unrealistic values, indicating compensation failure. | Pass |

---

## Test Summary

| Parameter | Value |
|-----------|-------|
| Total Positive Test Cases | 5 |
| Total Negative Test Cases | 5 |
| Total Test Cases | 10 |

### Result

BMP280 SPI interface, calibration data acquisition, raw ADC reading, and temperature compensation were verified successfully under both normal and error conditions.

Vedio link: https://drive.google.com/file/d/1U_c-zWKOIbS-a5m3oJlZNQmRadjsM4tI/view?usp=drivesdk