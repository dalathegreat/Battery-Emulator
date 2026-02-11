#include <Arduino.h>
#include "epaper_task.h"
#include <GxEPD2_3C.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <esp_task_wdt.h>
#include <WiFi.h>

// เรียกใช้ Datalayer
#include "../../datalayer/datalayer.h"
extern DataLayer datalayer;

// --- Pin Definition (สำหรับ LilyGo T-2CAN) ---
#ifdef HW_LILYGO2CAN
  #define EPD_BUSY    35 
  #define EPD_SCK     16
  #define EPD_MOSI    15
  #define EPD_DC      45
  #define EPD_RST     47
  #define EPD_CS      46  
#else
  // for Other board
  #define EPD_BUSY    -1
  #define EPD_SCK     -1
  #define EPD_MOSI    -1
  #define EPD_DC      -1
  #define EPD_RST     -1
  #define EPD_CS      -1
#endif

// ประกาศ Object จอ 4.2" 3-Color (BWR)
GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT> display(GxEPD2_420c(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// ตัวแปรสำหรับ Smart Update
unsigned long last_update_time = 0;
float last_voltage = -99.9;
int last_soc = -1;

void setupEpaper() {
    // บังคับ SPI ให้มาออกขาชุดนี้
    SPI.end(); 
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS); // SCK, MISO(-1), MOSI, CS
    
    display.init(0); // ปิด Debug Serial เพื่อไม่ให้กวนระบบหลัก
    display.setRotation(1); // แนวนอน (Landscape)
    
    // เคลียร์หน้าจอครั้งแรก
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeSansBold18pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(40, 150);
        display.print("Battery Emulator");
        display.setTextColor(GxEPD_RED);
        display.setCursor(40, 190);
        display.print("Dashboard Loading...");
    } while (display.nextPage());
    
    last_update_time = millis();
}

