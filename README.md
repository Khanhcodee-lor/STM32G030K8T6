# STM32G030K8

Project firmware cho board dùng `STM32G030K8T6`.

## Trạng thái hiện tại

- `main.c` chỉ làm nhiệm vụ bootstrap, sau đó chuyển quyền điều phối cho `app_init()` và `app_main()`.
- Debug log đi qua `USART2` tại header `J3`.
- RS485 chạy trên `USART1`, điều khiển chân `EN-485` bằng `PC6`.
- Modbus RTU hiện hỗ trợ:
  - `0x03 Read Holding Registers`
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

## Cấu trúc thư mục

- `Core/app`: tầng điều phối chính
- `Core/config`: hằng số cấu hình và cấu hình lưu Flash
- `Core/protocols`: RS485 mức thấp và Modbus RTU
- `Core/utils`: debug log và tiện ích dùng chung

Mỗi thư mục trên đều có `README.md` riêng để mô tả chi tiết hơn.
