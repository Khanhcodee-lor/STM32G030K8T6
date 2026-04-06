# Config

Thư mục này chứa các giá trị cấu hình dùng chung cho ứng dụng.

## File trong thư mục

`app_config.h`
- Khai báo các macro cấu hình dùng trong app.
- Hiện tại chứa chu kỳ nháy LED heartbeat và baudrate test RS485.
- Đây là nơi ưu tiên sửa khi cần đổi tham số vận hành mà không muốn sửa logic.

`app_config.c`
- Chứa các biến cấu hình dạng dữ liệu toàn cục.
- Hiện tại chứa chuỗi banner test `RS485 boot ok`.
- Dùng cho các cấu hình dạng chuỗi, bảng dữ liệu, hoặc giá trị cần cấp phát bộ nhớ.

## Nguyên tắc sử dụng

- Thư mục `config` chỉ chứa cấu hình, không chứa logic điều khiển.
- Khi cần đổi tốc độ test, chu kỳ LED, message khởi động, ưu tiên sửa tại đây trước.
