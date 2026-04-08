# Protocols

Thư mục này chứa tầng giao tiếp truyền thông ở mức thấp hoặc mức giao thức.

## File trong thư mục

`rs485_ll.h`
- Khai báo API mức thấp cho RS485.
- Cung cấp các hàm init, gửi dữ liệu và đọc byte nhận được.

`rs485_ll.c`
- Cài đặt RS485 mức thấp bằng `USART1`.
- Điều khiển chân `EN` để chuyển giữa transmit và receive.
- Đây là tầng UART/RS485 vật lý, chưa chứa logic nghiệp vụ.

`modbus_rtu.h`
- Khai báo API Modbus RTU mức ứng dụng.
- App chỉ cần gọi `init` và `poll`.

`modbus_rtu.c`
- Cài đặt Modbus RTU tối thiểu trên nền RS485 hiện tại.
- Hiện hỗ trợ:
- `0x03 Read Holding Registers`
- `0x06 Write Single Register`
- Dùng để đọc/ghi thanh ghi địa chỉ Modbus và lưu địa chỉ xuống Flash.
- Holding Register đang dùng là `0x0001`.

`i2c_bus_scan.h`
- Khai báo API scan bus `I2C2` và struct báo cáo kết quả scan.

`i2c_bus_scan.c`
- Quét dải địa chỉ I2C hợp lệ, gom các địa chỉ trả ACK và đánh dấu dải kỳ vọng cho `ADS1115` và `MCP4728`.
- Dùng cho bước bring-up phần cứng trước khi viết app AI/AO.

## Ghi chú kỹ thuật

- Theo tài liệu gốc, địa chỉ Modbus lấy từ DIP switch 3 bit.
- Vì phần cứng hiện tại thiếu DIP switch, module Modbus được dùng để cấu hình địa chỉ slave rồi lưu Flash.
- Dải địa chỉ áp dụng vẫn là `1..7`.
- Module scan I2C hiện chỉ xác nhận ACK ở địa chỉ mong đợi, chưa đọc thanh ghi riêng để định danh tuyệt đối IC.

## Nguyên tắc sử dụng

- `protocols` xử lý giao tiếp và giao thức, không xử lý điều phối hệ thống.
- `app_main` chỉ nên gọi API trong đây, không nên ghi trực tiếp vào thanh ghi `USART1`.
