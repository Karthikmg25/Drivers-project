
# SPI Driver Validation Using BMP280 Sensor

## Objective

Validate the SPI driver APIs by interfacing the STM32F401RE with a BMP280 sensor and verifying correct communication through:

- Sensor ID read
- Calibration data read
- Register write operation
- Raw temperature ADC read

---

# Test Setup

| Parameter | Value |
|------------|--------|
| MCU | STM32F401RE |
| Sensor | BMP280 |
| Interface | SPI1 |
| SPI Mode | Master |
| Data Frame Format | 8-bit |
| Clock Prescaler | 16 |
| Software Slave Management (SSM) | Enabled |

---

# APIs Used and Validation Status
| API | Purpose | Status |
|------|---------|--------|
| `SPI_GPIO_Configurations()` | Configure SPI GPIO pins | Pass |
| `SPI_Init()` | Initialize SPI peripheral | Pass |
| `SPI_TransmitByte()` | Send single byte through SPI | Pass |
| `SPI_ReceiveByte()` | Receive single byte through SPI | Pass |
| `SPI_Receive_Buffer()` | Receive multiple bytes through SPI | Pass |
| `SPI_TransmitByte()` | Write register value to BMP280 | Pass |
| `SPI_Receive_Buffer()` | Read ADC temperature data from BMP280 | Pass |

# Test Case 1: BMP280 Sensor ID Read

## Purpose

Verify SPI transmit and receive operations using BMP280 chip identification register.

## Procedure

1. Pull CS low.
2. Send register address `0xD0`.
3. Read one byte.
4. Pull CS high.

### Code Snippet

```c
SelectSlave();

SPI_TransmitByte(SPI1, 0xD0);
uint8_t sensor_id = SPI_ReceiveByte(SPI1);

DeselectSlave();
```

### Expected Result

```text
Sensor ID = 0x58
```

### Actual Result

```text
Sensor ID = 0x58
```

### Status

- [ ] Pass


---

# Test Case 2: Calibration Data Read

## Purpose

Verify multi-byte SPI reception.

## Registers Read

| Parameter | Address Range |
|------------|---------------|
| dig_t1 | 0x88 - 0x89 |
| dig_t2 | 0x8A - 0x8B |
| dig_t3 | 0x8C - 0x8D |

### Code Snippet

```c
uint8_t calib_values[6];

SelectSlave();

SPI_TransmitByte(SPI1, 0x88);
SPI_Receive_Buffer(SPI1, calib_values, 6);

DeselectSlave();
```

### Expected Result

Six bytes are received correctly and converted into valid calibration constants.

### Actual Result

| Parameter | Value |
|------------|--------|
| dig_t1 |27102 |
| dig_t2 | 26702 |
| dig_t3 | 64536 |

### Status

- [ ] Pass

---

# Test Case 3: BMP280 Configuration Write

## Purpose

Verify SPI write operation.

## Register

| Register | Address | Value Written |
|-----------|----------|---------------|
| control meas | 0xF4 | 0x27 |

### Code Snippet

```c
SelectSlave();

SPI_TransmitByte(SPI1, 0xF4);
SPI_TransmitByte(SPI1, 0x27);

DeselectSlave();
```

### Expected Result

BMP280 enters normal measurement mode and starts generating ADC values.

### Status

- [ ] Pass

---

# Test Case 4: Raw Temperature ADC Read

## Purpose

Verify repeated SPI transactions and multi-byte reception.

## Registers Read

| Register | Address |
|-----------|----------|
| TEMP_MSB | 0xFA |
| TEMP_LSB | 0xFB |
| TEMP_XLSB | 0xFC |

### Code Snippet

```c
uint8_t ADC_data[3];

SelectSlave();

SPI_TransmitByte(SPI1, 0xFA);
SPI_Receive_Buffer(SPI1, ADC_data, 3);

DeselectSlave();

uint32_t ADC_value =
((uint32_t)ADC_data[0] << 12) |
((uint32_t)ADC_data[1] << 4)  |
((uint32_t)ADC_data[2] >> 4);
```

### Expected Result

Valid 20-bit ADC value received continuously.

### Actual Result

```text
ADC Value = 524288
```

### Status

- [ ] Pass

---

# Temperature Calculation Validation

## Purpose

Verify correct processing of calibration data and raw ADC value.

### Formula Used

```c
int32_t var1 = (((((int32_t)ADC_value >> 3) -
                 ((int32_t)dig_t1 << 1))) *
                 ((int32_t)dig_t2)) >> 11;

int32_t var2 = ((((((int32_t)ADC_value >> 4) -
                  (int32_t)dig_t1) *
                 (((int32_t)ADC_value >> 4) -
                  (int32_t)dig_t1)) >> 12) *
                 (int32_t)dig_t3) >> 14;

int32_t t_fine = var1 + var2;
int32_t T = (t_fine * 5 + 128) >> 8;
```

### Expected Result

Temperature value should be within the ambient room temperature range.

### Actual Result

```text
Temperature = 28.76 °C
```

### Status

- [ ] Pass

---

# Test Coverage Summary

| Feature | Verification Status |
|-----------|-------------------|
| SPI GPIO Configuration | Pass |
| SPI Peripheral Initialization | Pass |
| Single Byte Transmission | Pass |
| Single Byte Reception | Pass |
| Multi-byte Reception | Pass |
| Register Write Operation | Pass |
| Continuous Sensor Read | Pass |
| Calibration Data Read | Pass |
| Temperature Calculation | Pass |

---

# Overall Result

| Test Item | Status |
|------------|---------|
| Sensor ID Read | Pass |
| Calibration Read | Pass |
| Register Write | Pass |
| ADC Read | Pass  |
| Temperature Calculation | Pass |

## Final Verdict

**SPI Driver Validation with BMP280: PASS**