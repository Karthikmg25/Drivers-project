# RTC I2C Interface Application – Test Cases

## Positive Test Cases

| Test Case ID | Description | Test Steps | Expected Result | Status |
|-------------|-------------|-------------|-----------------|---------|
| TC_POS_01 | Verify I2C Communication with RTC | Connect RTC module and run the application. | RTC acknowledges its slave address (`0x68`) and communication is established successfully. | Pass |
| TC_POS_02 | Verify RTC Time Initialization | Set time values (`23:10:00`) and write them to RTC registers. | RTC registers `0x00–0x02` store the transmitted BCD values correctly. | Pass |
| TC_POS_03 | Verify RTC Register Read Operation | Read registers `0x00–0x02` using `I2C_Receive_Buffer()`. | Valid BCD values for seconds, minutes, and hours are received. | Pass |
| TC_POS_04 | Verify BCD to Decimal Conversion | Read RTC time and perform BCD-to-decimal conversion. | Converted values match the actual stored time. | Pass |
| TC_POS_05 | Verify Continuous Time Update | Execute the application for several minutes and observe USART output. | Time increments correctly every second and is displayed continuously. | Pass |

---

## Negative Test Cases

| Test Case ID | Description | Test Steps | Expected Result | Status |
|-------------|-------------|-------------|-----------------|---------|
| TC_NEG_01 | RTC Module Disconnected | Disconnect RTC module and run the application. | No ACK received from slave address `0x68`. Communication fails. | Pass |
| TC_NEG_02 | Incorrect Slave Address | Change `RTC_ADDR` to an invalid address and execute the application. | RTC does not respond and no valid data is received. | Pass |
| TC_NEG_03 | Invalid Register Address Read | Attempt to read from a non-existent RTC register. | Returned data is invalid or does not correspond to valid time values. | Pass |
| TC_NEG_04 | Invalid BCD Data Written | Write values exceeding valid BCD ranges (e.g., `0x6A` seconds). | RTC stores incorrect data or displays invalid time values during readback. | Pass |
| TC_NEG_05 | SDA/SCL Connection Fault | Disconnect SDA or SCL line during operation. | I2C transaction fails and time cannot be read from RTC. | Pass |

---

## Test Summary

| Parameter | Value |
|-----------|-------|
| Total Positive Test Cases | 5 |
| Total Negative Test Cases | 5 |
| Total Test Cases | 10 |

### Result

The RTC I2C interface was successfully validated for:

- Time initialization
- RTC register read/write operations
- BCD-to-decimal conversion
- Continuous time monitoring

Error handling was verified under fault conditions such as:

- Invalid slave addresses
- Communication failures
- Invalid register access
- Incorrect BCD data
- SDA/SCL line faults

## Video link:
https://drive.google.com/file/d/1Zke7yg92_cRkQpAyuT1BJ1SlK_HgIIM1/view?usp=drivesdk