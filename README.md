# STM32G030K8

Project firmware cho board dùng `STM32G030K8T6`.

## Trạng thái hiện tại

- `main.c` chỉ làm nhiệm vụ bootstrap, sau đó chuyển quyền điều phối cho `app_init()` và `app_main()`.
- Debug log đi qua `USART2` tại header `J3`.
- I2C test hiện scan `I2C2` một lần lúc boot để tìm thiết bị ngoài.
- AI hiện đọc `ADS1115` theo vòng 4 kênh và expose qua `Input Registers (FC04)`.
- AO hiện ghi `MCP4728` theo 4 kênh và expose qua `Holding Registers`.
- RS485 chạy trên `USART1`, điều khiển chân `EN-485` bằng `PC6`.
- Modbus RTU hiện hỗ trợ:
  - `0x03 Read Holding Registers`
  - `0x04 Read Input Registers`
  - `0x06 Write Single Register`
  - `0x10 Write Multiple Registers`

## Lưu ý so với tài liệu gốc

Tài liệu ban đầu mô tả địa chỉ Modbus RTU được chọn bằng `DIP switch 3 bit` với dải địa chỉ `1..7`.

Phần cứng hiện tại chưa có cụm `DIP switch 3 bit`, nên firmware đang dùng cơ chế thay thế:

- cấu hình địa chỉ qua Modbus RTU
- lưu địa chỉ xuống Flash nội bộ
- sau khi mất nguồn, board vẫn nhớ địa chỉ đã lưu
- dải địa chỉ vẫn giữ đúng theo tài liệu: `1..7`

Nói ngắn gọn:

- tài liệu gốc: `DIP switch -> địa chỉ Modbus`
- phần cứng hiện tại: `Modbus register -> Flash -> địa chỉ Modbus`

Khi phần cứng được bổ sung đúng `DIP switch 3 bit`, firmware có thể đổi lại cơ chế đọc địa chỉ từ input GPIO.

## Thanh ghi Modbus hiện có

- Holding Registers `0x0000..0x0003`: `AO1_SETPOINT..AO4_SETPOINT`
- Holding Registers `0x0004..0x0007`: `AO1_MODE..AO4_MODE`
- Holding Register `0x0008`: `DEVICE_ID` (Modbus address hiện tại)
- Holding Register `0x0009`: `FW_VERSION`
- Holding Register `0x000A`: `STATUS`
- Holding Register `0x000B`: `ERROR_CODE`
- Input Registers `0x0000..0x0007`: `AI1_RAW..AI4_RAW`, `AI1_SCALED..AI4_SCALED`
- Debug Input Registers `0x0100..0x0103`: `AI1_BOARD_RAW..AI4_BOARD_RAW`
- Debug Input Registers `0x0104..0x0107`: `AI1_ADC_CODE..AI4_ADC_CODE`

Giới hạn giá trị:

- nhỏ nhất: `1`
- lớn nhất: `7`
- mặc định khi Flash chưa hợp lệ: `1`

## Cách test nhanh

### 1. Xem log debug

Dùng USB-UART TTL 3.3V nối vào `J3`:

- `J3 pin 1 = TX2`
- `J3 pin 2 = RX2`
- `J3 pin 4 = GND`

Mở terminal với cấu hình:

- `115200`
- `8 data bits`
- `no parity`
- `1 stop bit`

Khi board boot, log mẫu:

```text
boot ok
modbus addr: 1
rs485/modbus ready @ 9600
i2c2 scan range: 0x08..0x77
i2c2 devices: 2
i2c2 ack @ 0x48
i2c2 ack @ 0x60
ads1115 ack @ 0x48
mcp4728 ack @ 0x60
ai ads1115 ready @ 0x48
ao mcp4728 ready @ 0x60
```

### 2. Đọc địa chỉ Modbus

Dùng `Modbus Poll` hoặc tool tương đương trên cổng RS485:

- Lưu ý: đây là baudrate của `RS485/Modbus (USART1)`, khác với UART debug `J3/USART2`.
- mode: `RTU`
- baudrate: `9600`
- data bits: `8`
- parity: `None`
- stop bits: `1`

Đọc:

- `Slave ID = địa chỉ hiện tại của board`
- `Function = 03`
- `Address = 8`
- `Quantity = 1`

### 3. Ghi địa chỉ Modbus mới

Ghi:

- `Slave ID = địa chỉ hiện tại của board`
- `Function = 06`
- `Address = 8`
- `Value = địa chỉ mới`

Sau đó cấp nguồn lại board và kiểm tra log:

