// ============================================================
//  HỆ THỐNG TƯỚI TỰ ĐỘNG — ESP32 + Blynk
//  V0: Water Pump (switch + trạng thái)
//  V1: Độ ẩm (gauge 0-100%)
//  V2: Mode (0=Manual, 1=Auto)
// ============================================================

#define BLYNK_TEMPLATE_ID "TMPL6PV6oNWPZ"
#define BLYNK_TEMPLATE_NAME "Irrigation System"
#define BLYNK_AUTH_TOKEN "2OsFLPU534pFfxUuUNEDEYDxz-nvsSAx"

#define BLYNK_PRINT Serial  // In log Blynk ra Serial Monitor

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// ── Thông tin Wifi ──────────────────────────────────────────
const char* WIFI_SSID = "CameraAnNinh";   // Thay tên Wifi của bạn
const char* WIFI_PASS = "11223344";    // Thay mật khẩu Wifi

// ── Chân GPIO ───────────────────────────────────────────────
#define PIN_SENSOR  34
#define PIN_RELAY   26

// ── Ngưỡng độ ẩm (căn chỉnh theo thực tế của bạn) ──────────
#define NGUONG_KHO  2500   // ADC > 2500 → đất khô → bật bơm
#define NGUONG_AM   1800   // ADC < 1800 → đất ẩm → tắt bơm
#define ADC_KHO     3050   // Giá trị ADC khi khô hoàn toàn
#define ADC_UOT     1390   // Giá trị ADC khi ướt hoàn toàn

// ── Thời gian ────────────────────────────────────────────────
#define INTERVAL_DOC_SENSOR  2000  // Đọc cảm biến mỗi 2 giây
#define INTERVAL_GUI_BLYNK   2000  // Gửi dữ liệu lên Blynk mỗi 2 giây

// ── Biến trạng thái ─────────────────────────────────────────
int  mode       = 1;     // 1=Auto (mặc định), 0=Manual
bool bompDangBat = false; // Trạng thái bơm hiện tại
int  doAmPhanTram = 0;   // Độ ẩm % hiện tại

// ── Timer của Blynk ─────────────────────────────────────────
BlynkTimer timer;

// ============================================================
//  HÀM PHỤ: Bật/Tắt bơm và cập nhật Blynk
// ============================================================
void batBom() {
    if (!bompDangBat) {
        digitalWrite(PIN_RELAY, HIGH);   // Active LOW → bật relay
        bompDangBat = true;
        Blynk.virtualWrite(V0, 1);      // Cập nhật switch trên app
        Serial.println("[BOM] BAT");
    }
}

void tatBom() {
    if (bompDangBat) {
        digitalWrite(PIN_RELAY, LOW);  // Active LOW → tắt relay
        bompDangBat = false;
        Blynk.virtualWrite(V0, 0);      // Cập nhật switch trên app
        Serial.println("[BOM] TAT");
    }
}

// ============================================================
//  HÀM CHÍNH: Đọc cảm biến + xử lý Auto
// ============================================================
void docCamBienVaXuLy() {
    int raw = analogRead(PIN_SENSOR);

    // Quy đổi ADC sang phần trăm độ ẩm
    doAmPhanTram = map(raw, ADC_UOT, ADC_KHO, 100, 0);
    doAmPhanTram = constrain(doAmPhanTram, 0, 100);

    Serial.print("[SENSOR] ADC: ");
    Serial.print(raw);
    Serial.print("  |  Do am: ");
    Serial.print(doAmPhanTram);
    Serial.print("%  |  Mode: ");
    Serial.println(mode == 1 ? "AUTO" : "MANUAL");

    // Gửi độ ẩm lên Blynk
    Blynk.virtualWrite(V1, doAmPhanTram);

    // Chỉ xử lý tự động nếu đang ở chế độ Auto
    if (mode == 1) {
        if (raw > NGUONG_KHO) {
            batBom();
        } else if (raw < NGUONG_AM) {
            tatBom();
        }
        // Vùng trung gian: giữ nguyên trạng thái (hysteresis)
    }
}

// ============================================================
//  BLYNK: Nhận lệnh từ Switch V0 (Water Pump)
// ============================================================
BLYNK_WRITE(V0) {
    if (mode == 0) {  // Chỉ xử lý khi đang Manual
        int lenh = param.asInt();
        if (lenh == 1) {
            batBom();
        } else {
            tatBom();
        }
        Serial.print("[MANUAL] Lenh tu app: ");
        Serial.println(lenh == 1 ? "BAT BOM" : "TAT BOM");
    } else {
        // Auto mode: bỏ qua lệnh switch, đồng bộ lại trạng thái thật
        Serial.println("[AUTO] Bo qua lenh switch tu app");
        Blynk.virtualWrite(V0, bompDangBat ? 1 : 0);
    }
}

// ============================================================
//  BLYNK: Nhận lệnh từ Switch V2 (Mode)
// ============================================================
BLYNK_WRITE(V2) {
    mode = param.asInt();
    Serial.print("[MODE] Chuyen sang: ");
    Serial.println(mode == 1 ? "AUTO" : "MANUAL");

    // Khi chuyển sang Manual → tắt bơm để an toàn
    if (mode == 0) {
        tatBom();
        Serial.println("[MODE] Tat bom de an toan khi chuyen Manual");
    }
}

// ============================================================
//  BLYNK: Đồng bộ trạng thái khi ESP32 mới kết nối
// ============================================================
BLYNK_CONNECTED() {
    // Lấy trạng thái hiện tại của các switch từ Blynk Cloud
    // về ESP32 (quan trọng khi ESP32 bị reset)
    Blynk.syncVirtual(V0, V2);
    Serial.println("[BLYNK] Da ket noi, dong bo trang thai...");
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
    Serial.begin(115200);

    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);  // Tắt bơm ngay khi khởi động

    Serial.println("=== He thong tuoi tu dong khoi dong ===");

    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

    // Đăng ký timer đọc cảm biến mỗi 2 giây
    timer.setInterval(INTERVAL_DOC_SENSOR, docCamBienVaXuLy);

    Serial.println("=== San sang ===");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
    Blynk.run();   // Giữ kết nối Blynk + xử lý lệnh từ app
    timer.run();   // Chạy các timer đã đăng ký
}