// --- ฟังก์ชันวาดกราฟ Cell Map (แก้ไขให้ใช้ค่าจริง) ---
void drawCellMap(int x_start, int y_start, int width, int height, 
                 float limit_min_v, float limit_max_v, float warn_high_v, float warn_low_v, int total_cells) {

    // 1. คำนวณจำนวนแถวอัตโนมัติ
    int rows = 1;
    if (total_cells > 96) rows = 2;  // ถ้าเกิน 96 เซลล์ ให้แบ่ง 2 แถว
    if (total_cells > 200) rows = 3; // เผื่ออนาคต

    int cells_per_row = (total_cells + rows - 1) / rows; 
    int row_height = (height / rows); 
    int bar_chart_height = row_height - 18; // พื้นที่กราฟจริง

    // คำนวณความกว้างแท่งกราฟ
    int bar_w = width / cells_per_row;
    if (bar_w < 3) bar_w = 3; 

    // วนลูปวาดทีละแถว
    for (int r = 0; r < rows; r++) {
        int current_y_base = y_start + ((r + 1) * row_height) - 15;
        int start_index = r * cells_per_row;
        int end_index = start_index + cells_per_row;
        if (end_index > total_cells) end_index = total_cells;

        // วาดเส้นประกลาง (Nominal Voltage)
        float mid_v = (limit_max_v + limit_min_v) / 2.0;
        int mid_y_offset = map((long)(mid_v*1000), (long)(limit_min_v*1000), (long)(limit_max_v*1000), 0, bar_chart_height);
        int mid_y_abs = current_y_base - mid_y_offset;
        
        for (int k = x_start; k < x_start + width; k += 4) {
            display.drawPixel(k, mid_y_abs, GxEPD_BLACK);
        }

        // --- วาดแท่งกราฟแต่ละเซลล์ ---
        for (int i = start_index; i < end_index; i++) {
            
            // [UPDATED] ดึงค่าจริงจาก Datalayer แล้ว!
            // cell_voltages_mV เก็บค่าเป็น mV ต้องหาร 1000 ให้เป็น V
            float cell_v = datalayer.battery.status.cell_voltages_mV[i] / 1000.0;
            
            // ถ้าค่าเป็น 0 (อ่านไม่ได้) ให้ดันไปที่ Min เพื่อไม่ให้กราฟดูน่ากลัวเกินไป หรือปล่อย 0 ก็ได้
            if (cell_v < 0.1) cell_v = limit_min_v; 

            // คำนวณความสูงแท่งกราฟ
            int bar_h = map((long)(cell_v * 1000), (long)(limit_min_v * 1000), (long)(limit_max_v * 1000), 0, bar_chart_height);
            bar_h = constrain(bar_h, 0, bar_chart_height);

            // ตรวจสอบสี (แดงถ้าผิดปกติ)
            uint16_t color = GxEPD_BLACK;
            if (cell_v >= warn_high_v || cell_v <= warn_low_v) color = GxEPD_RED;

            // คำนวณพิกัด X
            int relative_i = i - start_index;
            int bar_x = x_start + (relative_i * bar_w);
            int bar_y = current_y_base - bar_h;

            // วาดแท่ง
            display.fillRect(bar_x, bar_y, bar_w - 1, bar_h, color);
        }

        // แสดงเลขกำกับแถว
        display.setFont(&FreeSansBold9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(x_start - 25, current_y_base - (bar_chart_height/2) + 5);
        display.print(start_index + 1);
    }
}

void updateEpaperDisplay(float voltage, float current, int soc, String status) {
    
    unsigned long currentMillis = millis();

    // Smart Update Logic
    if (currentMillis - last_update_time < 60000 && last_update_time != 0) {
        esp_task_wdt_reset(); 
        return; 
    }
    bool forceUpdate = (currentMillis - last_update_time > 600000);
    if (!forceUpdate && abs(voltage - last_voltage) < 0.2 && soc == last_soc) {
        esp_task_wdt_reset();
        return;
    }

    // --- [UPDATED] เตรียมข้อมูล Limits (แก้ชื่อตัวแปรให้ตรงกับ datalayer.h) ---
    
    // 1. ดึง Min/Max Design Voltage จาก battery.info
    float sys_min_v = datalayer.battery.info.min_cell_voltage_mV / 1000.0;
    float sys_max_v = datalayer.battery.info.max_cell_voltage_mV / 1000.0;
    
    // กันค่าเป็น 0 กรณีเริ่มต้น
    if (sys_min_v < 1.0) sys_min_v = 2.5;
    if (sys_max_v < 1.0) sys_max_v = 3.65;

    float warn_high = sys_max_v - 0.05; 
    float warn_low  = sys_min_v + 0.05;

    // 2. ดึงจำนวนเซลล์ที่ถูกต้อง (แก้จาก total_cell_count เป็น number_of_cells)
    int total_cells = datalayer.battery.info.number_of_cells;
    if (total_cells == 0) total_cells = 128; // Default

    // เริ่มวาด
    display.setFullWindow();
    display.firstPage();
    do {
        // told Watchdog still working
        esp_task_wdt_reset(); 
        display.fillScreen(GxEPD_WHITE);

        // ================= HEADER (แก้ไขใหม่) =================
        display.fillRect(0, 0, 400, 35, GxEPD_BLACK);
        display.setTextColor(GxEPD_WHITE);
        display.setFont(&FreeSansBold9pt7b);
        
        // มุมซ้าย: ชื่อโปรเจกต์
        display.setCursor(10, 25);
        display.print("BATTERY EMULATOR"); 
        
        // มุมขวา: IP Address 
        String ipStr;
        if (WiFi.status() == WL_CONNECTED) {
            ipStr = WiFi.localIP().toString(); // ดึง IP ถ้าต่อเน็ตติด
        } else {
            // เช็คว่าปล่อย Hotspot (AP Mode) อยู่ไหม
            if ((WiFi.getMode() & WIFI_AP) != 0) {
                 ipStr = WiFi.softAPIP().toString(); // ดึง IP ของ Hotspot (ปกติ 192.168.4.1)
            } else {
                 ipStr = "No WiFi";
            }
        }
        
        // จัดตำแหน่งชิดขวา (ขยับแกน X ตามความยาวข้อความ)
        // IP ยาวสุดประมาณ 15 ตัวอักษร ใช้พื้นที่ ~140px
        display.setCursor(250, 25); 
        display.print(ipStr);

        // ================= END HEADER =================

        // LEFT PANEL (Overview)
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&FreeSansBold24pt7b);
        display.setCursor(10, 100);
        display.print(soc);
        display.print("%");

        // Battery Icon
        display.drawRect(20, 115, 90, 35, GxEPD_BLACK);
        display.fillRect(110, 122, 6, 20, GxEPD_BLACK);
        uint16_t bat_color = (soc < 20) ? GxEPD_RED : GxEPD_BLACK;
        int fill_w = map(soc, 0, 100, 0, 86);
        display.fillRect(22, 117, fill_w, 31, bat_color);

        // Power (kW)
        float power_kw = (voltage * current) / 1000.0;
        display.setFont(&FreeSansBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(10, 185);
        display.print(power_kw, 1);
        display.print(" kW");

        // RIGHT PANEL (Cell Map)
        // วาดกราฟด้วยข้อมูลจริง
        drawCellMap(140, 45, 255, 160, sys_min_v, sys_max_v, warn_high, warn_low, total_cells);

        // FOOTER
        display.drawLine(0, 215, 400, 215, GxEPD_BLACK);

        // Total Voltage & Current
        display.setFont(&FreeSansBold12pt7b);
        display.setCursor(10, 245);
        display.print("Total: "); display.print(voltage, 1); display.print("V");
        
        display.setCursor(210, 245);
        if(current < 0) display.setTextColor(GxEPD_RED);
        else display.setTextColor(GxEPD_BLACK);
        display.print("Curr: "); display.print(current, 1); display.print("A");

        // Min/Max Cell Details
        float min_cell_v = datalayer.battery.status.cell_min_voltage_mV / 1000.0;
        float max_cell_v = datalayer.battery.status.cell_max_voltage_mV / 1000.0;
        
        if(min_cell_v < 1) min_cell_v = sys_min_v;
        if(max_cell_v < 1) max_cell_v = sys_max_v;
        
        int delta_mv = (int)((max_cell_v - min_cell_v) * 1000);

        display.setFont(&FreeSansBold9pt7b);
        display.setTextColor(GxEPD_BLACK);
        
        display.setCursor(10, 285);
        display.print("Min: "); display.print(min_cell_v, 3);
        
        display.setCursor(140, 285);
        display.print("Max: "); display.print(max_cell_v, 3);

        display.setCursor(270, 285);
        if (delta_mv > 30) display.setTextColor(GxEPD_RED);
        display.print("D: "); display.print(delta_mv); display.print("mV");

        esp_task_wdt_reset();

    } while (display.nextPage());

    last_voltage = voltage;
    last_soc = soc;
    last_update_time = currentMillis;
}
