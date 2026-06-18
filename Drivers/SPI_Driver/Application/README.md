# BMP280 Temperature Sensor Interface using SPI

## 1. Objective

This application demonstrates communication between the STM32F401RE microcontroller and the BMP280 temperature sensor using the custom SPI driver. The program:

- Initializes SPI communication.
- Reads the BMP280 sensor ID.
- Reads factory calibration constants.
- Configures the sensor for temperature measurement.
- Reads raw temperature ADC values.
- Converts the raw ADC value into temperature using the BMP280 compensation algorithm.
- Displays results through USART.

---

# 2. Driver APIs Used

## 2.1 SPI_GPIO_Configurations()

### Purpose

Configures GPIO pins required for SPI communication.

### API Call

```c
SPI_GPIO_Configurations();
```

### Functionality

Configures:

| Pin | Function |
|------|----------|
| PA5 | SPI1_SCK |
| PA6 | SPI1_MISO |
| PA7 | SPI1_MOSI |
| PB6 | Software Slave Select |

### Abstraction Provided

The application does not directly manipulate GPIO registers. The driver configures alternate function modes, output types, speeds, and pull settings internally.

---

## 2.2 SPI_Init()

### Purpose

Initializes and configures the SPI peripheral.

### API Call

```c
SPI_Init(&spi1);
```

### Configuration Structure

```c
SPI_Handle_t spi1;

spi1.pSPIx = SPI1;
spi1.SPI_Config.SPI_Bus_Config = SPI_BUS_CONFIG_FD;
spi1.SPI_Config.SPI_Device_Mode = SPI_DEVICE_MODE_MASTER;
spi1.SPI_Config.SPI_CLK_Speed = SPI_PRESCALAR_16;
spi1.SPI_Config.SPI_SSM = SPI_SSM_EN;
spi1.SPI_Config.SPI_DataFrame = SPI_DFF_8BITS;
```

### Functionality

Configures:

- Master mode
- Full duplex communication
- Clock prescaler = 16
- Software slave management
- 8-bit data frame format

### Abstraction Provided

The application configures SPI using high-level parameters without directly accessing SPI control registers.

---

## 2.3 SPI_TransmitByte()

### Purpose

Transmits a single byte over SPI.

### API Call

```c
SPI_TransmitByte(SPI1, data);
```

### Usage Examples

#### Reading Sensor ID

```c
SPI_TransmitByte(SPI1, 0xD0);
```

#### Writing Control Register

```c
SPI_TransmitByte(SPI1, 0xF4);
SPI_TransmitByte(SPI1, 0x27);
```

### Functionality

The API:

1. Waits for TXE flag.
2. Writes data to SPI data register.
3. Waits until transmission completes.

### Abstraction Provided

Eliminates direct polling of SPI status flags and register handling.

---

## 2.4 SPI_ReceiveByte()

### Purpose

Receives a single byte from the SPI slave.

### API Call

```c
uint8_t sensor_id = SPI_ReceiveByte(SPI1);
```

### Usage

Used to read BMP280 chip ID.

### Functionality

The API:

1. Generates SPI clock.
2. Waits for RXNE flag.
3. Reads received data.

### Abstraction Provided

Handles receive synchronization internally.

---

## 2.5 SPI_Receive_Buffer()

### Purpose

Receives multiple bytes from the SPI slave.

### API Call

```c
SPI_Receive_Buffer(SPI1, calib_values, 6);
```

### Parameters

| Parameter | Description |
|------------|------------|
| SPI1 | SPI peripheral |
| calib_values | Receive buffer |
| 6 | Number of bytes |

### Usage

#### Reading Calibration Data

```c
SPI_Receive_Buffer(SPI1, calib_values, 6);
```

#### Reading ADC Temperature Data

```c
SPI_Receive_Buffer(SPI1, ADC_data, 3);
```

### Functionality

Receives multiple bytes continuously while generating SPI clock pulses.

### Abstraction Provided

Avoids manual looping and flag checking in the application layer.

---

## 2.6 USART_Send_String()

### Purpose

Sends ASCII strings over USART.

### API Call

```c
USART_Send_String("Sensor ID: ");
```

### Usage

Used for displaying:

- Sensor ID
- Calibration constants
- ADC values
- Temperature values

### Abstraction Provided

Handles character-by-character transmission internally.

---

## 2.7 USART_SendHex_Number()

### Purpose

Displays hexadecimal values.

### API Call

```c
USART_SendHex_Number(sensor_id);
```

### Example Output

```text
58
```

### Usage

Used for BMP280 chip identification.

---

## 2.8 USART_Send_Number()

### Purpose

Displays signed or unsigned 16-bit integer values.

### API Call

```c
USART_Send_Number(dig_t1);
USART_Send_Number(dig_t2);
USART_Send_Number(dig_t3);
```

### Usage

Displays calibration constants.

---

## 2.9 USART_Send32bit_Number()

### Purpose

Displays 32-bit integer values.

### API Call

```c
USART_Send32bit_Number(ADC_value);
```

### Usage

Displays:

- Raw ADC values
- Calculated temperature values

---

## 2.10 delay_ms()

### Purpose

Creates a blocking delay.

### API Call

```c
delay_ms(500);
```

### Usage

Provides a 500 ms interval between sensor readings.

### Abstraction Provided

Uses timer-based delay functionality without requiring direct timer register manipulation.

---

# 3. Sensor Reading Flow

## Step 1: Initialize Peripherals

```c
USART_Configuration();
Delay_Creation();
SPI_GPIO_Configurations();
SPI_Init(&spi1);
```

---

## Step 2: Read BMP280 Sensor ID

```c
SelectSlave();

SPI_TransmitByte(SPI1, 0xD0);

sensor_id = SPI_ReceiveByte(SPI1);

DeselectSlave();
```

Expected value:

```text
0x58
```

---

## Step 3: Read Calibration Constants

```c
SPI_TransmitByte(SPI1, 0x88);

SPI_Receive_Buffer(SPI1, calib_values, 6);
```

Read:

```text
dig_t1
dig_t2
dig_t3
```

These constants are required for temperature compensation.

---

## Step 4: Configure Sensor

```c
SPI_TransmitByte(SPI1, 0xF4);
SPI_TransmitByte(SPI1, 0x27);
```

Configuration:

| Field | Value |
|---------|--------|
| Temperature Oversampling | x1 |
| Pressure Oversampling | x1 |
| Mode | Normal |

---

## Step 5: Read Raw Temperature Data

```c
SPI_TransmitByte(SPI1, 0xFA);

SPI_Receive_Buffer(SPI1, ADC_data, 3);
```

Combine:

```c
ADC_value =
((uint32_t)ADC_data[0] << 12) |
((uint32_t)ADC_data[1] << 4)  |
((uint32_t)ADC_data[2] >> 4);
```

---

## Step 6: Temperature Compensation

Using calibration constants:

```c
int32_t var1;
int32_t var2;
int32_t t_fine;
int32_t T;
```

Temperature calculation follows the compensation formula specified in the BMP280 datasheet.

Result:

```text
Temperature = T / 100 °C
```

Example:

```text
Temperature : 28.45
```

---

# 4. Level of Abstraction Achieved

The application code remains focused on **sensor operations**, while the drivers handle hardware-specific details.

| Application Layer | Driver Layer |
|------------------|-------------|
| Read sensor ID | SPI register handling |
| Read calibration data | TXE/RXNE polling |
| Read ADC values | Data register access |
| Calculate temperature | Peripheral configuration |
| Display results | USART transmission |

This separation improves:

- Code readability
- Reusability
- Maintainability
- Portability across projects using the same drivers