```text
modbus addr: <địa chỉ mới>
```

### 4. Test bus I2C

Sau khi boot, xem thêm log scan I2C trên cùng cổng debug UART:

- `i2c2 ack @ 0x48..0x4B`: có thiết bị ACK trong dải địa chỉ của `ADS1115`
- `i2c2 ack @ 0x60..0x67`: có thiết bị ACK trong dải địa chỉ của `MCP4728`
- `i2c2 scan: no ack`: MCU chưa thấy thiết bị nào trả ACK trên bus

Lưu ý:

- Bước này xác nhận bus và địa chỉ có phản hồi ACK.
- Đây chưa phải bước định danh tuyệt đối IC bằng thanh ghi riêng của từng chip.

### 5. Đọc AI qua Modbus

AI hiện dùng `Function Code = 04` với các `Input Registers`:

- `0x0000..0x0003`: `AI1_RAW..AI4_RAW`, dải `0..4095`
- `0x0004..0x0007`: `AI1_SCALED..AI4_SCALED`, giá trị theo `mV`, dải `0..10000`
- `0x0100..0x0103`: `AI1_BOARD_RAW..AI4_BOARD_RAW`, debug value trong miền analog của board
- `0x0104..0x0107`: `AI1_ADC_CODE..AI4_ADC_CODE`, mã đọc gốc từ `ADS1115`

Lưu ý:

- Firmware đang theo ví dụ của tài liệu: `5000` nghĩa là `5.000V`.
- Vì vậy `AIx_SCALED` hiện được lưu theo `mV`, dù cột scale trong bảng gốc có ghi chưa nhất quán.
- `0x0000..0x0007` là vùng `spec-facing`, tức contract public của module với controller/tester.
- Firmware không trả thẳng mã ADC gốc của `ADS1115` ở vùng `spec-facing`.
- Trong code có khái niệm `board raw`: giá trị debug nội bộ trong miền analog của board, trước khi ép về spec Modbus.
- `board raw` sau đó được ép lại về spec tài liệu để đầu ra public vẫn là:
  - `AI_RAW = 0..4095`
  - `AI_SCALED = 0..10000 mV`
- Hệ số ép hiện đang đặt trong `app_config.h` tại macro `APP_AI_CALIBRATED_RAW_AT_10V`.
- Vùng `0x0100..0x0107` là `internal/debug`, dùng cho bring-up và calibration, không phải contract chính của sản phẩm.

Ví dụ test nhanh bằng `Modbus Poll`:

- Đọc `AI1_RAW`: `Slave ID = địa chỉ hiện tại`, `Function = 04`, `Address = 0`, `Quantity = 1`
- Đọc `AI1..AI4_RAW`: `Function = 04`, `Address = 0`, `Quantity = 4`
- Đọc `AI1..AI4_SCALED`: `Function = 04`, `Address = 4`, `Quantity = 4`
- Đọc `AI1..AI4_BOARD_RAW` debug: `Function = 04`, `Address = 256`, `Quantity = 4`
- Đọc `AI1..AI4_ADC_CODE` debug: `Function = 04`, `Address = 260`, `Quantity = 4`

### 6. Ghi AO qua Modbus

AO hiện dùng `Holding Registers`:

- `0x0000..0x0003`: `AO1_SETPOINT..AO4_SETPOINT`, dải `0..4095`
- `0x0004..0x0007`: `AO1_MODE..AO4_MODE`, `0 = 0..10V`, `1 = 4..20mA`

Ví dụ test nhanh:

- Set `AO1_SETPOINT = 2048`: `Function = 06`, `Address = 0`, `Value = 2048`
- Set `AO1_MODE = 0`: `Function = 06`, `Address = 4`, `Value = 0`
- Ghi nhiều setpoint/mode liên tiếp: `Function = 16`, địa chỉ bắt đầu trong dải `0x0000..0x0007`

Lưu ý:

- `AO_MODE` hiện được expose đúng map Modbus và giữ trong firmware.
- Việc ngõ ra thực tế là `0..10V` hay `4..20mA` còn phụ thuộc mạch phần cứng của từng kênh.

## Cấu trúc thư mục

- `Core/app`: tầng điều phối chính
- `Core/config`: hằng số cấu hình và cấu hình lưu Flash
- `Core/protocols`: RS485 mức thấp, Modbus RTU và I2C bus scan
- `Core/utils`: debug log và tiện ích dùng chung

Mỗi thư mục trên đều có `README.md` riêng để mô tả chi tiết hơn.
