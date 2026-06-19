# STM32F401RE Bare-Metal Driver Study Material
## I²C & SPI Driver APIs + Application Development Guide

> **Source repository analyzed:** `https://github.com/Karthikmg25/Drivers-project`
>
> **Target hardware:** STM32F401RE (Nucleo-F401RE) — Cortex-M4, 84 MHz max, APB1 bus
> **Style:** Bare-metal register-level drivers (no HAL, no LL, no CMSIS-drivers)
> **Mode:** Polling (blocking) for both I²C and SPI

---

## Table of Contents

1. [Introduction & Project Layout](#1-introduction--project-layout)
2. [STM32F401RE Memory Map & Peripheral Access Primer](#2-stm32f401re-memory-map--peripheral-access-primer)
3. [I²C Driver — Theory + API](#3-i2c-driver--theory--api)
   - 3.1 I²C Protocol Refresher
   - 3.2 STM32F4 I²C Peripheral & Register Map
   - 3.3 Project's I²C Layered Architecture
   - 3.4 `I2C_Baremetal` — Low-Level Helpers
   - 3.5 `STM32F401RE_I2C_Driver` — High-Level APIs
   - 3.6 I²C GPIO Configuration
4. [I²C Application — RTC (DS1307) Walk-Through](#4-i2c-application--rtc-ds1307-walk-through)
5. [SPI Driver — Theory + API](#5-spi-driver--theory--api)
   - 5.1 SPI Protocol Refresher
   - 5.2 STM32F4 SPI Peripheral & Register Map
   - 5.3 `STM32F401RE_SPI_Driver` — Full API Reference
   - 5.4 SPI GPIO Configuration
6. [SPI Application — BMP280 Temperature Sensor Walk-Through](#6-spi-application--bmp280-temperature-sensor-walk-through)
7. [How to Write Your Own Application](#7-how-to-write-your-own-application)
8. [Cooking-Recipe: I²C Peripheral Driver from Scratch](#8-cooking-recipe-i2c-peripheral-driver-from-scratch)
9. [Cooking-Recipe: SPI Peripheral Driver from Scratch](#9-cooking-recipe-spi-peripheral-driver-from-scratch)
10. [Test Case Patterns](#10-test-case-patterns)
11. [Troubleshooting & Common Pitfalls](#11-troubleshooting--common-pitfalls)
12. [Glossary & Quick Reference](#12-glossary--quick-reference)

---

## 1. Introduction & Project Layout

The `Drivers-project` repository is a study project that builds **two custom bare-metal peripheral drivers** for the STM32F401RE and proves them out against two real slave devices:

| Driver | Application | Slave device |
|---|---|---|
| I²C | `RTC_I2C_Interface.c` | DS1307 RTC at `0x68` |
| SPI | `BMP280_Sensor_read.c` | BMP280 barometric/temperature sensor |

The whole project is **deliberately not using** ST's HAL or LL libraries. Every register bit the code touches is justified in comments. That makes the code longer than HAL, but it makes the *cause-and-effect* of every clock edge transparent — which is exactly what you want when you are learning.

### 1.1 Repository tree

```
Drivers-project/
├── Drivers/
│   ├── I2C_Driver/
│   │   ├── Baremetal/                ← low-level helpers
│   │   │   ├── inc/I2C_Baremetal.h
│   │   │   └── src/I2C_Baremetal.c
│   │   ├── Driver/                   ← the API surface
│   │   │   ├── inc/STM32F401RE_I2C_Driver.h
│   │   │   └── src/STM32F401RE_I2C_Driver.c
│   │   └── Application/
│   │       ├── RTC_I2C_Interface.c
│   │       └── README.md
│   └── SPI_Driver/
│       ├── Driver/
│       │   ├── inc/STM32F401RE_SPI_Driver.h
│       │   └── src/STM32F401RE_SPI_Driver.c
│       └── Application/
│           ├── BMP280_Sensor_read.c
│           └── README.md
└── Test case/
    ├── I2C/        ← high-level test cases (positive + negative)
    ├── SPI/        ← high-level test cases
    └── Test document/
        ├── I2C/   ← signed-off validation report
        └── SPI/   ← signed-off validation report + Output.txt
```

> Note: the I²C side has an extra "Baremetal" layer between the driver and the chip
> registers. The SPI side collapses both layers into one file. We'll explain why
> this matters in §3.3.

### 1.2 What you should already know

- C (pointers, structs, bitwise operators).
- Binary, hex, two's complement.
- Basic MCU concepts: clock tree, GPIO, memory-mapped I/O.
- A vague idea of what I²C and SPI are. (We re-derive them from scratch below.)

---

## 2. STM32F401RE Memory Map & Peripheral Access Primer

Everything in this project relies on the fact that **peripherals are just structs living at known addresses in memory**. The header `STM32F401RE.h` (referenced by the drivers but not in the repo) defines the layouts. For your own study material you can recreate it like this:

```c
/* Pointer base addresses from RM0368 §3.3 */
#define GPIOA_BASE   0x40020000UL
#define GPIOB_BASE   0x40020400UL
#define I2C1_BASE    0x40005400UL
#define SPI1_BASE    0x40013000UL
#define RCC_BASE     0x40023800UL

/* Cast each base to a pointer to a struct that mirrors the register layout */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_Reg_t;

#define GPIOA  ((GPIO_Reg_t *) GPIOA_BASE)
#define GPIOB  ((GPIO_Reg_t *) GPIOB_BASE)
```

The same pattern is used for `I2C_Reg_t` and `SPI_Reg_t`. Once you have the struct, `I2C1->I2C_CR1 |= (1 << 0)` is just a normal C assignment to a `volatile uint32_t` at a known address — and the compiler emits a single `STR` instruction.

### 2.1 Clock-enable macros (RCC)

Before you touch any peripheral you must switch on its clock in `RCC->AHB1ENR` / `RCC->APB1ENR` / `RCC->APB2ENR`. The project hides this behind small macros:

```c
/* In the I²C baremetal layer */
#define I2C1_CLK_EN()   (RCC->APB1ENR |= (1 << 21))
#define I2C2_CLK_EN()   (RCC->APB1ENR |= (1 << 22))
#define I2C3_CLK_EN()   (RCC->APB1ENR |= (1 << 23))
#define GPIOB_CLK_EN()  (RCC->AHB1ENR  |= (1 << 1))
```

For SPI the equivalent is `RCC->APB2ENR |= (1 << 12)` for SPI1 (APB2 bus), `RCC->APB1ENR |= (1 << 14)` for SPI2, etc.

---

## 3. I²C Driver — Theory + API

### 3.1 I²C Protocol Refresher

I²C is a **two-wire, half-duplex, multi-master, addressable** serial bus invented by Philips. The two wires are:

- **SCL** — Serial CLock (always driven by the current controller/master).
- **SDA** — Serial DAta (bidirectional, open-drain).

Both lines need **pull-up resistors** to Vcc (typically 4.7 kΩ). The peripheral outputs pull them LOW; nobody actively drives them HIGH.

#### 3.1.1 Key rules of the wire

1. **Open-drain only** — no device is ever allowed to drive SDA/SCL HIGH. Pull-ups do that.
2. **SDA is only allowed to change while SCL is LOW.** Data is sampled on the rising edge of SCL.
3. **START (S)** = SDA falls while SCL is HIGH.
4. **STOP (P)** = SDA rises while SCL is HIGH.
5. **Repeated START (Sr)** = a START issued without a preceding STOP, used to reverse direction on the bus.

#### 3.1.2 Byte format

```
  S | 7-bit slave address | R/W | ACK | 8-bit data | ACK | 8-bit data | ACK | ... | P
```

- After every byte the **receiver** (could be master or slave) holds SDA LOW for one clock to ACK.
- If the receiver is busy it can hold SCL LOW — that's *clock stretching*.

#### 3.1.3 Two flavors of speed the STM32 supports

| Mode | Max SCL | Max rise time | Notes |
|---|---|---|---|
| Standard Mode (Sm) | 100 kHz | 1000 ns | CCR_FS = 0 |
| Fast Mode (Fm) | 400 kHz | 300 ns | CCR_FS = 1; can choose DUTY=0 (T_low/T_high = 2) or DUTY=1 (T_low/T_high = 16/9) |

### 3.2 STM32F4 I²C Peripheral & Register Map

The I²C peripheral has a relatively large register set. The ones this driver actually touches:

| Register | Purpose | Bits we use |
|---|---|---|
| `I2C_CR1` | Control 1 | `PE` (0), `START` (8), `STOP` (9), `ACK` (10), `POS` (11), `SWRST` (15) |
| `I2C_CR2` | Control 2 | `FREQ[5:0]` — APB1 frequency in MHz |
| `I2C_DR`   | Data | write TX, read RX |
| `I2C_SR1`  | Status 1 | `SB` (0), `ADDR` (1), `BTF` (2), `RXNE` (6), `TXE` (7), `AF` (10), `OVR` (11), `BERR` (8) |
| `I2C_SR2`  | Status 2 | dual addressing, bus state — read to clear ADDR |
| `I2C_CCR`  | Clock control | `CCR[11:0]`, `FS` (15), `DUTY` (14) |
| `I2C_TRISE`| Rise time | `TRISE[5:0]` |

#### 3.2.1 The "ADDR flag" footgun

`ADDR` is the flag that says "slave ACKed its address". But you **cannot clear it by writing 0** — that does nothing. The only way to clear it is:

```c
/* Read SR1 then SR2 — order doesn't matter for ADDR, but RM says SR1 first */
(void)I2C1->I2C_SR1;
(void)I2C1->I2C_SR2;
```

Until you do that, the I²C peripheral will refuse to clock out the next byte. Every buggy STM32 I²C driver in the world has been bitten by this at least once.

#### 3.2.2 Receive byte count matters

When you call `I2C_Receive_Buffer(.., len=2)` you need a special dance: set `ACK=0` and `POS=1` *before* clearing `ADDR`, then wait for `BTF=1` (both bytes arrived, shift register full), then read `DR` twice. For `len==1` you set `NACK` and `STOP` *before* clearing ADDR. The driver below encodes all three cases.

### 3.3 Project's I²C Layered Architecture

The I²C code is split into two layers on purpose:

```
┌─────────────────────────────────────┐
│ Application (RTC_I2C_Interface.c)   │   ← calls Init, Transmit, Receive
├─────────────────────────────────────┤
│ STM32F401RE_I2C_Driver (high-level) │   ← configures peripheral, exposes
│                                     │     readable APIs (I2C_Init,
│                                     │     I2C_Transmit_Buffer, …)
├─────────────────────────────────────┤
│ I2C_Baremetal (low-level helpers)   │   ← owns the START/STOP/state-flag
│                                     │     choreography
├─────────────────────────────────────┤
│ STM32F401RE.h + register structs    │   ← the "metal"
└─────────────────────────────────────┘
```

The lower layer does **not** know about RTC or any other slave. The upper layer does **not** know what a `while(!TXE)` loop looks like. This split lets you reuse `I2C_Baremetal` if you ever need to talk to a brand-new I²C chip that doesn't fit `I2C_Transmit_Buffer`/`I2C_Receive_Buffer`.

### 3.4 `I2C_Baremetal` — Low-Level Helpers

Header `Drivers/I2C_Driver/Baremetal/inc/I2C_Baremetal.h`:

```c
void I2C_Initialization(void);                          // one-shot hard-coded init for I2C1 @ 100 kHz
void GPIO_Configurations_SCL_SDA(void);                  // PB8/PB9 as AF4 open-drain

void generate_start(I2C_Reg_t* I2Cx);
void send_address_write(I2C_Reg_t* I2Cx, uint8_t slv_address);
void send_address_read (I2C_Reg_t* I2Cx, uint8_t slv_address);
void clear_ADDR_flag   (I2C_Reg_t* I2Cx);

void send_data(I2C_Reg_t* I2Cx, uint8_t data);
uint8_t read_data(I2C_Reg_t* I2Cx);

void set_ACK (I2C_Reg_t* I2Cx);
void set_NACK(I2C_Reg_t* I2Cx);
void set_POS (I2C_Reg_t* I2Cx);
void clear_POS(I2C_Reg_t* I2Cx);

void generate_stop(I2C_Reg_t* I2Cx);
uint8_t check_status_flag(I2C_Reg_t* I2Cx, uint32_t flag);
void wait_till_transfer(I2C_Reg_t* I2Cx);

void I2C_Application(void);  // demo: write/read time from DS1307
```

The body of every helper is in `I2C_Baremetal.c`. The important ones, in execution order:

#### 3.4.1 `I2C_Initialization()` — register-by-register walkthrough

```c
void I2C_Initialization(void)
{
    /* 1. Enable APB1 clock for I2C1 */
    I2C1_CLK_EN();

    /* 2. Tell the I²C peripheral how fast APB1 is running (MHz) */
    I2C1->I2C_CR2 &= ~(0x3F << I2C_CR2_FREQ);
    I2C1->I2C_CR2 |=  (16  << I2C_CR2_FREQ);  // 16 MHz (HSI default on a fresh Nucleo)

    /* 3. Choose Standard Mode, set CCR = F_APB1 / (2 * F_SCL) */
    I2C1->I2C_CCR &= ~(1 << I2C_CCR_FS);          // Standard mode
    I2C1->I2C_CCR &= ~(0xFFF << 0);
    I2C1->I2C_CCR |=  (80  << 0);                  // 16 MHz / (2*100 kHz) = 80

    /* 4. Maximum allowed rise time: TRISE = (F_APB1 * t_rise) + 1 */
    I2C1->I2C_TRISE &= ~(0x3F << 0);
    I2C1->I2C_TRISE |=  (17  << 0);                // 16 MHz * 1000 ns + 1 = 17

    /* 5. GPIO — see §3.6 */
    GPIO_Configurations_SCL_SDA();

    /* 6. Enable the peripheral last */
    I2C1->I2C_CR1 |= (1 << I2C_CR1_PE);
}
```

**Why 17 for TRISE?** The TRISE field stores the number of clock cycles the I²C peripheral should wait to account for the SDA/SCL rise time. The formula in the reference manual is `TRISE = (F_APB1 in Hz) × (t_rise in seconds) + 1`. For 16 MHz and 1000 ns: `16,000,000 × 1e-6 + 1 = 17`.

**Why CCR = 80?** In Standard mode the master divides its APB1 clock by `2 × CCR` to make the SCL period. So `16 MHz / (2 × 100 kHz) = 80`.

#### 3.4.2 `generate_start()`

```c
void generate_start(I2C_Reg_t* I2Cx)
{
    I2Cx->I2C_CR1 |= (1 << I2C_CR1_START);
    while(!(I2Cx->I2C_SR1 & I2C_SR1_SB));   // wait for START bit to be sent (SB = 1)
}
```

`SB` (Start Bit) in `SR1` is set as soon as the START condition has been generated. As long as the bus is free and the peripheral is enabled, the master will assert SDA-low-while-SCL-high and then SB is set in hardware. **You must read or check SB and (in our driver) implicitly clear it by writing DR.**

#### 3.4.3 `send_address_write()` / `send_address_read()`

```c
void send_address_write(I2C_Reg_t* I2Cx, uint8_t slv_address)
{
    I2Cx->I2C_DR = (slv_address << 1) | 0;   // R/W bit = 0
    while(!(I2Cx->I2C_SR1 & I2C_SR1_ADDR));  // wait for ACK from slave
}
```

The I²C address byte is 7-bit address followed by a 1-bit R/W direction. The convention used here is "pass the 7-bit address, the driver shifts it left and ORs in 0 or 1". So `0x68` (the DS1307) becomes `0xD0` on the wire for write and `0xD1` for read.

`ADDR` is the *Acknowledge of ADDRess* flag. Setting ADDR means the slave sent ACK. We **do not** clear it here — the caller does that, because clearing ADDR is also the moment the peripheral starts clocking the data phase.

#### 3.4.4 `clear_ADDR_flag()`

```c
void clear_ADDR_flag(I2C_Reg_t* I2Cx)
{
    (void)I2Cx->I2C_SR1;   // dummy read
    (void)I2Cx->I2C_SR2;   // dummy read — clears ADDR
}
```

#### 3.4.5 `send_data()` / `read_data()`

```c
void send_data(I2C_Reg_t* I2Cx, uint8_t data)
{
    while(!(I2Cx->I2C_SR1 & I2C_SR1_TXE));   // wait for TX buffer empty
    I2Cx->I2C_DR = data;
}

uint8_t read_data(I2C_Reg_t* I2Cx)
{
    while(!(I2Cx->I2C_SR1 & I2C_SR1_RXNE));  // wait for RX buffer not empty
    return (uint8_t)I2Cx->I2C_DR;
}
```

#### 3.4.6 ACK / NACK / POS / STOP

```c
void set_ACK  (I2C_Reg_t* I2Cx){ I2Cx->I2C_CR1 |=  (1<<I2C_CR1_ACK); }
void set_NACK (I2C_Reg_t* I2Cx){ I2Cx->I2C_CR1 &= ~(1<<I2C_CR1_ACK); }
void set_POS  (I2C_Reg_t* I2Cx){ I2Cx->I2C_CR1 |=  (1<<I2C_CR1_POS); }
void clear_POS(I2C_Reg_t* I2Cx){ I2Cx->I2C_CR1 &= ~(1<<I2C_CR1_POS); }
void generate_stop(I2C_Reg_t* I2Cx){ I2Cx->I2C_CR1 |= (1<<I2C_CR1_STOP); }
```

`ACK` is the bit the master uses to tell the slave "send me one more byte". `NACK` is "this is the last byte, don't send more". `POS` is the "ACK the first of two bytes, NACK the second" trick we use for `len==2`. `STOP` releases the bus.

#### 3.4.7 `wait_till_transfer()`

```c
void wait_till_transfer(I2C_Reg_t* I2Cx)
{
    while(!(I2Cx->I2C_SR1 & I2C_SR1_BTF));   // BTF = Byte Transfer Finished
}
```

BTF is set after the last byte has **both** shifted out and emptied TX/RX. It's the safe time to issue a STOP.

### 3.5 `STM32F401RE_I2C_Driver` — High-Level APIs

The "real" driver file has just four public functions, all listed in `STM32F401RE_I2C_Driver.h`:

```c
/* Configuration structures */
typedef enum { FastMode_duty_2, FastMode_duty_16_9 } FM_Duty_t;
typedef enum { Standard_Mode,    Fast_Mode          } SCL_Mode_t;

typedef struct {
    uint32_t   I2C_SCL_Speed;        // Hz, e.g. 100000
    SCL_Mode_t I2C_Speed_mode;      // Standard_Mode or Fast_Mode
    FM_Duty_t  I2C_SCL_DutyCycle;   // only used in Fast_Mode
} I2C_Config_t;

typedef struct {
    I2C_Reg_t*  I2Cx;               // I2C1 / I2C2 / I2C3
    I2C_Config_t I2C_Config;
} I2C_Handle_t;

/* Public APIs */
void I2C_Init                 (I2C_Handle_t* pI2C_Handle);
void I2C_PeriClockControl     (I2C_Reg_t *pI2Cx, uint8_t EnorDi);
void I2C_Transmit_Buffer      (I2C_Reg_t*, uint8_t reg_address,
                               uint8_t* Tx_Buff, uint8_t len, uint8_t slv_address);
void I2C_Receive_Buffer       (I2C_Reg_t*, uint8_t reg_address,
                               uint8_t* Rx_Buff, uint8_t len, uint8_t slv_address);
```

#### 3.5.1 `I2C_PeriClockControl(pI2Cx, EnorDi)`

Turns the APB1 clock for I²C1/2/3 on or off. The `EnorDi` parameter uses whatever the project defines `ENABLE`/`DISABLE` as (typically 1 / 0).

```c
if (EnorDi == ENABLE) {
    if (pI2Cx == I2C1) I2C1_CLK_EN();
    else if (pI2Cx == I2C2) I2C2_CLK_EN();
    else if (pI2Cx == I2C3) I2C3_CLK_EN();
} else { /* ... DISABLE paths ... */ }
```

#### 3.5.2 `I2C_Init(pI2C_Handle)` — what it does

The full body is in the source file. In short:

1. `I2C_PeriClockControl(pI2C_Handle->I2Cx, ENABLE)`
2. Set `FREQ[5:0]` = `16` (assumes 16 MHz APB1 — change this if you re-clock).
3. Branch on speed mode:

   *Standard mode (`Standard_Mode`)*
   - Clear `CCR_FS` bit.
   - `CCR = 16,000,000 / (2 × SCL_speed)`.
   - `TRISE = 16,000,000 × 1e-6 + 1 = 17`.

   *Fast mode (`Fast_Mode`)*
   - Set `CCR_FS` bit.
   - If duty = `FastMode_duty_2`:  `CCR = 16,000,000 / (3 × SCL_speed)`.
   - If duty = `FastMode_duty_16_9`: `CCR = 16,000,000 / (25 × SCL_speed)`.
   - `TRISE = 16,000,000 × 3e-7 + 1 ≈ 5`.

4. Set `PE` to enable the peripheral.

> ⚠️ The driver currently hard-codes 16 MHz. If your Nucleo is running from PLL on a different APB1 frequency, compute `FREQ` and `TRISE` against your actual clock and adjust accordingly.

#### 3.5.3 `I2C_Transmit_Buffer(...)` — write sequence

The transmit path is the same for any slave register that auto-increments:

```
START → ADDR(W) → clear ADDR → [reg_address byte] → data[0] → data[1] → … → BTF → STOP
```

`reg_address` is the **first internal register** of the slave you want to start writing at. Most I²C chips (DS1307, AT24 EEPROMs, MPU6050, …) auto-increment the address pointer after each ACK, so writing N bytes from `reg_address` writes `reg_address`, `reg_address+1`, …, `reg_address+N-1`.

Code (excerpted):

```c
generate_start(I2Cx);
send_address_write(I2Cx, slv_address);
clear_ADDR_flag(I2Cx);
send_data(I2Cx, reg_address);

while (len > 0) { send_data(I2Cx, *Tx_Buff++); len--; }

wait_till_transfer(I2Cx);
generate_stop(I2Cx);
```

#### 3.5.4 `I2C_Receive_Buffer(...)` — read sequence

A typical "read N bytes from register R" transaction looks like this on the wire:

```
START → ADDR(W) → reg_addr → REPEATED_START → ADDR(R) → data[0] → data[1] → … → STOP
```

The repeated START (Sr) is mandatory. After the Sr the master switches from write to read on the same slave. The driver does this:

```c
generate_start(I2Cx);
send_address_write(I2Cx, slv_address);
clear_ADDR_flag(I2Cx);
send_data(I2Cx, reg_address);
generate_start(I2Cx);          // repeated start
send_address_read(I2Cx, slv_address);
```

From here it forks based on `len`:

**`len == 1`** (single byte)
```c
set_NACK(I2Cx);
generate_stop(I2Cx);     // STOP is set BEFORE clearing ADDR on purpose
clear_ADDR_flag(I2Cx);   // ADDR clear starts the receive
*Rx_Buff = read_data(I2Cx);
```

**`len == 2`** (the "BTF" trick)
```c
set_NACK(I2Cx);
set_POS (I2Cx);          // ACK first byte, NACK second
clear_ADDR_flag(I2Cx);
wait_till_transfer(I2Cx);  // both bytes in DR + shift register
generate_stop(I2Cx);
*Rx_Buff++ = I2Cx->I2C_DR;
*Rx_Buff   = I2Cx->I2C_DR;
clear_POS(I2Cx);         // clean up for next time
```

**`len > 2`** (multi-byte)
```c
set_ACK(I2Cx);                       // ACK every byte except the last
clear_ADDR_flag(I2Cx);
for (len = len - 1; len > 0; len--) {
    *Rx_Buff++ = read_data(I2Cx);    // each read ACKs the next byte
}
set_NACK(I2Cx);
generate_stop(I2Cx);
*Rx_Buff = read_data(I2Cx);          // last byte, no ACK, STOP already issued
```

> All three branches come straight from **RM0368 §27.6.1 "Master receiver"** — the reference manual is the source of truth.

### 3.6 I²C GPIO Configuration

`GPIO_Configurations_SCL_SDA()` configures PB8 and PB9 (the I2C1 pins on the LQFP64 package, also broken out as `D15`/`D14` on the Arduino header of the Nucleo-F401RE):

```c
GPIOB_CLK_EN();

/* Mode = Alternate Function (0b10) */
GPIOB->MODER  &= ~((0x3 << (8*2)) | (0x3 << (9*2)));
GPIOB->MODER  |=  ((0x2 << (8*2)) | (0x2 << (9*2)));

/* AFR[1] holds pins 8..15; AF4 = I2C1/2/3 */
GPIOB->AFR[1] &= ~((0xF << 0) | (0xF << 4));
GPIOB->AFR[1] |=  ((0x4 << 0) | (0x4 << 4));

/* Output type = open-drain (required for I²C) */
GPIOB->OTYPER |= (1 << 8) | (1 << 9);

/* Pull-up — internal weak pull is usually enough for short jumpers on a breadboard.
   For real buses use 4.7 kΩ external pull-ups. */
GPIOB->PUPDR &= ~((0x3 << (8*2)) | (0x3 << (9*2)));
GPIOB->PUPDR |=  ((0x1 << (8*2)) | (0x1 << (9*2)));

/* High speed slew */
GPIOB->OSPEEDR &= ~((0x3 << (8*2)) | (0x3 << (9*2)));
GPIOB->OSPEEDR |=  ((0x3 << (8*2)) | (0x3 << (9*2)));
```

**Why open-drain?** I²C is a wired-AND bus. If two devices ever both tried to drive SDA, the one driving HIGH would source current into the one driving LOW, possibly damaging pins or fighting each other. Open-drain + pull-up means the line is *passive* HIGH and *active* LOW — only one device can ever "win" by pulling LOW. Same reason for SCL.

**Why pull-up is mandatory?** The STM32's I²C pins in open-drain mode literally cannot source current to drive HIGH — the pull-up resistor is what makes the line rise.

---

## 4. I²C Application — RTC (DS1307) Walk-Through

File: `Drivers/I2C_Driver/Application/RTC_I2C_Interface.c`.

The DS1307 is a classic 7-bit-addressed RTC. Its slave address is `0x68`, and its first 8 registers (addresses `0x00`–`0x07`) are the timekeeper in **BCD format**:

| Address | Register | Layout (bits 7..0) |
|---|---|---|
| 0x00 | Seconds | `CH` (1) + seconds BCD (7) |
| 0x01 | Minutes | minutes BCD |
| 0x02 | Hours   | `12/24` (1) + `AM/PM` (1) + hours (BCD) |
| 0x03 | Day     | day-of-week (1..7) |
| 0x04 | Date    | date BCD |
| 0x05 | Month   | month BCD |
| 0x06 | Year    | year BCD (00..99) |
| 0x07 | Control | square-wave / OUT |

### 4.1 The whole `app_main()` annotated

```c
#define RTC_ADDR 0x68

int app_main(void)
{
    USART_Configuration();        // debug print over UART
    GPIO_Configurations_SCL_SDA();// PB8/PB9 → AF4 open-drain

    I2C_Handle_t handle = {
        .I2Cx                   = I2C1,
        .I2C_Config.I2C_Speed_mode = Standard_Mode,
        .I2C_Config.I2C_SCL_Speed  = 100000,   // 100 kHz
    };
    I2C_Init(&handle);

    /* ---- Build BCD payload ---- */
    uint8_t buffer[3];
    uint8_t seconds = 0, minutes = 10, hours = 23;

    /* Each line: tens digit <<4 | units digit, then mask away the control bits */
    buffer[0] = (((seconds/10) << 4) | (seconds%10)) & 0x7F; // CH must be 0
    buffer[1] =   (minutes/10) << 4   | (minutes%10);
    buffer[2] = (((hours  /10) << 4) | (hours%10))   & 0x3F; // 12/24 bit must be 0

    /* ---- Write 3 bytes starting at register 0x00 ---- */
    I2C_Transmit_Buffer(I2C1, 0x00, buffer, 3, RTC_ADDR);

    /* ---- Continuous read-back loop ---- */
    while (1) {
        I2C_Receive_Buffer(I2C1, 0x00, buffer, 3, RTC_ADDR);

        seconds = ((buffer[0] >> 4) * 10) + (buffer[0] & 0x0F);
        minutes = ((buffer[1] >> 4) * 10) + (buffer[1] & 0x0F);
        hours   = ((buffer[2] >> 4) * 10) + (buffer[2] & 0x0F);

        USART_Send_String("\n\nCurrent Time: ");
        USART_Send_Number(hours);
        USART_Send_String(": ");
        USART_Send_Number(minutes);
        USART_Send_String(": ");
        USART_Send_Number(seconds);

        for (volatile uint32_t i = 0; i < 500000; i++);  // crude 1-Hz delay
    }
}
```

### 4.2 The BCD dance explained

BCD = Binary Coded Decimal. Each decimal digit is stored in 4 bits. `23` becomes `0010 0011` = `0x23`.

```c
/* Decimal → BCD: tens<<4 | units */
buffer[0] = (((seconds/10) << 4) | (seconds%10)) & 0x7F;
```

- `seconds/10` is the tens digit (0..5).
- `seconds%10` is the units digit (0..9).
- Shift tens into the high nibble.
- Mask with `0x7F` to clear bit 7 of the seconds register (the **CH** "Clock Halt" bit — leave it 0 to keep ticking).

```c
/* BCD → Decimal: (high_nibble * 10) + low_nibble */
seconds = ((buffer[0] >> 4) * 10) + (buffer[0] & 0x0F);
```

The hours register needs `& 0x3F` because bit 6 is the 12/24 mode bit and bit 5 is AM/PM. We always want 24-hour mode, so we mask both away.

### 4.3 Expected output

The test report logs (and the README in the project shows):

```
Current Time: 23: 10: 00
Current Time: 23: 10: 01
Current Time: 23: 10: 02
...
```

### 4.4 Test cases that prove the driver works

From `Test case/I2C/README.md`:

| ID | What it checks |
|---|---|
| TC_POS_01 | Slave ACK at 0x68 |
| TC_POS_02 | Time written into registers 0x00–0x02 |
| TC_POS_03 | Time read back correctly |
| TC_POS_04 | BCD↔decimal conversion correct |
| TC_POS_05 | Long-running loop, seconds increment |
| TC_NEG_01 | RTC unplugged → no ACK, code doesn't hang |
| TC_NEG_02 | Wrong slave address → no response |
| TC_NEG_03 | Invalid register read |
| TC_NEG_04 | Bad BCD value (e.g. 0x6A) → displayed time is wrong |
| TC_NEG_05 | SDA/SCL disconnected mid-transaction |

> The "no hang" behavior in TC_NEG_01/05 is interesting — the baremetal driver
> as written would *block* forever in `while(!ADDR)`. In a real production
> driver you would guard these with timeouts. See §11 for the pattern.

---

## 5. SPI Driver — Theory + API

### 5.1 SPI Protocol Refresher

SPI is a **four-wire, full-duplex, master-driven** serial bus from Motorola. Signals:

| Signal | Direction | Purpose |
|---|---|---|
| **SCK** | Master → Slave | Clock |
| **MOSI** | Master → Slave | Master Out, Slave In (data) |
| **MISO** | Slave → Master | Master In, Slave Out (data) |
| **NSS / SS** | Master → Slave (active low) | Chip select; "you're being talked to" |

**Key traits:**

- **Full duplex** — every clock edge shifts one bit in **and** one bit out, simultaneously.
- **No addressing** — to talk to a different chip you just de-assert its NSS and assert another.
- **No acknowledgment** — the receiver cannot say "I missed that". You are on your own to set a slow enough clock.
- **Push-pull, not open-drain** — no pull-ups needed, lines actively driven.
- **Configurable clock polarity (CPOL) and phase (CPHA)** — four "modes" numbered 0..3.

#### 5.1.1 The four SPI modes

| Mode | CPOL | CPHA | Idle clock | Sampling edge |
|---|---|---|---|---|
| 0 | 0 | 0 | low  | rising |
| 1 | 0 | 1 | low  | falling |
| 2 | 1 | 0 | high | falling |
| 3 | 1 | 1 | high | rising |

Mode 0 (CPOL=0, CPHA=0) is by far the most common — BMP280 uses it.

#### 5.1.2 Frame format

A *frame* is one chip-select-low to chip-select-high transaction. Inside the frame the master shifts out `N` bits (usually 8, sometimes 16). MSB first by default.

### 5.2 STM32F4 SPI Peripheral & Register Map

| Register | Purpose | Bits we use |
|---|---|---|
| `SPI_CR1` | Control 1 | `CPHA` (0), `CPOL` (1), `MSTR` (2), `BR[2:0]` (3..5), `SPE` (6), `LSBFIRST` (7), `SSI` (8), `SSM` (9), `RXONLY` (10), `DFF` (11), `BIDIMODE` (15) |
| `SPI_CR2` | Control 2 | `SSOE` (2) (for hardware NSS) |
| `SPI_SR`  | Status   | `RXNE` (0), `TXE` (1), `BSY` (7), `OVR` (6), `MODF` (5) |
| `SPI_DR`  | Data     | write TX, read RX |

#### 5.2.1 The RXNE footgun

Unlike I²C, SPI in full-duplex mode **always** receives a byte every time it sends one. The hardware sets `RXNE` after the last bit has been shifted in. If you ignore `RXNE` and start a new transaction, the next byte's RX will collide with the previous one → **`OVR` (Overrun) error**. That's why `SPI_Transmit_Buffer` does a dummy read of `SPI_DR` after every byte.

#### 5.2.2 BSY

`BSY` in `SPI_SR` is set as long as a transaction is in progress. The driver waits for `BSY=0` after the last byte before returning — at that point it's safe to de-assert NSS.

#### 5.2.3 SSM / SSI

If you want to control NSS in software (just a regular GPIO), you set **SSM=1** (Software Slave Management) and **SSI=1** (Internal SS high). This avoids the hardware NSS pin going haywire when the master is alone on the bus. If you want hardware NSS, you set SSM=0 and configure SSOE in CR2.

#### 5.2.4 DFF and the data size

`DFF=0` → 8-bit frames. `DFF=1` → 16-bit frames. The driver treats the buffer pointer type based on DFF, writing `*(uint16_t*)pTxBuffer` for 16-bit mode.

#### 5.2.5 Bus configuration

| Bus config | BIDIMODE | RXONLY | Meaning |
|---|---|---|---|
| Full duplex | 0 | 0 | MOSI + MISO both used |
| Half duplex  | 1 | 0 | One wire, direction chosen by BIDIOE |
| Simplex RX   | 0 | 1 | Receive only, MOSI ignored |

### 5.3 `STM32F401RE_SPI_Driver` — Full API Reference

Header `Drivers/SPI_Driver/Driver/inc/STM32F401RE_SPI_Driver.h` exposes:

```c
/* ---------- Config struct ---------- */
typedef struct {
    uint8_t SPI_CPOL;          // SPI_CPOL_LOW / SPI_CPOL_HIGH
    uint8_t SPI_CPHA;          // SPI_CPHA_LOW / SPI_CPHA_HIGH
    uint8_t SPI_Device_Mode;   // SPI_DEVICE_MODE_MASTER / SPI_DEVICE_MODE_SLAVE
    uint8_t SPI_CLK_Speed;     // SPI_PRESCALAR_2/4/8/16/32/64/128/256
    uint8_t SPI_Bus_Config;    // SPI_BUS_CONFIG_FD / _HD / _SIMPLEX_RX
    uint8_t SPI_DataFrame;     // SPI_DFF_8BITS / SPI_DFF_16BITS
    uint8_t SPI_SSM;           // SPI_SSM_EN / SPI_SSM_DI
} SPI_Config_t;

typedef struct {
    SPI_Reg_t   *pSPIx;
    SPI_Config_t SPI_Config;
    /* (interrupt state fields present but unused in polling mode) */
    uint8_t *pRxBuffer; uint8_t Rx_len; uint8_t Tx_State; uint8_t Rx_State;
} SPI_Handle_t;

/* ---------- APIs ---------- */
void SPI_PeriClockControl    (SPI_Reg_t *pSPIx, uint8_t EnorDi);
void SPI_Init                (SPI_Handle_t *pSPIHandle);
void SPI_DeInit              (SPI_Reg_t *pSPIx);
void SPI_PeripheralControl   (SPI_Reg_t *pSPIx, uint8_t EnorDi);
uint8_t SPI_Get_FlagStatus   (SPI_Reg_t *pSPIx, uint8_t Flag);
void SPI_Transmit_Buffer     (SPI_Reg_t *pSPIx, uint8_t *pTxBuffer, uint8_t Tx_len);
void SPI_Receive_Buffer      (SPI_Reg_t *pSPIx, uint8_t *pRxBuffer, uint8_t Rx_len);
void SPI_ClearOVRFlag        (SPI_Reg_t *pSPIx);
```

#### 5.3.1 `SPI_PeriClockControl(pSPIx, EnorDi)`

Enables the APB1/APB2 clock for SPI1..SPI4. Note: **SPI1 is on APB2**, **SPI2/3 are on APB1**, **SPI4 is on APB2**. The macros in the project reflect this — `SPI1_CLK_EN()` writes `RCC->APB2ENR`, the others write `RCC->APB1ENR`.

#### 5.3.2 `SPI_PeripheralControl(pSPIx, EnorDi)`

Toggles the `SPE` (Serial Peripheral Enable) bit. After `SPI_Init` the peripheral is **already enabled** — so this is mostly useful for temporarily disabling it (e.g. to reconfigure baud rate) or for the transmit-receive trick of "disable, swap, re-enable".

#### 5.3.3 `SPI_Init(pSPIHandle)` — line-by-line

```c
SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);

/* Device mode — MSTR bit in CR1 */
if (cfg->SPI_Device_Mode == SPI_DEVICE_MODE_MASTER)
    pSPIx->SPI_CR1 |=  (1 << SPI_CR1_MSTR);
else
    pSPIx->SPI_CR1 &= ~(1 << SPI_CR1_MSTR);

/* Clock phase and polarity */
pSPIx->SPI_CR1 |=  (cfg->SPI_CPHA << SPI_CR1_CPHA);
pSPIx->SPI_CR1 |=  (cfg->SPI_CPOL << SPI_CR1_CPOL);

/* Bus config — BIDIMODE and RXONLY bits */
switch (cfg->SPI_Bus_Config) {
    case SPI_BUS_CONFIG_FD:
        pSPIx->SPI_CR1 &= ~(1 << SPI_CR1_BIDIMODE);
        break;
    case SPI_BUS_CONFIG_HD:
        pSPIx->SPI_CR1 |=  (1 << SPI_CR1_BIDIMODE);
        break;
    case SPI_BUS_CONFIG_SIMPLEX_RX:
        pSPIx->SPI_CR1 &= ~(1 << SPI_CR1_BIDIMODE);
        pSPIx->SPI_CR1 |=  (1 << SPI_CR1_RXONLY);
        break;
}

/* Data frame format */
if (cfg->SPI_DataFrame == SPI_DFF_8BITS)
    pSPIx->SPI_CR1 &= ~(1 << SPI_CR1_DFF);
else
    pSPIx->SPI_CR1 |=  (1 << SPI_CR1_DFF);

/* Software Slave Management */
if (cfg->SPI_SSM == SPI_SSM_EN) {
    pSPIx->SPI_CR1 |= (1 << SPI_CR1_SSM);  // software NSS
    pSPIx->SPI_CR1 |= (1 << SPI_CR1_SSI);  // internal NSS high
} else {
    pSPIx->SPI_CR1 |=  (1 << SPI_CR1_SSOE); // hardware NSS
    pSPIx->SPI_CR1 &= ~(1 << SPI_CR1_SSM);
}

/* Baud rate prescaler (BR[2:0]) */
pSPIx->SPI_CR1 &= ~(0x7 << SPI_CR1_BR);
pSPIx->SPI_CR1 |=  (cfg->SPI_CLK_Speed << SPI_CR1_BR);

SPI_PeripheralControl(pSPIHandle->pSPIx, ENABLE);  // SPE = 1
```

**Baud rate formula:** `SCK = F_APBx / 2^(BR+1)`. So for APB2 = 50 MHz and prescaler 16 (BR=3), `SCK = 50 MHz / 16 = 3.125 MHz`. For BMP280 that's well below the 10 MHz max.

#### 5.3.4 `SPI_Get_FlagStatus(pSPIx, Flag)`

```c
return ((pSPIx->SPI_SR & Flag) ? 1 : 0);
```

Pass `SPI_SR_TXE`, `SPI_SR_RXNE`, `SPI_SR_BSY`, `SPI_SR_OVR` as `Flag`.

#### 5.3.5 `SPI_Transmit_Buffer(pSPIx, pTxBuffer, Tx_len)` — the full-duplex trap

```c
while (Tx_len > 0) {
    while (!(pSPIx->SPI_SR & SPI_SR_TXE));  // wait TX buffer empty

    if (pSPIx->SPI_CR1 & (1 << SPI_CR1_DFF)) {  // 16-bit frame
        pSPIx->SPI_DR = *((uint16_t*)pTxBuffer);
        pTxBuffer += 2;  Tx_len -= 2;
    } else {                                    // 8-bit frame
        pSPIx->SPI_DR = *pTxBuffer;
        pTxBuffer++;   Tx_len--;
    }

    while (!(pSPIx->SPI_SR & SPI_SR_RXNE));   // wait for receive to complete
    (void)pSPIx->SPI_DR;                      // dummy read — clears RXNE
}
while (pSPIx->SPI_SR & SPI_SR_BSY);            // wait for the bus to go idle
```

The line `(void)pSPIx->SPI_DR;` is the easy-to-miss hero of the function. Without it, `RXNE` stays set, the next iteration's `while(!RXNE)` would return immediately, and the next-to-last received byte would get clobbered → overrun.

#### 5.3.6 `SPI_Receive_Buffer(pSPIx, pRxBuffer, Rx_len)`

Receiving is "transmit a dummy byte to generate clock, then read the result":

```c
while (Rx_len > 0) {
    while (!(pSPIx->SPI_SR & SPI_SR_TXE));
    pSPIx->SPI_DR = 0xFF;          // or 0x0000 in 16-bit mode

    while (!(pSPIx->SPI_SR & SPI_SR_RXNE));
    if (pSPIx->SPI_CR1 & (1 << SPI_CR1_DFF)) {
        *((uint16_t*)pRxBuffer) = pSPIx->SPI_DR;
        pRxBuffer += 2; Rx_len -= 2;
    } else {
        *pRxBuffer = pSPIx->SPI_DR;
        pRxBuffer++;   Rx_len--;
    }
}
while (pSPIx->SPI_SR & SPI_SR_BSY);
```

#### 5.3.7 `SPI_ClearOVRFlag(pSPIx)`

If overrun has already happened, the only way to clear it is the **read DR, then read SR** sequence prescribed by the reference manual:

```c
uint32_t temp = pSPIx->SPI_DR;  // clear RXNE
      temp   = pSPIx->SPI_SR;   // clear OVR
(void)temp;
```

#### 5.3.8 `SPI_DeInit(pSPIx)`

Resets the SPI peripheral via `RCC->APBxxRSTR` — sets the reset bit, then clears it on the next instruction. This brings every register to its reset value.

### 5.4 SPI GPIO Configuration

The application uses these pins on the Nucleo-F401RE:

| Pin | Function | Arduino header label |
|---|---|---|
| PA5 | SPI1_SCK | D13 |
| PA6 | SPI1_MISO | D12 |
| PA7 | SPI1_MOSI | D11 |
| PB6 | manual CS   | D10 |

`SPI_GPIO_Configurations()` (lives in `SPI_Baremetal.h`/`SPI_Baremetal.c` referenced by the application) configures PA5/PA6/PA7 as **Alternate Function 5** push-pull, and PB6 as a general-purpose output used to manually control chip select.

> The driver never touches CS for you — that's a deliberate design choice. The application code wraps each SPI transaction in a `SelectSlave()` / `DeselectSlave()` pair. This is the *idiomatic* way to use this driver; we'll see the pattern in §6.

---

## 6. SPI Application — BMP280 Temperature Sensor Walk-Through

File: `Drivers/SPI_Driver/Application/BMP280_Sensor_read.c`.

The BMP280 is a Bosch barometric pressure + temperature sensor. It can speak both I²C and SPI. On the SPI side:

- The first byte of every transaction is a **register address**. The MSB of that address is the R/W bit (`0` for write, `1` for read).
- The chip ID register is at address `0xD0` and reads as `0x58`.
- 3 temperature calibration values live at `0x88`–`0x8D` (`dig_t1`, `dig_t2`, `dig_t3`).
- The 20-bit raw temperature ADC is spread across `0xFA`–`0xFC`.

### 6.1 The whole `App1_main()` annotated

```c
static void SelectSlave()  { GPIOB->ODR &= ~(1 << 6); }   // CS low
static void DeselectSlave(){ GPIOB->ODR |=  (1 << 6); }   // CS high

int App1_main(void)
{
    USART_Configuration();
    Delay_Creation();
    SPI_GPIO_Configurations();

    SPI_Handle_t spi1 = {
        .pSPIx                  = SPI1,
        .SPI_Config.SPI_Bus_Config  = SPI_BUS_CONFIG_FD,
        .SPI_Config.SPI_Device_Mode = SPI_DEVICE_MODE_MASTER,
        .SPI_Config.SPI_CLK_Speed   = SPI_PRESCALAR_16,
        .SPI_Config.SPI_SSM         = SPI_SSM_EN,
        .SPI_Config.SPI_DataFrame   = SPI_DFF_8BITS,
        /* CPOL and CPHA default 0 → Mode 0, which is what BMP280 needs */
    };
    SPI_Init(&spi1);

    /* ---- 1. Read sensor ID ---- */
    SelectSlave();
    SPI_TransmitByte(SPI1, 0xD0);                 // first byte = register | R/W (0)
    uint8_t sensor_id = SPI_ReceiveByte(SPI1);
    DeselectSlave();

    /* ---- 2. Read calibration block ---- */
    uint8_t calib_values[6] = {0};
    SelectSlave();
    SPI_TransmitByte(SPI1, 0x88);
    SPI_Receive_Buffer(SPI1, calib_values, 6);    // 6 bytes clocked in
    DeselectSlave();

    uint16_t dig_t1 = ((uint16_t)calib_values[1] << 8) | calib_values[0];
    int16_t  dig_t2 = ((int16_t) calib_values[3] << 8) | calib_values[2];
    int16_t  dig_t3 = ((int16_t) calib_values[5] << 8) | calib_values[4];

    /* ---- 3. Configure: write 0x27 to control-meas register 0xF4 ---- */
    SelectSlave();
    SPI_TransmitByte(SPI1, 0xF4);
    SPI_TransmitByte(SPI1, 0x27);
    DeselectSlave();

    /* ---- 4. Continuous temperature read ---- */
    while (1) {
        uint8_t ADC_data[3] = {0};
        SelectSlave();
        SPI_TransmitByte(SPI1, 0xFA);
        SPI_Receive_Buffer(SPI1, ADC_data, 3);
        DeselectSlave();

        uint32_t ADC_value =
              ((uint32_t)ADC_data[0] << 12)
            | ((uint32_t)ADC_data[1] << 4)
            | ((uint32_t)ADC_data[2] >> 4);

        int32_t var1 = ((((int32_t)ADC_value >> 3) - ((int32_t)dig_t1 << 1)) * (int32_t)dig_t2) >> 11;
        int32_t var2 = (((((int32_t)ADC_value >> 4) - (int32_t)dig_t1) *
                         (((int32_t)ADC_value >> 4) - (int32_t)dig_t1)) >> 12) * (int32_t)dig_t3) >> 14;
        int32_t t_fine = var1 + var2;
        int32_t T      = (t_fine * 5 + 128) >> 8;

        USART_SendString("\n\n\nSensor ID: ");   USART_SendHex_Number(sensor_id);
        USART_SendString("\ndig_t1 : ");         USART_Send_Number(dig_t1);
        /* … */
        USART_SendString("\nTemperature : ");
        USART_Send32bit_Number(T / 100);
        USART_Transmission('.');
        USART_Send32bit_Number(T % 100);

        delay_ms(500);
    }
}
```

### 6.2 Step-by-step: reading a register

For a "read register R" operation the SPI protocol for BMP280 is:

```
CS low → MOSI: (R | 0x80) → MOSI: dummy → MISO: data
              ^^^^^^^^^^^^^^^^^      ^^^^^^^^^^^
              "read this register"   "I need clock"
```

That's why every read in the application has a `SPI_TransmitByte(SPI1, addr)` *before* the `SPI_ReceiveByte/Buffer(...)`. The transmit pumps out 8 clocks; the receive samples the line during those same clocks.

### 6.3 Step-by-step: writing a register

```
CS low → MOSI: (W | addr) → MOSI: data → CS high
```

A register write is just two consecutive transmit bytes. No receive is needed.

### 6.4 The 20-bit ADC reconstruction

BMP280 returns temperature as a 20-bit value spread across three bytes:

```
bits:    19..12     11..4      3..0
byte:    MSB        LSB        XLSB
```

The reconstruction is:

```c
ADC_value = (MSB << 12) | (LSB << 4) | (XLSB >> 4);
```

### 6.5 Bosch compensation formula

Once you have `dig_t1..dig_t3` (16-bit values, some signed) and the raw `ADC_value`, the temperature in 0.01 °C is:

```c
int32_t var1  = ((((ADC_value >> 3) - ((int32_t)dig_t1 << 1))) * dig_t2) >> 11;
int32_t var2  = (((((ADC_value >> 4) - dig_t1) * ((ADC_value >> 4) - dig_t1)) >> 12) * dig_t3) >> 14;
int32_t t_fine = var1 + var2;
int32_t T      = (t_fine * 5 + 128) >> 8;   // T in 0.01 °C
```

The final test report's `Output.txt` shows `Temperature : 28.76` for room temperature — exactly what we'd expect.

### 6.6 Test cases that prove the SPI driver works

From `Test case/SPI/README.md`:

| ID | What it checks |
|---|---|
| TC_POS_01 | Sensor ID == 0x58 |
| TC_POS_02 | Calibration registers return valid non-zero values |
| TC_POS_03 | Write 0x27 to 0xF4, sensor enters normal mode |
| TC_POS_04 | Continuous 20-bit ADC read, values stay in range |
| TC_POS_05 | Compensation produces a sane room-temperature reading |
| TC_NEG_01 | Sensor unplugged → invalid ID |
| TC_NEG_02 | Wrong register address → garbage |
| TC_NEG_03 | SCK too fast → corruption |
| TC_NEG_04 | No CS asserted → no response |
| TC_NEG_05 | Forcing calibration to zero → compensation produces garbage |

---

## 7. How to Write Your Own Application

Now that you've seen the two reference applications, here is the **universal recipe** for adding a new slave to either bus.

### 7.1 Adding a new I²C slave (e.g. AT24C02 EEPROM)

1. **Look up the slave address and protocol in the chip's datasheet.** For an EEPROM, the first byte you send after the address byte is the memory address; subsequent bytes are data.
2. **Configure GPIO** by calling `GPIO_Configurations_SCL_SDA()` (or replicate it for PB8/PB9) — this is the bus-level setup, you only do it once per project.
3. **Configure the I²C peripheral** once:
   ```c
   I2C_Handle_t hi2c1 = { .I2Cx = I2C1,
                           .I2C_Config.I2C_Speed_mode = Standard_Mode,
                           .I2C_Config.I2C_SCL_Speed  = 100000 };
   I2C_Init(&hi2c1);
   ```
4. **Write**:
   ```c
   uint8_t data[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
   I2C_Transmit_Buffer(I2C1, /*mem_addr*/0x00, data, 4, /*slv*/0x50);
   ```
5. **Read**:
   ```c
   uint8_t rx[4];
   I2C_Receive_Buffer(I2C1, /*mem_addr*/0x00, rx, 4, /*slv*/0x50);
   ```
6. **Read from a register that is a single byte** (the `len==1` branch) — the driver already does the right thing.
7. **Read a 2-byte register** (the `len==2` branch) — the driver uses `POS` automatically.

#### Common I²C gotchas

- **Page-write size limit on EEPROMs** (e.g. AT24C02 has 8-byte pages). If you write across a page boundary, the address rolls over and you corrupt your data. Either use `len <= 8` or split into multiple `I2C_Transmit_Buffer` calls.
- **Write cycle time** — EEPROMs need 5 ms after a write before you can read them back. Add `delay_ms(10)`.
- **Repeated start** — the driver always uses Sr between write-address and read-address phases. Some slaves (DS1307) are OK with `STOP → START`, others (BMP280 in I²C mode) insist on Sr. This driver is already Sr-friendly.

### 7.2 Adding a new SPI slave (e.g. MPU6050 IMU)

1. **Pick your pins.** On the Nucleo-F401RE the natural SPI1 pins are PA5/PA6/PA7. Pick a free GPIO for chip-select (PB6 is used in the BMP280 example, but any pin works).
2. **Configure GPIO** (AF5 for SPI1, push-pull, high-speed, no pull-up/down):
   ```c
   /* SCK/MISO/MOSI: alternate function 5 on PA5/6/7, push-pull, high-speed */
   /* CS: PB6 as general-purpose output, initially high */
   ```
3. **Initialize SPI**:
   ```c
   SPI_Handle_t hspi1 = {
       .pSPIx = SPI1,
       .SPI_Config.SPI_Bus_Config  = SPI_BUS_CONFIG_FD,
       .SPI_Config.SPI_Device_Mode = SPI_DEVICE_MODE_MASTER,
       .SPI_Config.SPI_CLK_Speed   = SPI_PRESCALAR_16,   // 50 MHz / 16 ≈ 3 MHz
       .SPI_Config.SPI_SSM         = SPI_SSM_EN,         // software NSS
       .SPI_Config.SPI_DataFrame   = SPI_DFF_8BITS,
       /* Set CPOL/CPHA to match the slave's required SPI mode (0 for MPU6050) */
   };
   SPI_Init(&hspi1);
   ```
4. **Wrap every transaction in Select/Deselect:**
   ```c
   SelectSlave();
   SPI_TransmitByte(SPI1, 0x6B);          // address
   SPI_TransmitByte(SPI1, 0x00);          // write value
   DeselectSlave();

   SelectSlave();
   SPI_TransmitByte(SPI1, 0x6B | 0x80);   // read address
   uint8_t whoami = SPI_ReceiveByte(SPI1);
   DeselectSlave();
   ```
5. **For multi-byte reads** use `SPI_Receive_Buffer(SPI1, buf, n)`. Don't forget the leading address transmit that pumps out clocks.
6. **Wait for `BSY=0` before deselecting** — `SPI_Transmit_Buffer`/`Receive_Buffer` already do this internally, so you only need to worry about it if you mix-and-match helpers.

### 7.3 A reusable application template

```c
#include "STM32F401RE.h"
#include "STM32F401RE_I2C_Driver.h"   // or _SPI_Driver.h
#include "I2C_Baremetal.h"            // only for the baremetal helpers

#define SLAVE_ADDR  0x50              // example

static void bus_init(void)
{
    GPIO_Configurations_SCL_SDA();    // or SPI_GPIO_Configurations();

    I2C_Handle_t h = {
        .I2Cx = I2C1,
        .I2C_Config.I2C_Speed_mode = Standard_Mode,
        .I2C_Config.I2C_SCL_Speed  = 100000,
    };
    I2C_Init(&h);
}

static uint8_t reg_read(uint8_t reg)
{
    uint8_t v;
    I2C_Receive_Buffer(I2C1, reg, &v, 1, SLAVE_ADDR);
    return v;
}

static void reg_write(uint8_t reg, uint8_t v)
{
    I2C_Transmit_Buffer(I2C1, reg, &v, 1, SLAVE_ADDR);
}

int main(void)
{
    bus_init();
    while (1) {
        uint8_t whoami = reg_read(0x00);
        reg_write(0x01, 0xAB);
        (void)whoami;
    }
}
```

---

## 8. Cooking-Recipe: I²C Peripheral Driver from Scratch

If you were to write the I²C driver from scratch (not using the project as a base), here is the order in which to write code, with the test you should run at each step.

### Step 1 — Confirm the I²C lines wiggle

Write a stub that just initializes GPIO and toggles SCL/SDA manually with bit-banging (or use a logic analyzer). Make sure your pull-ups are correct. If you don't see a clean square wave, **stop and fix the hardware** before doing anything else.

### Step 2 — Get an ACK from a known device

The cheapest possible I²C transaction: a START followed by the slave's address. If you get ACK, your GPIOs, pull-ups, peripheral clock, and CR2.FREQ are all good.

```c
generate_start(I2C1);
send_address_write(I2C1, 0x68);
clear_ADDR_flag(I2C1);
generate_stop(I2C1);
```

Put a breakpoint (or toggle a debug LED) after `send_address_write` returns — if you got there, the slave ACKed.

### Step 3 — Write one byte to a register

```c
I2C_Transmit_Buffer(I2C1, 0x00, &val, 1, 0x68);
```

Read the same register with a logic analyzer and confirm the byte appeared at `0x00`. (Or use a second I²C transaction to read it back.)

### Step 4 — Read one byte from a register

```c
uint8_t b;
I2C_Receive_Buffer(I2C1, 0x00, &b, 1, 0x68);
```

### Step 5 — Read two bytes (the `POS` trick)

The single-byte read works because you set NACK+STOP before clearing ADDR. Two bytes are special — make sure you set `POS=1`, wait for `BTF`, then read DR twice. The driver handles this; just write the test.

### Step 6 — Read N>2 bytes

ACK all but the last. Last byte gets NACK and STOP. Driver handles this.

### Step 7 — Speed sweep

Re-run the tests with `I2C_Speed_mode = Fast_Mode` and `I2C_SCL_Speed = 400000`. If anything breaks, suspect TRISE and DUTY settings.

### Step 8 — Timeout wrappers

Wrap each `while` loop with a timeout counter and an error return code. (See §11.4 for the pattern.)

---

## 9. Cooking-Recipe: SPI Peripheral Driver from Scratch

### Step 1 — Loopback test

Connect MOSI and MISO together with a jumper. Send `0xA5`, expect to read `0xA5`. This validates GPIO, AF, baud rate, and clock polarity in one go.

### Step 2 — Logic-analyzer check

With no slave attached, run the loopback test on a logic analyzer. Verify CPOL matches your configuration, that MOSI toggles 8 times per `SPI_TransmitByte` call, and that there's exactly 8 SCK pulses per byte.

### Step 3 — Real slave ID read

Attach the actual slave and read its WHO_AM_I / chip ID register. For BMP280 it should be `0x58`. If it's `0xFF` you've got a wiring or CS issue; if it's `0x00` you have a clock-polarity mismatch.

### Step 4 — Multi-byte read

Use `SPI_Receive_Buffer` to read the 6 calibration bytes. Compare them with the reference values from the datasheet (or another working driver). If they match, your timing is right.

### Step 5 — Write-then-read-back

Write to a writable register (e.g. BMP280's `0xF4`), then read it back, and confirm the value matches.

### Step 6 — Speed sweep

Re-run at prescaler 8, then 4. If 4 fails, the slave's max SCK rating is below `F_APB/4`. Slow down.

### Step 7 — Long-running stress test

Run a 24-hour loop. Watch for OVR errors (use `SPI_Get_FlagStatus(SPI1, SPI_SR_OVR)` periodically). OVRs mean somewhere a byte was received but not drained.

---

## 10. Test Case Patterns

The repo's tests follow a simple two-axis structure that you should reuse:

### 10.1 Positive tests

Prove the *happy path* works end-to-end. For each one, write a one-line criterion for "pass" (e.g. "time increments every second").

### 10.2 Negative tests

Prove the driver *fails safely* on bad inputs. Specifically:

- Slave disconnected (open bus)
- Wrong slave address
- Invalid register
- Bad data
- Mid-transaction bus fault

### 10.3 Each test entry should have

| Column | Example |
|---|---|
| Test Case ID | `TC_POS_03` |
| Description | "Verify RTC Register Read Operation" |
| Test Steps | exact procedure |
| Expected Result | what success looks like |
| Status | Pass / Fail |
| (optional) Actual Result | what you observed |

This is the same structure used in the repo. Adopt it for any new driver you build.

---

## 11. Troubleshooting & Common Pitfalls

This is the section you keep open while you debug. These are the issues **specifically caused by mis-using the two drivers in this project**.

### 11.1 I²C hangs in `while(!ADDR)`

**Symptom:** the program locks up after calling `I2C_Transmit_Buffer`.
**Cause:** the slave did not ACK. Maybe wrong address, missing pull-up, slave not powered, SDA/SCL crossed, or peripheral clock not running.
**Fix:** verify the address with the datasheet, add 4.7 kΩ pull-ups if missing, check that `RCC->APB1ENR` has the right bit set, and confirm the slave is getting power.
**Long-term fix:** add a timeout (see §11.4).

### 11.2 I²C reads garbage after a previous transaction

**Symptom:** the first `I2C_Receive_Buffer` call works, subsequent ones return wrong data.
**Cause:** the ADDR flag is not being cleared. Or the `POS` bit was left set from a previous `len==2` read.
**Fix:** the driver clears `POS` at the end of the `len==2` branch. If you rolled your own receive code, make sure you do too.

### 11.3 SPI returns `0xFF` or `0x00` always

**Symptom:** `SPI_ReceiveByte` always returns the same value.
**Cause:** the CS line is not being asserted (`SelectSlave()` not called), or the slave is wired backwards (MISO/MOSI swapped).
**Fix:** check the wiring and the GPIO state of the CS pin. Probe with a logic analyzer — CS should go low *before* the first SCK edge and rise *after* the last one.

### 11.4 Add timeouts everywhere

The baremetal helpers wait forever. In a real product that's a watchdog reset waiting to happen. Wrap each loop:

```c
static uint32_t spin_until(volatile uint32_t* reg, uint32_t mask, uint32_t timeout_ticks)
{
    while (((*reg & mask) == 0)) {
        if (--timeout_ticks == 0) return 0;   // timeout
    }
    return 1;                                  // success
}
```

Use it like:

```c
if (!spin_until(&I2C1->I2C_SR1, I2C_SR1_SB, 100000)) {
    /* handle timeout: log it, return error, reset bus, … */
}
```

### 11.5 Wrong baud rate vs clock tree

The driver hard-codes 16 MHz for the I²C `FREQ` field. If you re-clock the Nucleo to use the external crystal (HSE 8 MHz) and the PLL, that number must change. For I²C `FREQ` must be `F_APB1 in MHz`, and for SPI the baud rate `F_SCK = F_APBx / 2^(BR+1)`.

### 11.6 "But the slave is at 0x68" — no, it's at 0xD0

A classic. The 7-bit address `0x68` is shifted left to `0xD0` (write) and `0xD1` (read) on the wire. The driver takes the 7-bit form. If you copy-paste an I²C address from a tutorial that uses the 8-bit form (`0xD0`), the driver will see `0xD0 << 1 = 0x1A0` — definitely no slave there.

### 11.7 Open-drain on I²C, push-pull on SPI

If you accidentally configure PB8/PB9 as push-pull, the STM32 and the slave will *fight* over the line. The symptom is "I can see the START on the scope, but the slave never ACKs." Always set `OTYPER` bit for I²C pins, *never* for SPI pins.

### 11.8 Reading the wrong number of bytes in I²C

`I2C_Receive_Buffer` has three branches (`len==1`, `len==2`, `len>2`). If you pass `len=0` the function does nothing. If you pass a value larger than your buffer, you have a buffer overflow — the function trusts you.

### 11.9 SPI overrun after long bursts

If you write a long loop that doesn't call `SPI_Receive_Buffer` (only `Transmit_Buffer`), make sure the dummy read is happening inside the transmit function. The driver's transmit does include the dummy read; the baremetal `SPI_TransmitByte`/`SPI_ReceiveByte` helpers used in the BMP280 application do not — they do one byte at a time, so each `TransmitByte` is followed by a `ReceiveByte` (which performs the dummy read).

### 11.10 BCD math surprises

`(seconds/10) << 4 | (seconds%10)` works only if both operands are 0..9. If you accidentally use `seconds` itself, you'll get a byte full of garbage. Keep the divide-then-modulo pattern.

---

## 12. Glossary & Quick Reference

### 12.1 I²C bit fields we touched

| Field | Register | Bits | Meaning |
|---|---|---|---|
| `PE`   | CR1 | 0 | Peripheral enable |
| `START`| CR1 | 8 | Generate START condition |
| `STOP` | CR1 | 9 | Generate STOP condition |
| `ACK`  | CR1 | 10 | Send ACK after a byte received |
| `POS`  | CR1 | 11 | ACK position (used for `len==2` reads) |
| `FREQ` | CR2 | 5:0 | APB1 frequency, MHz |
| `FS`   | CCR | 15 | 0 = Sm, 1 = Fm |
| `DUTY` | CCR | 14 | Fm duty cycle |
| `CCR`  | CCR | 11:0 | Clock control divider |
| `TRISE`| TRISE | 5:0 | Maximum rise time |
| `SB`   | SR1 | 0 | Start bit generated |
| `ADDR` | SR1 | 1 | Address sent + ACK received |
| `BTF`  | SR1 | 2 | Byte transfer finished |
| `RXNE` | SR1 | 6 | RX buffer not empty |
| `TXE`  | SR1 | 7 | TX buffer empty |

### 12.2 SPI bit fields we touched

| Field | Register | Bits | Meaning |
|---|---|---|---|
| `CPHA` | CR1 | 0 | Clock phase |
| `CPOL` | CR1 | 1 | Clock polarity |
| `MSTR` | CR1 | 2 | 1 = master, 0 = slave |
| `BR`   | CR1 | 5:3 | Baud-rate prescaler |
| `SPE`  | CR1 | 6 | SPI enable |
| `LSBFIRST` | CR1 | 7 | LSB-first if 1 |
| `SSI`  | CR1 | 8 | Internal slave select (with SSM) |
| `SSM`  | CR1 | 9 | Software slave management |
| `RXONLY` | CR1 | 10 | Receive-only mode |
| `DFF`  | CR1 | 11 | 0 = 8-bit, 1 = 16-bit frame |
| `BIDIMODE` | CR1 | 15 | Half-duplex (1-wire) mode |
| `SSOE` | CR2 | 2 | SS output enable (hardware NSS) |
| `RXNE` | SR | 0 | RX buffer not empty |
| `TXE`  | SR | 1 | TX buffer empty |
| `OVR`  | SR | 6 | Overrun |
| `BSY`  | SR | 7 | Bus busy |
| `MODF` | SR | 5 | Mode fault (NSS conflict) |

### 12.3 One-page cheat sheet

```c
/* ----------------- I2C ----------------- */
I2C_Handle_t hi2c1 = { .I2Cx = I2C1,
                       .I2C_Config.I2C_Speed_mode = Standard_Mode,
                       .I2C_Config.I2C_SCL_Speed  = 100000 };
GPIO_Configurations_SCL_SDA();
I2C_Init(&hi2c1);

uint8_t tx[3] = { 0x55, 0xAA, 0x01 };
I2C_Transmit_Buffer(I2C1, 0x00, tx, 3, 0x68);

uint8_t rx[3];
I2C_Receive_Buffer(I2C1, 0x00, rx, 3, 0x68);

/* ----------------- SPI ----------------- */
SPI_Handle_t hspi1 = { .pSPIx = SPI1,
                       .SPI_Config.SPI_Bus_Config  = SPI_BUS_CONFIG_FD,
                       .SPI_Config.SPI_Device_Mode = SPI_DEVICE_MODE_MASTER,
                       .SPI_Config.SPI_CLK_Speed   = SPI_PRESCALAR_16,
                       .SPI_Config.SPI_SSM         = SPI_SSM_EN,
                       .SPI_Config.SPI_DataFrame   = SPI_DFF_8BITS,
                       .SPI_Config.SPI_CPOL        = SPI_CPOL_LOW,
                       .SPI_Config.SPI_CPHA        = SPI_CPHA_LOW };
SPI_GPIO_Configurations();
SPI_Init(&hspi1);

GPIOB->ODR &= ~(1<<6);            /* CS low  */
SPI_TransmitByte(SPI1, 0x80);     /* read cmd */
uint8_t who = SPI_ReceiveByte(SPI1);
GPIOB->ODR |=  (1<<6);            /* CS high */
```

### 12.4 Study order

If you read the project end-to-end, the recommended order is:

1. `I2C_Baremetal.h` → see the public surface
2. `I2C_Baremetal.c` → see how each function touches the registers (read the comments line by line)
3. `STM32F401RE_I2C_Driver.h` → see the configuration struct
4. `STM32F401RE_I2C_Driver.c` → see the three branches of `I2C_Receive_Buffer` side-by-side with the RM
5. `RTC_I2C_Interface.c` → see the application
6. Repeat for SPI (header → source → application)
7. Write your own I²C loopback app
8. Write your own SPI loopback app
9. Port a new sensor (e.g. MPU6050) — you're now a peripheral driver author

---

*End of study material. For the original project, see `https://github.com/Karthikmg25/Drivers-project`. The test reports in `Test case/Test document/{I2C,SPI}/README.md` are also worth reading for the actual numerical results of the validation runs.*
