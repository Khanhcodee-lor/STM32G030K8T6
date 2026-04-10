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
- Chạy scan `I2C2` một lần lúc boot để kiểm tra ACK của thiết bị ngoài.
- Khởi tạo module AI dựa trên địa chỉ `ADS1115` đã scan được.
- Khởi tạo module AO dựa trên địa chỉ `MCP4728` đã scan được.
- Chạy heartbeat LED.
- Poll vòng đọc `ADS1115` để cập nhật 4 kênh AI.
- Apply nền các thay đổi DAC cho 4 kênh AO.
- Poll Modbus RTU liên tục.

`analog_input.h`
- Khai báo API AI mức ứng dụng.
- Cho phép app init/process và cho Modbus đọc `Input Registers`.

`analog_input.c`
- Cài đặt đọc `ADS1115` theo vòng 4 kênh.
- Chuyển kết quả ADC sang `raw 0..4095` và `scaled 0..10000 mV`.
- Có thêm lớp quy đổi `board raw` để tách giá trị raw theo front-end thực tế của board ra khỏi giá trị raw theo spec Modbus.
- Nhờ vậy firmware vẫn có thể trả `AI_RAW/AI_SCALED` đúng format tài liệu ngay cả khi phần cứng analog thực tế cần calibration.
- Vùng `0x0000..0x0007` là `spec-facing` cho controller.
- Vùng debug riêng hiện expose thêm `board raw` và `ADS1115 ADC code` để thuận tiện khi test/calibration.

`analog_output.h`
- Khai báo API AO mức ứng dụng.
- Cho phép Modbus đọc/ghi holding registers và app apply DAC nền.

`analog_output.c`
- Cài đặt ghi `MCP4728` 4 kênh theo setpoint raw.
- Giữ thanh ghi mode AO ở mức firmware để bám theo map tài liệu.

## Ghi chú kỹ thuật

- Theo tài liệu gốc, địa chỉ Modbus phải lấy từ DIP switch 3 bit.
- Vì phần cứng hiện tại chưa có DIP switch tương ứng, app đang dùng cấu hình địa chỉ qua Modbus + Flash thay thế.
- Khi phần cứng có DIP switch đúng thiết kế, phần này có thể đổi lại sang cơ chế đọc chân input.

## Nguyên tắc sử dụng

- `app` là nơi ghép các module lại với nhau.
- Không nên đặt thanh ghi mức thấp, code UART raw, hoặc logic Flash trực tiếp tại đây nếu đã có module riêng.
