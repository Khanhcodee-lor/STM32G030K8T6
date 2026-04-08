# App

Thư mục này chứa tầng điều phối chính của hệ thống.

## File trong thư mục

`app_main.h`
- Khai báo 2 hàm đầu vào của ứng dụng: `app_init()` và `app_main()`.
- `main.c` chỉ cần biết đến file này.

`app_main.c`
- Điều phối các module con sau khi MCU đã init xong HAL, clock và GPIO.
- Hiện tại:
- Bật UART debug.
- Đọc cấu hình thiết bị từ Flash.
- Khởi tạo RS485.
- Khởi tạo Modbus RTU.
- In log boot và địa chỉ Modbus hiện tại.
- Chạy heartbeat LED.
- Poll Modbus RTU liên tục.

## Ghi chú kỹ thuật

- Theo tài liệu gốc, địa chỉ Modbus phải lấy từ DIP switch 3 bit.
- Vì phần cứng hiện tại chưa có DIP switch tương ứng, app đang dùng cấu hình địa chỉ qua Modbus + Flash thay thế.
- Khi phần cứng có DIP switch đúng thiết kế, phần này có thể đổi lại sang cơ chế đọc chân input.

## Nguyên tắc sử dụng

- `app` là nơi ghép các module lại với nhau.
- Không nên đặt thanh ghi mức thấp, code UART raw, hoặc logic Flash trực tiếp tại đây nếu đã có module riêng.
