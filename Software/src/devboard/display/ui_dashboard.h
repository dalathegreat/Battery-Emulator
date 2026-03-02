#pragma once
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "../../inverter/InverterProtocol.h"
#include <WiFi.h>
#include <time.h>

// Load fonts
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "FreeSans6pt7b.h"

// Theme structure for display colors
struct UI_Theme {
    uint16_t bg;
    uint16_t text;
    uint16_t line;
    uint16_t alert;
    uint16_t bar;
    uint16_t normal;
};

// ======================================================================
// 📶 Helper Function: Draw Curved WiFi Signal Icon (Smart Device Style)
// ======================================================================
template <typename T>
void drawCurvedWiFiIcon(T* display, int x, int y, int rssi, uint16_t color) {
    // Determine the number of signal bars (max 4: Dot + 3 Arches)
    int bars = 0;
    if (rssi > -60) bars = 4;        // Excellent
    else if (rssi > -70) bars = 3;   // Good
    else if (rssi > -80) bars = 2;   // Fair
    else if (rssi > -90) bars = 1;   // Weak (Dot only)
    else if (rssi != 0) bars = 0;    // No signal

    int cx = x + 10; // Center X coordinate
    int cy = y - 2;  // Center Y (Base of the icon, slightly above text baseline)

    // Level 1: Draw the base dot
    if (bars >= 1) {
        display->fillRect(cx - 1, cy - 1, 3, 3, color); // Solid dot
    } else {
        display->drawRect(cx - 1, cy - 1, 3, 3, color); // Empty outline dot
    }

    // Level 2-4: Draw the curved arches using circle equation (x^2 + y^2 = r^2)
    for (int i = 1; i <= 3; i++) {
        if (bars >= i + 1) {
            int r = i * 4; // Radii for each arch: 4, 8, 12 pixels
            int x_max = r * 0.75; // Limit arch angle to approx 45 degrees
            
            for (int px = 0; px <= x_max; px++) {
                int py = sqrt(r * r - px * px); // Calculate Y coordinate
                
                // Draw symmetric points (left and right) with 2px thickness
                display->drawPixel(cx + px, cy - py, color);     // Right side
                display->drawPixel(cx - px, cy - py, color);     // Left side
                display->drawPixel(cx + px, cy - py + 1, color); // Right thickness
                display->drawPixel(cx - px, cy - py + 1, color); // Left thickness
            }
        }
    }
}

// ======================================================================
// ⚡ Helper Function: Draw Lightning Bolt Icon
// ======================================================================
template <typename T>
void drawLightning(T* display, int x, int y, uint16_t color) {
    // Draw upper triangle part
    display->fillTriangle(x + 5, y,      x, y + 10,     x + 6, y + 10, color);
    // Draw lower triangle part
    display->fillTriangle(x + 6, y + 9,  x + 9, y + 8,  x + 2, y + 20, color);
}

