# Config

Thư mục này chứa các giá trị cấu hình dùng chung cho ứng dụng và cấu hình thiết bị lưu bền vững.

## File trong thư mục

`app_config.h`
- Khai báo các macro cấu hình dùng trong app.
- Hiện tại chứa chu kỳ nháy LED heartbeat, baudrate RS485/Modbus, giới hạn địa chỉ Modbus và thông tin vùng Flash dành cho config.

`app_config.c`
- Chứa các biến cấu hình dạng dữ liệu toàn cục.
- Hiện tại gần như chỉ dùng cho dữ liệu cấu hình tĩnh.

`device_config.h`
- Khai báo API đọc/ghi cấu hình thiết bị.
- Hiện tại dùng để lấy địa chỉ Modbus hiện tại, đổi địa chỉ và lưu xuống Flash.

`device_config.c`
- Cài đặt phần lưu cấu hình thiết bị vào Flash.
- Hiện tại lưu địa chỉ Modbus RTU để board vẫn nhớ sau khi mất nguồn.

## Ghi chú kỹ thuật

- Theo tài liệu gốc, địa chỉ Modbus lấy từ DIP switch 3 bit.
- Phần cứng hiện tại chưa có cụm DIP switch đó, nên firmware chuyển sang cấu hình địa chỉ qua Modbus và lưu Flash.
- Dải địa chỉ vẫn giữ đúng yêu cầu tài liệu: `1..7`.
- Holding Register dùng để cấu hình địa chỉ hiện tại là `0x0001`.

## Nguyên tắc sử dụng

- Thư mục `config` chỉ chứa cấu hình và dữ liệu cấu hình, không chứa flow điều khiển chính.
- Khi cần đổi giới hạn địa chỉ, baudrate, timeout, hoặc vị trí vùng Flash, ưu tiên sửa tại đây.
