# Protocols

Thư mục này chứa tầng giao tiếp truyền thông ở mức thấp hoặc mức giao thức.

## File trong thư mục

`rs485_ll.h`
- Khai báo API mức thấp cho RS485.
- Cung cấp 3 hàm chính: init, gửi dữ liệu, đọc 1 byte nhận được.

`rs485_ll.c`
- Cài đặt RS485 mức thấp bằng thanh ghi `USART1`.
- Điều khiển chân `EN` để chuyển giữa transmit và receive.
- Chưa có Modbus, chỉ mới đảm bảo tầng UART/RS485 hoạt động ổn định để test.

## Nguyên tắc sử dụng

- Thư mục `protocols` xử lý giao tiếp, không xử lý nghiệp vụ app.
- `app_main` chỉ nên gọi API trong đây, không nên ghi trực tiếp vào thanh ghi `USART1`.
- Sau này nếu thêm Modbus RTU thì đặt module mới cũng trong thư mục này.
