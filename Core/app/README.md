# App

Thư mục này chứa tầng điều phối chính của hệ thống.

## File trong thư mục

`app_main.h`
- Khai báo 2 hàm mức vào của ứng dụng: `app_init()` và `app_main()`.
- `main.c` chỉ cần biết đến file này.

`app_main.c`
- Điều phối các module con sau khi MCU đã init xong HAL, clock và GPIO.
- Hiện tại:
- Gọi `DebugLog_Init()` để bật UART debug.
- Gọi `Rs485Ll_Init()` để bật RS485 mức thấp.
- Gửi log `boot ok` và banner test RS485.
- Chạy heartbeat LED.
- Poll byte nhận được từ RS485 và echo lại để test.

## Nguyên tắc sử dụng

- Thư mục `app` là nơi ghép các module lại với nhau.
- Không nên đặt thanh ghi mức thấp, code UART raw, hoặc config cứng trực tiếp tại đây nếu đã có module riêng.
- Khi thêm AI, AO, RS485 protocol, Modbus, app chỉ nên gọi hàm và điều phối thứ tự thực hiện.
