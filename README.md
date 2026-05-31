# HỆ THỐNG TƯỚI TỰ ĐỘNG VÀ ĐIỀU KHIỂN BLYNK

Dự án triển khai hệ thống tưới cây IoT dùng ESP32, đọc độ ẩm đất và điều khiển bơm qua relay. Hệ thống hoạt động ở chế độ tự động (dựa trên ngưỡng độ ẩm) và thủ công (điều khiển từ Blynk), đồng thời gửi dữ liệu về Blynk Cloud theo thời gian thực.

## Thông tin môn học
- Môn học: Hệ thống nhúng mạng không dây
- Giảng viên hướng dẫn: ThS. Lê Minh Khánh Hội
- Video demo: https://youtu.be/zK3I3auDBkM
- Báo cáo chi tiết: https://drive.google.com/file/d/1dm_5DiGxELEaVF1ul7Sg3U9702IOB9W1/view?usp=sharing 

## Thành viên
| STT | Họ và tên | MSSV |
| --- | --- | --- |
| 1 | Nguyễn Tiến Vĩnh | 24522022 |
| 2 | Bùi Phúc Tâm | 24521564 |
| 3 | Trần Văn Tài | 24521560 |
| 4 | Phạm Thành Danh | 24520265 |

## Cấu trúc thư mục
```
platformio.ini
README.md
include/
	README
lib/
	README
src/
	main.cpp
test/
	README
```
## Tính năng
- Tự động bật/tắt bơm theo độ ẩm đất và ngưỡng hysteresis.
- Thủ công bật/tắt bơm từ app Blynk.
- Cập nhật độ ẩm (%) và đồng bộ trạng thái bơm lên dashboard.
- Điều chỉnh ngưỡng bật/tắt trực tiếp trên Blynk (không cần sửa code).

## Kiến trúc hệ thống
- ESP32 đọc cảm biến độ ẩm đất (analog) và điều khiển relay 1 kênh.
- Relay kích hoạt bơm chìm mini 5V.
- ESP32 kết nối WiFi và giao tiếp Blynk Cloud; Blynk App/Web dashboard hiển thị và điều khiển.

## Phần cứng chính
- ESP32 NodeMCU (ESP32 DevKit).
- Cảm biến độ ẩm đất điện dung (analog).
- Module relay 1 kênh 5V.
- Bơm chìm mini 5V (1.2-1.6 L/phút).
- Nguồn 5V 2A, dây nối, breadboard. Chi tiết BOM xem Data/materials-list.txt.

## Kết nối chân
- GPIO34 -> Vout cảm biến độ ẩm (ADC).
- GPIO26 -> IN relay (relay active HIGH).
- Bơm -> relay -> nguồn 5V; tất cả chung GND.

## Blynk datastream (virtual pins)
| Pin | Chức năng | Ghi chú |
| --- | --- | --- |
| V0 | Công tắc bơm | Chỉ có hiệu lực ở Manual; Auto sẽ đồng bộ lại trạng thái |
| V1 | Độ ẩm (%) | Gửi lên mỗi 2 giây |
| V2 | Mode | 1 = Auto, 0 = Manual |
| V3 | Ngưỡng bật (%) | Mặc định 40 |
| V4 | Ngưỡng tắt (%) | Mặc định 75 |

## Cấu hình nhanh
1. Mở thư mục Firmware bằng PlatformIO (VS Code).
2. Sửa Firmware/src/main.cpp:
	- BLYNK_TEMPLATE_ID, BLYNK_TEMPLATE_NAME, BLYNK_AUTH_TOKEN
	- WIFI_SSID, WIFI_PASS (chỉ băng tần 2.4GHz)
3. Tạo datastream trên Blynk theo V0..V4 và tạo dashboard.
4. Build/Upload lên ESP32 và mở Serial Monitor (115200).

## Thông số quan trọng trong code
- Đọc cảm biến mỗi 2 giây; gửi Blynk mỗi 2 giây.
- ADC_KHO = 3050, ADC_UOT = 1390 (cần hiệu chỉnh theo cảm biến/đất).
- Tự động bật bơm khi raw > nguongBatADC, tắt bơm khi raw < nguongTatADC (hysteresis).