// ======================================================================
// 🖥️ Main Function: HMI 3-Tier Layout (Ultimate Edition)
// ======================================================================
template <typename T>
void drawSharedDashboard(T* display, bool is_lcd) {
    int dw = display->width();
    int dh = display->height();

    // ------------------------------------------------------------------
    // PART 1: DATA RETRIEVAL & PROCESSING (Business Logic)
    // ------------------------------------------------------------------

    // --- Font Selection (Responsive) ---
    const GFXfont* f_small = (dw >= 320) ? &FreeSans6pt7b : NULL;
    const GFXfont* f_normal = (dw >= 480) ? &FreeSansBold12pt7b : &FreeSansBold9pt7b;
    const GFXfont* f_large = (dw >= 480) ? &FreeSansBold24pt7b : &FreeSansBold18pt7b;

    // --- Battery Core Data ---
    int raw_soc = datalayer.battery.status.reported_soc;
    int display_soc = constrain(raw_soc, 0, 100); 

    int soh = datalayer.battery.status.soh_pptt / 100; 
    if (soh <= 0) soh = 100; // Default to 100% if no data is provided

    float min_cv_info = datalayer.battery.info.min_cell_voltage_mV / 1000.0f;
    float max_cv_info = datalayer.battery.info.max_cell_voltage_mV / 1000.0f;
    if (min_cv_info <= 0.0f) min_cv_info = 2.80f; // Default Undervoltage threshold
    if (max_cv_info <= 0.0f) max_cv_info = 3.65f; // Default Overvoltage threshold

    float voltage = datalayer.battery.status.voltage_dV / 10.0f;
    float current = datalayer.battery.status.current_dA / 10.0f;
    float power_kw = (voltage * current) / 1000.0;
    
    // Calculate Total Capacity (Convert Wh to kWh)
    float cap_kwh = datalayer.battery.info.reported_total_capacity_Wh / 1000.0f;
    if (cap_kwh <= 0) cap_kwh = 30.0; // Default 30kWh for Fake Battery or missing data

    // String processing for Inverter and Battery names
    String raw_inv = String(name_for_inverter_type(user_selected_inverter_protocol));
    String raw_bat = String(datalayer.system.info.battery_protocol);
    if (raw_inv.length() > 12) raw_inv = raw_inv.substring(0, 12) + ".";
    if (raw_bat.length() > 12) raw_bat = raw_bat.substring(0, 12) + ".";

    // --- System Status Evaluation ---
    String content_sys_stts = "";
    switch (datalayer.battery.status.bms_status) {
        case ACTIVE:   content_sys_stts = "OK"; break;
        case UPDATING: content_sys_stts = "UPDATING"; break;
        case FAULT:    content_sys_stts = "FAULT"; break;
        case INACTIVE: content_sys_stts = "INACTIVE"; break;
        case STANDBY:  content_sys_stts = "STANDBY"; break;
        default:       content_sys_stts = "UNKNOWN"; break;
    }

    // --- Battery Status Evaluation ---
    String bat_status = "";
    switch (datalayer.battery.status.real_bms_status) {
        case BMS_ACTIVE:       bat_status = "OK"; break;
        case BMS_FAULT:        bat_status = "FAULT"; break;
        case BMS_DISCONNECTED: bat_status = "DISCONNECTED"; break;
        case BMS_STANDBY:      bat_status = "STANDBY"; break;
        default:               bat_status = "UNKNOWN"; break;
    }

    // --- Cell Diagnostics & Min/Max Extraction ---
    int total_cells = datalayer.battery.info.number_of_cells;
    if (total_cells <= 0) total_cells = 16; // Default to 16 cells

    float max_cv = -1.0, min_cv = 99.0, sum_cv = 0;
    int max_cell_idx = 0, min_cell_idx = 0, active_cells = 0;
    String ov_cells = "";
    String uv_cells = "";
    int ov_count = 0;
    int uv_count = 0;

    // Scan through the cell voltage array
    for (int i = 0; i < total_cells; i++) {
        float cv = datalayer.battery.status.cell_voltages_mV[i] / 1000.0;
        if (cv > 0.5) { // Filter out invalid/zero data
            if (cv > max_cv) { max_cv = cv; max_cell_idx = i + 1; }
            if (cv < min_cv) { min_cv = cv; min_cell_idx = i + 1; }
            sum_cv += cv;
            active_cells++;
            
            // Detect Over Voltage
            if (cv > max_cv_info) {
                if (ov_count < 3) ov_cells += "#" + String(i + 1) + ",";
                ov_count++;
            }
            // Detect Under Voltage
            if (cv < min_cv_info) {
                if (uv_count < 3) uv_cells += "#" + String(i + 1) + ",";
                uv_count++;
            }
        }
    }
    
    // Fallback to summary variables if array has no data (e.g., Fake Battery)
    if (active_cells == 0) {
        min_cv = datalayer.battery.info.min_cell_voltage_mV / 1000.0;
        max_cv = datalayer.battery.info.max_cell_voltage_mV / 1000.0;
        if (min_cv == 0 && max_cv == 0) { min_cv = 3.50; max_cv = 3.50; } 
    }

    // Format OV/UV Strings for display
    if (ov_count > 0) {
        if (ov_cells.endsWith(",")) ov_cells = ov_cells.substring(0, ov_cells.length() - 1);
        if (ov_count > 3) ov_cells += "...";
    } else { ov_cells = "None"; }

    if (uv_count > 0) {
        if (uv_cells.endsWith(",")) uv_cells = uv_cells.substring(0, uv_cells.length() - 1);
        if (uv_count > 3) uv_cells += "...";
    } else { uv_cells = "None"; }

    float avg_cv = (active_cells > 0) ? (sum_cv / active_cells) : ((max_cv + min_cv) / 2.0);
    int delta_mv = (max_cv - min_cv) * 1000;

    // --- Alarm Status Evaluation ---
    bool has_alarm = false;
    String alarm_msg = "SYSTEM NORMAL";
    
    if (delta_mv > 150) { has_alarm = true; alarm_msg = "WARN: HIGH CELL DELTA (" + String(delta_mv) + "mV)"; }
    else if (max_cv > max_cv_info) { has_alarm = true; alarm_msg = "ALARM: CELL OVERVOLTAGE"; }
    else if (min_cv > 0.5 && min_cv < min_cv_info) { has_alarm = true; alarm_msg = "ALARM: CELL UNDERVOLTAGE"; }
    else if (voltage > (datalayer.battery.info.max_design_voltage_dV / 10.0f)) { has_alarm = true; alarm_msg = "ALARM: PACK OVERVOLTAGE"; }
    else if (display_soc <= 10) { has_alarm = true; alarm_msg = "WARN: LOW BATTERY SOC"; }
    else if (content_sys_stts == "FAULT" || bat_status == "FAULT") { has_alarm = true; alarm_msg = "FAULT: BMS HAS ERROR!"; }

    // ------------------------------------------------------------------
    // PART 2: UI RENDERING (Presentation)
    // ------------------------------------------------------------------

    // --- Theme Color Configuration ---
    UI_Theme theme;
    if (is_lcd) {
        theme.bg = 0x0000; theme.text = 0xFFFF; theme.line = 0x7BEF; 
        theme.alert = 0xF800; theme.bar = 0x07E0; theme.normal = 0x07E0;
    } else {
        theme.bg = 0xFFFF; theme.text = 0x0000; theme.line = 0x0000; 
        theme.alert = 0xF800; theme.bar = 0x0000; theme.normal = 0x0000;
    }

    display->fillScreen(theme.bg);
    display->setTextColor(theme.text);

    // ==========================================
    // TIER 1: TOP OVERVIEW (Height 45%)
    // ==========================================
    int t1_h = dh * 0.45;

    // --- Left Column: Power and System Stats ---
    display->setFont(f_normal);
    uint16_t bolt_color = is_lcd ? theme.bar : theme.text; 
    drawLightning(display, 10, 12, bolt_color);
    
    display->setCursor(28, 30); display->printf("%.1f kW", power_kw);
    display->setFont(f_small);
    display->setCursor(20, 55); display->printf(" >> %.1f V", voltage);
    display->setCursor(20, 75); display->printf(" >> %.1f A", current);

    if (content_sys_stts == "FAULT") display->setTextColor(theme.alert);
    display->setCursor(10, 95); display->print("System: " + content_sys_stts);
    display->setTextColor(theme.text);
    
    if (bat_status == "FAULT") display->setTextColor(theme.alert);
    display->setCursor(10, 115); display->print("Battery: " + bat_status);
    display->setTextColor(theme.text);

    // --- Middle Column: SOC & Battery Health ---
    int bat_w = dw * 0.25; 
    int bat_h = t1_h * 0.45; 
    int bat_x = (dw / 2) - (bat_w / 2); 
    int bat_y = 20;

    // Draw Battery Frame
    display->drawRect(bat_x, bat_y, bat_w, bat_h, theme.text);
    display->fillRect(bat_x + bat_w, bat_y + (bat_h * 0.25), 5, bat_h * 0.5, theme.text); // Battery terminal
    
    // Draw SOC Fill
    uint16_t bat_color = (display_soc < 20) ? theme.alert : theme.bar;
    int fill_w = map(display_soc, 0, 100, 0, bat_w - 4);
    display->fillRect(bat_x + 2, bat_y + bat_h - 7, fill_w, 5, bat_color);
    
    // Draw Chemistry Text (Vertical on the left side of the battery)
    String chem_str = String(datalayer.battery.info.chemistry);
    if (chem_str == "LiFePO4" || chem_str == "1") chem_str = "LFP";
    else if (chem_str == "Li-ion" || chem_str == "2") chem_str = "NMC";
    else if (chem_str.length() > 5) chem_str = chem_str.substring(0, 4);

    display->setFont(f_small); 
    display->setTextSize(1);
    display->setTextColor(theme.text);

    int chem_x = bat_x + bat_w - (7 * (chem_str.length() + 1)); 
    int chem_y = bat_y + (4 * 3); 
    display->setCursor(chem_x, chem_y); 
    display->print(chem_str);

    // Draw SOC Percentage
    display->setFont(f_large);
    int txt_x = (display_soc < 10) ? bat_x + (bat_w * 0.35) : (display_soc < 100) ? bat_x + (bat_w * 0.2) : bat_x + (bat_w * 0.05);
    display->setCursor(txt_x, bat_y + (bat_h * 0.7));
    display->print(display_soc); 
    display->setFont(f_normal); display->print("%");

    // Draw SOH and Capacity
    display->setFont(f_normal);
    display->setCursor(bat_x + 5, bat_y + bat_h + 20);
    display->printf("SOH: %d %%", soh);
    
    display->setCursor(bat_x + 5, bat_y + bat_h + 40);
    display->printf("Cap: %.1f kWh", cap_kwh);
    
    // --- Network IP Address & RSSI ---
    String ipStr = "No WiFi";
    int rssi = -200;
    if (WiFi.status() == WL_CONNECTED) {
        ipStr = WiFi.localIP().toString(); 
        rssi = WiFi.RSSI();
    } else if ((WiFi.getMode() & WIFI_AP) != 0) {
        ipStr = WiFi.softAPIP().toString();
        rssi = WiFi.RSSI();
    }

    // --- Right Column: Network and Device Info ---
    struct tm timeinfo;
    int right_x = dw * 0.70;

    display->setFont(f_small);
    display->setTextColor(theme.text);
    
    drawCurvedWiFiIcon(display, right_x, 20, rssi, theme.text);
    display->setCursor(right_x + 18, 20); display->print(": " + ipStr);
    display->setCursor(right_x, 40); display->print("INV: " + raw_inv);
    display->setCursor(right_x, 60); display->print("BAT: " + raw_bat);
    
    if (getLocalTime(&timeinfo)) { 
        display->setCursor(right_x,  80); display->print("Updated");
        display->setCursor(right_x + 10, 100); display->printf("Date: %02d-%02d-%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
        display->setCursor(right_x + 10, 120); display->printf("Time: %02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    }

    // ==========================================
    // TIER 2: MIDDLE ALARM BANNER (Height 15%)
    // ==========================================
    int t2_y = t1_h;
    int t2_h = dh * 0.15;
    
    uint16_t banner_bg;
    uint16_t banner_fg;
    
    if (has_alarm) {
        // Alarm State: Red background, White text (Blinking for LCD)
        if (is_lcd && (millis() / 1000) % 2 == 0) {
            banner_bg = theme.alert; banner_fg = theme.bg; 
        } else {
            banner_bg = theme.alert; banner_fg = theme.bg; 
        }
    } else {
        // Normal State
        if (is_lcd) {
            banner_bg = 0x03E0;     // Dark Green background for LCD
            banner_fg = 0xFFFF;     // White text
        } else {
            banner_bg = theme.bg;   // White background for E-Paper
            banner_fg = theme.text; // Black text
        }
    }

    // Draw banner background and borders
    display->fillRect(0, t2_y, dw, t2_h, banner_bg);
    if (!is_lcd) {
        display->drawLine(0, t2_y, dw, t2_y, theme.line);
        display->drawLine(0, t2_y + t2_h, dw, t2_y + t2_h, theme.line);
    }

    // Print Alarm/Normal message
    display->setFont(f_normal);
    display->setTextColor(banner_fg);
    int msg_x = (dw / 2) - (alarm_msg.length() * 6); 
    if (msg_x < 0) msg_x = 5;
    display->setCursor(msg_x, t2_y + (t2_h * 0.65));
    display->print(alarm_msg);

    // ==========================================
    // TIER 3: BOTTOM DIAGNOSTICS (Ultra-Wide Graph Edition)
    // ==========================================
    int t3_y = t2_y + t2_h;
    int t3_h = dh - t3_y;
    display->setTextColor(theme.text);
    display->setFont(f_small);

    // --- 1. Top Text Info (Horizontal Layout) ---
    int text_y = t3_y + 20;
    
    // Format Max and Min strings with cell numbers
    String max_str = (active_cells > 0) ? String(max_cv, 3) + "V [#" + String(max_cell_idx) + "]" : String(max_cv, 3) + "V";
    String min_str = (active_cells > 0) ? String(min_cv, 3) + "V [#" + String(min_cell_idx) + "]" : String(min_cv, 3) + "V";
    
    // Row 1: Max, Min, Delta printed inline
    display->setCursor(10, text_y);
    display->print("Max: " + max_str + "   Min: " + min_str + "   ");
    
    if (delta_mv > 100) display->setTextColor(theme.alert);
    display->print("Delta: " + String(delta_mv) + "mV");
    display->setTextColor(theme.text);

    // Row 2: OVC and UVC printed inline
    int text_y2 = text_y + 18; 
    display->setCursor(10, text_y2);
    display->print("OVC: ");
    if (ov_count > 0) display->setTextColor(theme.alert);
    display->print(ov_cells + "    "); 
    display->setTextColor(theme.text);
    
    display->print("UVC: ");
    if (uv_count > 0) display->setTextColor(theme.alert);
    display->print(uv_cells);
    display->setTextColor(theme.text);

    // --- 2. Ultra-Wide Bar Graph ---
    int gx = 35;                     // X-coordinate
    int gy = text_y2 + 10;           // Y-coordinate
    int gw = dw - gx - 10;           // Graph width
    int gh = (t3_y + t3_h) - gy - 15; // Graph height 
    
    // Set graph scale limits
    float graph_min_v = min_cv_info; 
    float graph_max_v = max_cv_info;

    // Draw graph boundary box
    display->drawRect(gx, gy, gw, gh, theme.line);

    if (active_cells > 0) {
        // Y-axis scale values
        display->setFont(NULL); 
        display->setTextSize(1);
        display->setTextColor(theme.text);
        display->setCursor(gx - 28, gy); display->printf("%.1f", graph_max_v); 
        display->setCursor(gx - 28, gy + gh - 7); display->printf("%.1f", graph_min_v); 

        // Draw dashed line for "Average Voltage"
        int avg_y = gy + gh - map(avg_cv * 1000, graph_min_v * 1000, graph_max_v * 1000, 0, gh);
        if (avg_y > gy && avg_y < gy + gh) {
            for(int px = gx + 2; px < gx + gw - 2; px += 4) {
                display->drawPixel(px, avg_y, theme.text); 
            }
        }

        // Dynamic bar width calculation
        int bar_w = gw / active_cells;
        if(bar_w > 12) bar_w = 12; // Maximum width for a single bar
        int act_bar_w = (bar_w > 1) ? bar_w - 1 : 1; 

        for (int i = 0; i < active_cells; i++) {
            float cv = datalayer.battery.status.cell_voltages_mV[i] / 1000.0;
            if (cv < 0.5) continue;
            
            // Calculate bar height
            int bar_h_px = map(cv * 1000, graph_min_v * 1000, graph_max_v * 1000, 0, gh); 
            if (bar_h_px > gh) bar_h_px = gh;
            if (bar_h_px < 1) bar_h_px = 1;

            int bx = gx + 1 + (i * bar_w);
            int by = gy + gh - bar_h_px;

            // Apply alert color for abnormal cells
            uint16_t c_color = theme.text;
            if (cv > max_cv_info || cv < min_cv_info || abs(cv - avg_cv) > 0.150) {
                c_color = theme.alert;
            }

            display->fillRect(bx, by, act_bar_w, bar_h_px, c_color);
        }

        // --- X-Axis Labels (Cell Numbers) ---
        display->setFont(NULL); 
        display->setTextSize(1);
        display->setTextColor(theme.text);
        
        // Left side label
        display->setCursor(gx, gy + gh + 4); 
        display->print("1");
        
        // Right side label
        String x_right_str = String(active_cells) + " cells";
        int str_w = x_right_str.length() * 6; 
        display->setCursor(gx + gw - str_w, gy + gh + 4); 
        display->print(x_right_str);
        
    } else {
        // Display fallback message for Fake Battery / Summary Mode
        display->setFont(f_small);
        display->setCursor(gx + (gw/2) - 60, gy + (gh/2));
        display->print("Summary Mode (No Array)");
    }
}