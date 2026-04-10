# Utils

Thư mục này chứa các tiện ích dùng lại được ở nhiều nơi trong dự án.

## File trong thư mục

`debug_log.h`
- Khai báo hàm log debug.
- Cung cấp macro `DEBUG_LOG(...)` để bật tắt log theo macro `DEBUG`.

`debug_log.c`
- Cài đặt kênh debug UART bằng `USART2`.
- Baudrate debug hiện tại là `115200 8N1`, tách biệt với baudrate `RS485/Modbus`.
- Dùng để in chuỗi debug như `boot ok` ra cổng serial.
- Đây là tầng log kỹ thuật, không chứa logic ứng dụng.

## Nguyên tắc sử dụng

- Thư mục `utils` chứa các hàm phụ trợ, không chứa flow chính của hệ thống.
- Khi app, protocol, hoặc BSP cần in log, nên gọi qua `DEBUG_LOG(...)` thay vì tự in trực tiếp.
