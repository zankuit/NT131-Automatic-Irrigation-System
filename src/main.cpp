
#define BLYNK_TEMPLATE_ID "your-blynk-template-id"
#define BLYNK_TEMPLATE_NAME "your-blynk-template-name"
#define BLYNK_AUTH_TOKEN "your-blynk-auth-token"

#define BLYNK_PRINT Serial  // In log Blynk ra Serial Monitor

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Thông tin Wifi 
const char* WIFI_SSID = "your-wifi-ssid";   // Chỉ dùng băng tần 2.4GHz
const char* WIFI_PASS = "your-wifi-password";    

// Chân GPIO 
#define PIN_SENSOR  34
#define PIN_RELAY   26

// Ngưỡng độ ẩm 
#define ADC_KHO     3050
#define ADC_UOT     1390

// Ngưỡng mặc định theo % (có thể đổi từ Blynk qua V3/V4)
#define DEFAULT_NGUONG_BAT_PHAN_TRAM  40
#define DEFAULT_NGUONG_TAT_PHAN_TRAM  75

// Thời gian 
#define INTERVAL_DOC_SENSOR  2000  // Đọc cảm biến mỗi 2 giây
#define INTERVAL_GUI_BLYNK   2000  // Gửi dữ liệu lên Blynk mỗi 2 giây

// Khai báo các biến trạng thái và giá trị mặc định của nó
int  mode       = 1;     // 1=Auto, 0=Manual
bool bompDangBat = false; // Trạng thái bơm hiện tại
int  doAmPhanTram = 0;   // Độ ẩm % hiện tại

int nguongBatPhanTram = DEFAULT_NGUONG_BAT_PHAN_TRAM; // V3
int nguongTatPhanTram = DEFAULT_NGUONG_TAT_PHAN_TRAM; // V4
int nguongBatADC = 0;
int nguongTatADC = 0;

// Timer của Blynk 
BlynkTimer timer;

// 
//  HÀM PHỤ: Bật/Tắt bơm và cập nhật Blynk
// 

void batBom() {
    if (!bompDangBat) {
        digitalWrite(PIN_RELAY, HIGH);   // Active HIGH, bật relay
        bompDangBat = true;
        Blynk.virtualWrite(V0, 1);      // Cập nhật switch trên app
        Serial.println("[BOM] BAT");
    }
}

void tatBom() {
    if (bompDangBat) {
        digitalWrite(PIN_RELAY, LOW);  // Active LOW, tắt relay
        bompDangBat = false;
        Blynk.virtualWrite(V0, 0);      // Cập nhật switch trên app
        Serial.println("[BOM] TAT");
    }
}

int phanTramSangADC(int phanTram) {
    phanTram = constrain(phanTram, 0, 100);
    long span = ADC_KHO - ADC_UOT;
    long raw = ADC_UOT + (long)(100 - phanTram) * span / 100;
    return (int)constrain(raw, ADC_UOT, ADC_KHO);
}

void capNhatNguongADC() {
    nguongBatADC = phanTramSangADC(nguongBatPhanTram);
    nguongTatADC = phanTramSangADC(nguongTatPhanTram);
}

// 
//  HÀM CHÍNH: Đọc cảm biến + xử lý Auto
// 

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
        if (raw > nguongBatADC) {
            batBom();
        } else if (raw < nguongTatADC) {
            tatBom();
        }
    }
}

// 
//  BLYNK: Nhận lệnh từ thao tác trên Dashboard
// 

// Nhận lệnh điều khiển bơm thủ công
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
        // Bỏ qua lệnh điều khiển bơm nếu hệ thống đang ở mode Auto, đồng bộ lại trạng thái thật của bơm lên Dashboard 
        Serial.println("[AUTO] Bo qua lenh switch tu app");
        Blynk.virtualWrite(V0, bompDangBat ? 1 : 0);
    }
}

// Nhận lệnh từ Switch V2 (Mode)
BLYNK_WRITE(V2) {
    mode = param.asInt();
    Serial.print("[MODE] Chuyen sang: ");
    Serial.println(mode == 1 ? "AUTO" : "MANUAL");

    // Khi chuyển sang Manual thì tắt bơm để an toàn 
    if (mode == 0) {
        tatBom();
        Serial.println("[MODE] Tat bom de an toan khi chuyen Manual");
    }
}

// Hiệu chỉnh ngưỡng bật bơm (giá trị Blynk trả về là số phần trăm)
BLYNK_WRITE(V3) {
    nguongBatPhanTram = param.asInt();
    capNhatNguongADC();
    Serial.print("[NGUONG] Bat %: ");
    Serial.print(nguongBatPhanTram);
    Serial.print(" | ADC: ");
    Serial.println(nguongBatADC);
}

// Hiệu chỉnh ngưỡng tắt bơm (giá trị Blynk trả về là số phần trăm)
BLYNK_WRITE(V4) {
    nguongTatPhanTram = param.asInt();
    capNhatNguongADC();
    Serial.print("[NGUONG] Tat %: ");
    Serial.print(nguongTatPhanTram);
    Serial.print(" | ADC: ");
    Serial.println(nguongTatADC);
}

// Đồng bộ trạng thái khi ESP32 mới kết nối
BLYNK_CONNECTED() {
    // Lấy trạng thái hiện tại của các switch từ Blynk Cloud về ESP32 
    Blynk.syncVirtual(V0, V2, V3, V4);
    Serial.println("[BLYNK] Da ket noi, dong bo trang thai...");
}

// 
//  SETUP
// 

void setup() {
    Serial.begin(115200);

    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);  // Tắt bơm ngay khi khởi động

    capNhatNguongADC();

    Serial.println("=== He thong tuoi tu dong khoi dong ===");

    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

    // Đăng ký timer đọc cảm biến mỗi 2 giây
    timer.setInterval(INTERVAL_DOC_SENSOR, docCamBienVaXuLy);

    Serial.println("=== San sang ===");
}

// 
//  LOOP
// 

void loop() {
    Blynk.run();   // Giữ kết nối Blynk + xử lý lệnh từ app
    timer.run();   // Chạy các timer đã đăng ký
}