
# I2C Driver Validation Using RTC Module

## Objective

Validate the I2C driver APIs by interfacing the STM32F401RE with an RTC device and verifying correct communication through:

- RTC initialization
- Time write operation
- Time read operation
- BCD to decimal conversion
- Continuous RTC monitoring

---

# Test Setup

| Parameter | Value |
|------------|--------|
| MCU | STM32F401RE |
| RTC Slave Address | 0x68 |
| Interface | I2C1 |
| I2C Mode | Standard Mode |
| SCL Frequency | 100 kHz |

---

# APIs Used and Validation Status

| API | Purpose | Status |
|------|---------|--------|
| `GPIO_Configurations_SCL_SDA()` | Configure I2C GPIO pins | Pass |
| `I2C_Init()` | Initialize I2C peripheral | Pass |
| `I2C_Transmit_Buffer()` | Write RTC time data | Pass |
| `I2C_Receive_Buffer()` | Read RTC time data | Pass |

---

# Test Case 1: I2C Peripheral Initialization

## Purpose

Verify initialization of I2C peripheral and GPIO configuration.

### APIs Used

```c
GPIO_Configurations_SCL_SDA();
I2C_Init(&handle);
```

### Expected Result

I2C communication established successfully with RTC device.

### Status

- [ ] Pass


---

# Test Case 2: RTC Time Write

## Purpose

Verify transmission of time data to RTC registers.

### Registers Written

| Register | Address |
|-----------|----------|
| Seconds | 0x00 |
| Minutes | 0x01 |
| Hours | 0x02 |

### APIs Used

```c
I2C_Transmit_Buffer(I2C1, 0x00, buffer, 3, RTC_ADDR);
```

### Buffer Data Written

| Parameter | Value |
|------------|--------|
| Hours | 23 |
| Minutes | 10 |
| Seconds | 00 |

### Status

- [ ] Pass

---

# Test Case 3: RTC Time Read

## Purpose

Verify multi-byte reception from RTC registers.

### APIs Used

```c
I2C_Receive_Buffer(I2C1, 0x00, buffer, 3, RTC_ADDR);
```

### Registers Read

| Register | Address |
|-----------|----------|
| Seconds | 0x00 |
| Minutes | 0x01 |
| Hours | 0x02 |

### Expected Result

RTC returns valid BCD encoded time values.

### Status

- [ ] Pass

---

# Test Case 4: BCD to Decimal Conversion

## Purpose

Verify correct conversion of RTC BCD data into decimal values.

### Conversion Logic

```c
seconds = (((buffer[0] >> 4) * 10) + (buffer[0] & 0x0F));
minutes = (((buffer[1] >> 4) * 10) + (buffer[1] & 0x0F));
hours   = (((buffer[2] >> 4) * 10) + (buffer[2] & 0x0F));
```

### Expected Result

Displayed time matches the programmed RTC time and increments correctly.

### Status

- [ ] Pass

---

# Test Case 5: Continuous RTC Monitoring

## Purpose

Verify stable long-term I2C communication.

### Verification Method

Observe USART terminal output continuously.

### Sample Output

```text
Current Time: 23:10:00
Current Time: 23:10:01
Current Time: 23:10:02
Current Time: 23:10:03
```

### Expected Result

Seconds increment continuously without communication errors.

### Status

- [ ] Pass

---

# Test Coverage Summary

| Feature | Verification Status |
|-----------|-------------------|
| GPIO Configuration | Pass |
| I2C Peripheral Initialization | Pass |
| RTC Time Write | Pass |
| RTC Time Read | Pass |
| Multi-byte Data Transfer | Pass |
| BCD Encoding/Decoding | Pass |
| Continuous Communication | Pass |

---

# Overall Result

| Test Item | Status |
|------------|---------|
| I2C Initialization | Pass  |
| RTC Time Write | Pass|
| RTC Time Read | Pass |
| BCD Conversion | Pass|
| Continuous Monitoring | Pass |

## Final Verdict

**I2C Driver Validation with RTC Interface: PASS**

## Video link:
https://drive.google.com/file/d/1Zke7yg92_cRkQpAyuT1BJ1SlK_HgIIM1/view?usp=drivesdk