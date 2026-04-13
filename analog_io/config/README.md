# Config

Thư mục này chứa các giá trị cấu hình dùng chung cho ứng dụng và cấu hình thiết bị lưu bền vững.

## File trong thư mục

`app_config.h`
- Khai báo các macro cấu hình dùng trong app.
- Hiện tại chứa chu kỳ nháy LED heartbeat, baudrate RS485/Modbus, calibration AI/AO, map thanh ghi và thông tin vùng Flash dành cho config.

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
- `DEVICE_ID` hiện nằm ở Holding Register `0x0008`.
- Input Register public hiện giữ đúng contract `AI_RAW/AI_SCALED` ở `0x0000..0x0007`.
- Vùng `0x0100..0x0107` dành cho `internal/debug`, hiện dùng để expose `board raw` và `ADS1115 ADC code`.
- Calibration AI hiện dùng thêm khái niệm `board raw`, tức giá trị trong miền analog thực tế của board trước khi ép lại về spec Modbus.
- Macro `APP_AI_CALIBRATED_RAW_AT_10V` là điểm full-scale đang dùng để đổi `board raw` về `AI_RAW = 0..4095` và `AI_SCALED = 0..10000 mV`.

## Nguyên tắc sử dụng

- Thư mục `config` chỉ chứa cấu hình và dữ liệu cấu hình, không chứa flow điều khiển chính.
- Khi cần đổi giới hạn địa chỉ, baudrate, timeout, hoặc vị trí vùng Flash, ưu tiên sửa tại đây.
