# STM32G030K8

Project firmware cho board dùng `STM32G030K8T6`.

## Trạng thái hiện tại

- `main.c` chỉ làm nhiệm vụ bootstrap, sau đó chuyển quyền điều phối cho `app_init()` và `app_main()`.
- Debug log đi qua `USART2` tại header `J3`.
- I2C test hiện scan `I2C2` một lần lúc boot để tìm thiết bị ngoài.
- AI hiện đọc `ADS1115` theo vòng 4 kênh và expose qua `Input Registers (FC04)`.
- RS485 chạy trên `USART1`, điều khiển chân `EN-485` bằng `PC6`.
- Modbus RTU hiện hỗ trợ:
  - `0x03 Read Holding Registers`
  - `0x04 Read Input Registers`
  - `0x06 Write Single Register`

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

- Holding Register `0x0001`: `Slave Address`

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
rs485/modbus ready @ 115200
i2c2 scan range: 0x08..0x77
i2c2 devices: 2
i2c2 ack @ 0x48
i2c2 ack @ 0x60
ads1115 ack @ 0x48
mcp4728 ack @ 0x60
ai ads1115 ready @ 0x48
```

### 2. Đọc địa chỉ Modbus

Dùng `Modbus Poll` hoặc tool tương đương trên cổng RS485:

- mode: `RTU`
- baudrate: `115200`
- data bits: `8`
- parity: `None`
- stop bits: `1`

Đọc:

- `Slave ID = địa chỉ hiện tại của board`
- `Function = 03`
- `Address = 1`
- `Quantity = 1`

### 3. Ghi địa chỉ Modbus mới

Ghi:

- `Slave ID = địa chỉ hiện tại của board`
- `Function = 06`
- `Address = 1`
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

Lưu ý:

- Firmware đang theo ví dụ của tài liệu: `5000` nghĩa là `5.000V`.
- Vì vậy `AIx_SCALED` hiện được lưu theo `mV`, dù cột scale trong bảng gốc có ghi chưa nhất quán.

Ví dụ test nhanh bằng `Modbus Poll`:

- Đọc `AI1_RAW`: `Slave ID = địa chỉ hiện tại`, `Function = 04`, `Address = 0`, `Quantity = 1`
- Đọc `AI1..AI4_RAW`: `Function = 04`, `Address = 0`, `Quantity = 4`
- Đọc `AI1..AI4_SCALED`: `Function = 04`, `Address = 4`, `Quantity = 4`

## Cấu trúc thư mục

- `Core/app`: tầng điều phối chính
- `Core/config`: hằng số cấu hình và cấu hình lưu Flash
- `Core/protocols`: RS485 mức thấp, Modbus RTU và I2C bus scan
- `Core/utils`: debug log và tiện ích dùng chung

Mỗi thư mục trên đều có `README.md` riêng để mô tả chi tiết hơn.
