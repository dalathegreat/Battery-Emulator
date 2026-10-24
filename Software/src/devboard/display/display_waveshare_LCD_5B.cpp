// UI-200: SOC window editor and enable-only rescaling control.
// Instrument-panel logic: solid opaque surfaces, no decorative animations.
// Based on the supplied board routing/timings; LVGL integration targets 9.3.0.
// Source reviewed, not compiled or hardware-tested.
#if defined(HW_WAVESHARE_LCD_5B) && !defined(SMALL_FLASH_DEVICE)
#include "display.h"
#include "waveshare_ui_text.h"
#include "../hal/hal.h"
#include "../safety/safety.h"
#include "../../datalayer/datalayer.h"
#include <Arduino.h>
#include <WiFi.h>
#include "../../communication/pump/pump_link.h"
#include "../../communication/nvm/comm_nvm.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lvgl.h>
#include "driver/i2c_master.h"
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"

#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR != 3 || LVGL_VERSION_PATCH != 0
#error This driver targets LVGL 9.3.0
#endif
#if !LV_USE_SCALE
#error UI191 requires LV_USE_SCALE (enabled by default in LVGL 9.3.0)
#endif

namespace {
constexpr int LCD_WIDTH = 1024;
constexpr int LCD_HEIGHT = 600;
constexpr uint32_t LCD_PCLK_HZ = 14000000;
constexpr int LCD_HSYNC_FRONT = 40, LCD_HSYNC_PULSE = 20, LCD_HSYNC_BACK = 40;
constexpr int LCD_VSYNC_FRONT = 40, LCD_VSYNC_PULSE = 20, LCD_VSYNC_BACK = 40;
class WaveshareLCD5BLGFX : public lgfx::LGFX_Device {

 public:
  lgfx::Bus_RGB bus;
  lgfx::Panel_RGB panel;

  WaveshareLCD5BLGFX() {

    {
      auto cfg = panel.config();

      cfg.memory_width  = LCD_WIDTH;
      cfg.memory_height = LCD_HEIGHT;
      cfg.panel_width   = LCD_WIDTH;
      cfg.panel_height  = LCD_HEIGHT;
      cfg.offset_x = 0;
      cfg.offset_y = 0;

      panel.config(cfg);
    }

    {
      auto cfg = panel.config_detail();

      // Same strategy as the reference project:
      // framebuffer managed by LovyanGFX in PSRAM.
      cfg.use_psram = 1;

      panel.config_detail(cfg);
    }

    {
      auto cfg = bus.config();

      cfg.panel = &panel;

      // RGB565 data pins: exact Waveshare 28151 routing.
      cfg.pin_d0  = 14;  // B0
      cfg.pin_d1  = 38;  // B1
      cfg.pin_d2  = 18;  // B2
      cfg.pin_d3  = 17;  // B3
      cfg.pin_d4  = 10;  // B4

      cfg.pin_d5  = 39;  // G0
      cfg.pin_d6  = 0;   // G1
      cfg.pin_d7  = 45;  // G2
      cfg.pin_d8  = 48;  // G3
      cfg.pin_d9  = 47;  // G4
      cfg.pin_d10 = 21;  // G5

      cfg.pin_d11 = 1;   // R0
      cfg.pin_d12 = 2;   // R1
      cfg.pin_d13 = 42;  // R2
      cfg.pin_d14 = 41;  // R3
      cfg.pin_d15 = 40;  // R4

      cfg.pin_henable = 5;
      cfg.pin_vsync   = 3;
      cfg.pin_hsync   = 46;
      cfg.pin_pclk    = 7;

      // Exact known-good timing from VaAndCob's 1024x600 example.
      cfg.freq_write = LCD_PCLK_HZ;

      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = LCD_HSYNC_FRONT;
      cfg.hsync_pulse_width = LCD_HSYNC_PULSE;
      cfg.hsync_back_porch  = LCD_HSYNC_BACK;

      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = LCD_VSYNC_FRONT;
      cfg.vsync_pulse_width = LCD_VSYNC_PULSE;
      cfg.vsync_back_porch  = LCD_VSYNC_BACK;

      cfg.pclk_active_neg = 0;
      cfg.de_idle_high    = 0;
      cfg.pclk_idle_high  = 0;

      bus.config(cfg);
    }

    panel.setBus(&bus);
    setPanel(&panel);
  }
};

static WaveshareLCD5BLGFX lcd;



i2c_master_bus_handle_t bus_handle = nullptr;
i2c_master_dev_handle_t expander_mode = nullptr;
i2c_master_dev_handle_t expander_io = nullptr;
lv_display_t* display = nullptr;
void* draw_buffer = nullptr;
bool ready = false;
bool attempted = false;

// Same LVGL owner, same buffer, no extra task. Diagnostics never wait for USB.
#ifndef LCD5B_UI_DIAGNOSTICS
#define LCD5B_UI_DIAGNOSTICS 1
#endif
struct UiMetrics {
  uint32_t gap = 0, input = 0, data = 0, draw = 0, copy = 0, action = 0;
  uint32_t taps = 0, debounce = 0, errors = 0;
};
UiMetrics ui_metrics;
uint32_t frame_copy_us = 0;
bool render_requested = false;
bool pump_refresh_requested = false;
bool action_pending = false;
uint32_t action_started_us = 0;

void record_max(uint32_t& maximum, uint32_t value) {
  if (value > maximum) maximum = value;
}

void report_ui(uint32_t now) {
#if LCD5B_UI_DIAGNOSTICS
  static uint32_t last_report = 0;
  static UiMetrics snapshot;
  static unsigned line = 3;
  if (line == 3 && uint32_t(now - last_report) >= 5000) {
    last_report = now;
    snapshot = ui_metrics;
    ui_metrics = UiMetrics{};
    line = 0;
  }
  if (line == 3 || !Serial) return;
  // Each complete line fits a 64-byte USB packet even with 10-digit counters.
  // If the TX buffer is busy, retry on a later idle iteration, never flush().
  char b[64];
  int n = 0;
  if (line == 0)
    n = snprintf(b, sizeof(b), "UI191 us gap=%lu input=%lu data=%lu\n",
                 (unsigned long)snapshot.gap, (unsigned long)snapshot.input, (unsigned long)snapshot.data);
  else if (line == 1)
    n = snprintf(b, sizeof(b), "UI191 us draw=%lu copy=%lu action=%lu\n",
                 (unsigned long)snapshot.draw, (unsigned long)snapshot.copy, (unsigned long)snapshot.action);
  else
    n = snprintf(b, sizeof(b), "UI191 n taps=%lu debounce=%lu errors=%lu\n",
                 (unsigned long)snapshot.taps, (unsigned long)snapshot.debounce, (unsigned long)snapshot.errors);
  if (n > 0 && size_t(n) < sizeof(b) && Serial.availableForWrite() >= n) {
    Serial.write(reinterpret_cast<const uint8_t*>(b), size_t(n));
    ++line;
  }
#endif
}

bool checked(esp_err_t error, const char* stage) {
  if (error == ESP_OK) return true;
  Serial.printf("LCD5B: %s failed: %s\n", stage, esp_err_to_name(error));
  return false;
}

bool add_device(uint8_t address, i2c_master_dev_handle_t* device) {
  i2c_device_config_t cfg = {};
  cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  cfg.device_address = address;
  cfg.scl_speed_hz = 400000;
  return checked(i2c_master_bus_add_device(bus_handle, &cfg, device), "I2C device");
}

bool write_byte(i2c_master_dev_handle_t device, uint8_t value) {
  return checked(i2c_master_transmit(device, &value, 1, 100), "CH422G write");
}

uint32_t tick_ms() { return millis(); }

void flush(lv_display_t* disp, const lv_area_t* area, uint8_t* pixels) {
  // LVGL v9 supplies a byte map, not an array of v8 lv_color_t.
  uint32_t started = micros();
  lcd.pushImage(area->x1, area->y1,
                area->x2 - area->x1 + 1, area->y2 - area->y1 + 1,
                reinterpret_cast<lgfx::rgb565_t*>(pixels));
  // Synchronous copy: buffer may only be reused after pushImage returns.
  frame_copy_us += uint32_t(micros() - started);
  lv_display_flush_ready(disp);
}
// UI-01: base dashboard + cell monitor. No writes to battery settings.
// Colours follow a test-instrument panel: navy, white and amber controls.
constexpr uint32_t UI_BG = 0x101E29, UI_PANEL = 0x1B3040;
constexpr uint32_t UI_TEXT = 0xEDF3F5, UI_MUTED = 0xAAC0CC;
constexpr uint32_t UI_ACCENT = 0xFFB34D;
lv_obj_t *page_main, *page_cells, *navigation, *status_label, *footer_label;
lv_obj_t *soc_label, *soh_label, *temp_label, *voltage_label, *current_label, *power_label;
lv_obj_t *charge_label, *discharge_label, *charge_effective, *discharge_effective;
lv_obj_t *pump_track, *pump_knob, *pump_label;
lv_obj_t *cell_charts[2], *cell_stats, *cell_scale, *cell_count_label;
lv_obj_t* wifi_label;
lv_obj_t* wifi_icon;
lv_obj_t* wifi_bars[4];
lv_chart_series_t* cell_series[2];
constexpr unsigned PACK_CELLS = 98, CELLS_PER_ROW = 49;
static_assert(MAX_AMOUNT_CELLS >= PACK_CELLS, "Cell array too small");
int32_t plotted_cells[2][CELLS_PER_ROW];
bool refresh_requested = false;
void refresh_data();
bool cells_visible = false;
lv_obj_t *pump_window=nullptr, *pump_value=nullptr, *pump_info=nullptr, *pump_card_status=nullptr;
lv_obj_t *pump_amps=nullptr, *pump_mode=nullptr;
bool pump_window_open=false;
lv_obj_t *soc_window=nullptr,*soc_min_value=nullptr,*soc_max_value=nullptr;
lv_obj_t *soc_info=nullptr,*scale_button=nullptr,*scale_label=nullptr;
bool soc_window_open=false;
int soc_edit_min=1000,soc_edit_max=10000;
int soc_original_min=0,soc_original_max=0;
bool soc_original_scaling=false;
unsigned soc_pending=0; // 1: save window, 2: enable scaling; serviced after painting.
void refresh_soc();
lv_obj_t* pump_buttons[3] = {};  // manual zero, up, down
lv_obj_t* pump_button_ink[3] = {};
int pump_pulse = -1;
uint32_t pump_pulse_at = 0;
const char* pump_notice = nullptr;
uint32_t pump_notice_at = 0;
void refresh_pump();
bool touch_ready = false;
i2c_master_dev_handle_t touch_device = nullptr;
uint16_t touch_width = LCD_WIDTH, touch_height = LCD_HEIGHT;

void text(lv_obj_t* obj, const char* value) {
  if (strcmp(lv_label_get_text(obj), value) != 0) lv_label_set_text(obj, value);
}

lv_obj_t* panel(lv_obj_t* parent, int x, int y, int w, int h, uint32_t colour) {
  lv_obj_t* obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_set_pos(obj, x, y);
  lv_obj_set_size(obj, w, h);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(obj, lv_color_hex(colour), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(obj, 12, 0);
  return obj;
}

lv_obj_t* label(lv_obj_t* parent, int x, int y, int w, const char* value,
                const lv_font_t* font, uint32_t colour = UI_TEXT) {
  lv_obj_t* obj = lv_label_create(parent);
  lv_obj_set_pos(obj, x, y);
  lv_obj_set_width(obj, w);
  lv_obj_set_style_text_font(obj, font, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(colour), 0);
  lv_label_set_text(obj, value);
  return obj;
}

lv_obj_t* metric(int x, int y, int w, int h, const char* title) {
  lv_obj_t* card = panel(page_main, x, y, w, h, UI_PANEL);
  label(card, 18, 12, w - 36, title, &lv_font_montserrat_20, UI_MUTED);
  return label(card, 18, 49, w - 36, "--", &lv_font_montserrat_40);
}

lv_obj_t* centred_label(lv_obj_t* parent, int width, const char* value,
                       const lv_font_t* font, uint32_t colour=UI_TEXT) {
  auto* obj=label(parent,0,0,width,value,font,colour);
  lv_obj_set_style_text_align(obj,LV_TEXT_ALIGN_CENTER,0);
  lv_obj_center(obj);
  return obj;
}

bool valid_soc_window(int lo,int hi) {
  return lo>=-1000 && lo<=5000 && hi>=5000 && hi<=10000 && lo<hi;
}

int soc_step_value(int value,bool minimum,bool increase) {
  int lo=minimum?-1000:5000,hi=minimum?5000:10000;
  int next=value+(increase?1000:-1000); // 10 percentage points; retain web decimals.
  if(next<lo)next=lo;
  if(next>hi)next=hi;
  return next;
}

void refresh_soc() {
  auto& settings=datalayer.battery_settings;
  text(scale_label,settings.soc_scaling_active?ui_text::scale_on:ui_text::scale_off);
  static int previous=-1;
  int active=settings.soc_scaling_active?1:0;
  if(previous!=active){
    lv_obj_set_style_bg_color(scale_button,lv_color_hex(active?UI_ACCENT:0x3A4B58),0);
    lv_obj_set_style_text_color(scale_label,lv_color_hex(active?UI_BG:UI_TEXT),0);
    previous=active;
  }
  if(soc_window_open){
    char b[24];
    snprintf(b,sizeof(b),"%.1f %%",soc_edit_min/100.0);text(soc_min_value,b);
    snprintf(b,sizeof(b),"%.1f %%",soc_edit_max/100.0);text(soc_max_value,b);
  }
}

void open_soc() {
  auto& s=datalayer.battery_settings;
  soc_original_min=s.min_percentage;soc_original_max=s.max_percentage;
  soc_original_scaling=s.soc_scaling_active;
  // Opening never overwrites stored settings. Explicit preset loads 10/100.
  soc_edit_min=soc_original_min;soc_edit_max=soc_original_max;
  soc_window_open=true;
  text(soc_info,ui_text::soc_edit);
  refresh_soc();
  lv_obj_remove_flag(soc_window,LV_OBJ_FLAG_HIDDEN);
}

void soc_action(int target) {
  if(soc_pending)return;
  if(target==15){soc_window_open=false;lv_obj_add_flag(soc_window,LV_OBJ_FLAG_HIDDEN);return;}
  if(target==21){soc_edit_min=1000;soc_edit_max=10000;}
  else if(target>=17 && target<=20) {
    bool minimum=target<=18,increase=target==17 || target==19;
    int& value=minimum?soc_edit_min:soc_edit_max;
    value=soc_step_value(value,minimum,increase);
  } else if(target==16) {
    if(!valid_soc_window(soc_edit_min,soc_edit_max)){text(soc_info,ui_text::soc_invalid);return;}
    soc_pending=1;text(soc_info,ui_text::soc_saving);
  }
  if(!soc_pending)text(soc_info,valid_soc_window(soc_edit_min,soc_edit_max)?ui_text::soc_edit:ui_text::soc_invalid);
  refresh_soc();
}

void enable_soc_scaling() {
  auto& s=datalayer.battery_settings;
  // User asked for command 1=yes, not a toggle or a one-shot calibration.
  if(s.soc_scaling_active || soc_pending)return;
  if(!valid_soc_window(s.min_percentage,s.max_percentage)){
    open_soc();text(soc_info,ui_text::soc_invalid);return;
  }
  soc_pending=2;
}

void service_soc_save() {
  if(!soc_pending)return;
  unsigned operation=soc_pending;soc_pending=0;
  auto& s=datalayer.battery_settings;
  if(operation==1){
    // Do not overwrite an edit made through the web while this menu was open.
    // This is a best-effort conflict check, not a cross-task transaction.
    if(s.min_percentage!=soc_original_min || s.max_percentage!=soc_original_max ||
       s.soc_scaling_active!=soc_original_scaling){text(soc_info,ui_text::soc_conflict);return;}
    if(!valid_soc_window(soc_edit_min,soc_edit_max)){text(soc_info,ui_text::soc_invalid);return;}
    bool changed=soc_edit_min!=s.min_percentage || soc_edit_max!=s.max_percentage;
    // Keep a nonempty intermediate window where the old window was valid.
    if(soc_edit_min>=s.max_percentage){
      s.max_percentage=uint16_t(soc_edit_max);s.min_percentage=int16_t(soc_edit_min);
    } else {
      s.min_percentage=int16_t(soc_edit_min);s.max_percentage=uint16_t(soc_edit_max);
    }
    if(changed)store_settings(); // Same persistence mechanism as the web UI.
    soc_original_min=s.min_percentage;soc_original_max=s.max_percentage;
    text(soc_info,ui_text::soc_applied);
  } else if(operation==2){
    if(!valid_soc_window(s.min_percentage,s.max_percentage)){
      open_soc();text(soc_info,ui_text::soc_invalid);return;
    }
    if(!s.soc_scaling_active){s.soc_scaling_active=true;store_settings();}
  }
  refresh_soc();
}

// UI percent is the selected command range, not calibrated RPM/flow.
unsigned pump_percent(unsigned raw) {
  if(raw>153)raw=153;
  return (raw*100U+76U)/153U;
}

// Keep the validated raw increments (0,5,...150,153); only presentation changes.
unsigned pump_next_raw(unsigned raw, bool increase) {
  if(raw>153)raw=153;
  unsigned next=increase ? (raw/5+1)*5 : raw ? ((raw-1)/5)*5 : 0;
  return next>153 ? 153 : next;
}

void build_cell_row(unsigned row, int y) {
  // 49 bars in 931 pixels = 19 pixels each. No 98-label object allocation.
  auto* chart=lv_chart_create(page_cells);
  cell_charts[row]=chart;
  lv_obj_set_pos(chart,68,y);
  lv_obj_set_size(chart,931,126);
  lv_obj_remove_flag(chart,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(chart,lv_color_hex(UI_PANEL),0);
  lv_obj_set_style_bg_opa(chart,LV_OPA_COVER,0);
  lv_obj_set_style_border_width(chart,0,0);
  lv_obj_set_style_radius(chart,0,0);
  lv_obj_set_style_pad_all(chart,0,0);
  lv_obj_set_style_pad_column(chart,2,0);
  lv_chart_set_type(chart,LV_CHART_TYPE_BAR);
  lv_chart_set_point_count(chart,CELLS_PER_ROW);
  lv_chart_set_axis_range(chart,LV_CHART_AXIS_PRIMARY_Y,3500,4200);
  lv_chart_set_div_line_count(chart,8,0);
  for(auto& v:plotted_cells[row])v=LV_CHART_POINT_NONE;
  cell_series[row]=lv_chart_add_series(chart,lv_color_hex(UI_ACCENT),LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_series_ext_y_array(chart,cell_series[row],plotted_cells[row]);
  // Static axis labels: same fixed voltage scale for both rows.
  label(page_cells,24,y-7,42,"4.2 V",&lv_font_montserrat_14,UI_MUTED);
  label(page_cells,24,y+47,42,"3.9 V",&lv_font_montserrat_14,UI_MUTED);
  label(page_cells,24,y+119,42,"3.5 V",&lv_font_montserrat_14,UI_MUTED);
  auto* scale=lv_scale_create(page_cells);
  lv_obj_remove_style_all(scale);
  lv_obj_remove_flag(scale,LV_OBJ_FLAG_SCROLLABLE);
  // Tick centres match the centres of 19-pixel chart slots.
  lv_obj_set_pos(scale,77,y+130);
  lv_obj_set_size(scale,913,20);
  lv_scale_set_mode(scale,LV_SCALE_MODE_HORIZONTAL_BOTTOM);
  lv_scale_set_range(scale,row*49+1,row*49+49);
  lv_scale_set_total_tick_count(scale,49);
  lv_scale_set_major_tick_every(scale,1);
  lv_scale_set_label_show(scale,true);
  lv_obj_set_style_text_font(scale,&lv_font_montserrat_14,LV_PART_INDICATOR);
  lv_obj_set_style_text_color(scale,lv_color_hex(UI_MUTED),LV_PART_INDICATOR);
  lv_obj_set_style_length(scale,0,LV_PART_INDICATOR);
  lv_obj_set_style_line_width(scale,0,LV_PART_INDICATOR);
  lv_obj_set_style_line_width(scale,0,LV_PART_MAIN);
  lv_obj_set_style_pad_all(scale,0,LV_PART_MAIN);
}

void build_ui() {
  lv_obj_t* root = lv_display_get_screen_active(display);
  lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(root, lv_color_hex(UI_BG), 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  wifi_icon = label(root, 24, 20, 32, LV_SYMBOL_WIFI, &lv_font_montserrat_24, UI_MUTED);
  for (unsigned i = 0; i < 4; ++i) {
    int h = 6 + int(i) * 6;
    wifi_bars[i] = panel(root, 60 + int(i) * 9, 46 - h, 6, h, 0x3A4B58);
    lv_obj_set_style_radius(wifi_bars[i], 1, 0);
  }
  wifi_label = label(root, 106, 22, 220, ui_text::no_wifi, &lv_font_montserrat_20, UI_MUTED);
  lv_label_set_long_mode(wifi_label, LV_LABEL_LONG_MODE_DOTS);
  lv_obj_set_size(wifi_label, 220, 26);
  status_label = label(root, 344, 27, 232, ui_text::can_wait, &lv_font_montserrat_14, UI_ACCENT);
  scale_button=panel(root,588,12,200,52,0x3A4B58);
  scale_label=centred_label(scale_button,190,ui_text::scale_off,&lv_font_montserrat_14);
  lv_obj_t* nav = panel(root, 800, 12, 200, 52, UI_ACCENT);
  navigation = centred_label(nav,180,ui_text::cells,&lv_font_montserrat_20,UI_BG);
  lv_obj_set_style_text_align(navigation, LV_TEXT_ALIGN_CENTER, 0);

  page_main = panel(root, 0, 76, 1024, 474, UI_BG);
  page_cells = panel(root, 0, 76, 1024, 474, UI_BG);
  lv_obj_add_flag(page_cells, LV_OBJ_FLAG_HIDDEN);
  soc_label = metric(24, 8, 314, 142, ui_text::soc);
  lv_obj_set_style_text_color(soc_label, lv_color_hex(UI_ACCENT), 0);
  soh_label = metric(24, 166, 314, 142, ui_text::soh);
  temp_label = metric(354, 8, 314, 142, ui_text::temperature);
  voltage_label = metric(684, 8, 314, 142, ui_text::voltage);
  current_label = metric(354, 166, 314, 142, ui_text::current);
  power_label = metric(684, 166, 314, 142, ui_text::power);

  lv_obj_t* c = panel(page_main, 24, 324, 314, 142, UI_PANEL);
  label(c, 16, 10, 278, ui_text::charge_limit, &lv_font_montserrat_14, UI_MUTED);
  charge_label = label(c, 16, 36, 200, "-- A", &lv_font_montserrat_36);
  lv_obj_t* up = panel(c, 230, 32, 64, 48, UI_ACCENT);
  lv_obj_t* arrow = label(up, 0, 10, 64, LV_SYMBOL_UP, &lv_font_montserrat_24, UI_BG);
  lv_obj_set_style_text_align(arrow, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_t* dn = panel(c, 230, 86, 64, 48, UI_ACCENT);
  arrow = label(dn, 0, 10, 64, LV_SYMBOL_DOWN, &lv_font_montserrat_24, UI_BG);
  lv_obj_set_style_text_align(arrow, LV_TEXT_ALIGN_CENTER, 0);
  charge_effective = label(c, 16, 96, 210, ui_text::available_missing, &lv_font_montserrat_20, UI_MUTED);
  c = panel(page_main, 354, 324, 314, 142, UI_PANEL);
  label(c, 16, 10, 278, ui_text::discharge_limit, &lv_font_montserrat_14, UI_MUTED);
  discharge_label = label(c, 16, 36, 200, "-- A", &lv_font_montserrat_36);
  up = panel(c, 230, 32, 64, 48, UI_ACCENT);
  arrow = label(up, 0, 10, 64, LV_SYMBOL_UP, &lv_font_montserrat_24, UI_BG);
  lv_obj_set_style_text_align(arrow, LV_TEXT_ALIGN_CENTER, 0);
  dn = panel(c, 230, 86, 64, 48, UI_ACCENT);
  arrow = label(dn, 0, 10, 64, LV_SYMBOL_DOWN, &lv_font_montserrat_24, UI_BG);
  lv_obj_set_style_text_align(arrow, LV_TEXT_ALIGN_CENTER, 0);
  discharge_effective = label(c, 16, 96, 210, ui_text::available_missing, &lv_font_montserrat_20, UI_MUTED);
  c = panel(page_main, 684, 324, 314, 142, UI_PANEL);
  label(c, 16, 10, 282, ui_text::pump, &lv_font_montserrat_20, UI_MUTED);
  pump_label = label(c, 16, 45, 282, ui_text::no_link, &lv_font_montserrat_24);
  pump_card_status = label(c, 16, 88, 282, ui_text::pump_unknown, &lv_font_montserrat_14, UI_ACCENT);

  label(page_cells,24,6,440,ui_text::cells_title,&lv_font_montserrat_24);
  cell_count_label=label(page_cells,600,10,400,ui_text::no_data,&lv_font_montserrat_20,UI_MUTED);
  lv_obj_set_style_text_align(cell_count_label,LV_TEXT_ALIGN_RIGHT,0);
  cell_stats=label(page_cells,24,46,976,ui_text::cell_stats_missing,&lv_font_montserrat_20,UI_MUTED);
  build_cell_row(0,92);
  build_cell_row(1,266);
  cell_scale=label(page_cells,24,440,976,"",&lv_font_montserrat_20,UI_MUTED);
  footer_label = label(root, 24, 566, 976, ui_text::no_data, &lv_font_montserrat_20, UI_MUTED);
  lv_obj_set_style_text_align(footer_label, LV_TEXT_ALIGN_CENTER, 0);

  pump_window=panel(root,80,100,864,414,UI_PANEL);
  // Fully opaque rectangular cover: no corner masks or translucent overlay.
  lv_obj_set_style_radius(pump_window,0,0);
  lv_obj_set_style_border_width(pump_window,1,0);
  lv_obj_set_style_border_color(pump_window,lv_color_hex(UI_ACCENT),0);
  label(pump_window,28,22,620,ui_text::pump,&lv_font_montserrat_24);
  auto* close=panel(pump_window,708,14,128,48,0x3A4B58);
  centred_label(close,112,ui_text::close,&lv_font_montserrat_20);
  label(pump_window,40,94,260,ui_text::command,&lv_font_montserrat_14,UI_MUTED);
  pump_value=label(pump_window,40,123,260,"0%",&lv_font_montserrat_40);
  label(pump_window,320,94,240,ui_text::pump_current,&lv_font_montserrat_14,UI_MUTED);
  pump_amps=label(pump_window,320,123,240,"-- A",&lv_font_montserrat_40);
  pump_mode=label(pump_window,40,194,520,ui_text::no_link,&lv_font_montserrat_20,UI_ACCENT);
  label(pump_window,40,230,530,ui_text::auto_rule,&lv_font_montserrat_14,UI_MUTED);
  auto* arrowbox=panel(pump_window,600,86,220,72,UI_ACCENT);
  auto* al=centred_label(arrowbox,220,LV_SYMBOL_UP,&lv_font_montserrat_24,UI_BG);
  pump_buttons[1]=arrowbox; pump_button_ink[1]=al;
  lv_obj_set_style_text_align(al,LV_TEXT_ALIGN_CENTER,0);
  arrowbox=panel(pump_window,600,176,220,72,UI_ACCENT);
  al=centred_label(arrowbox,220,LV_SYMBOL_DOWN,&lv_font_montserrat_24,UI_BG);
  pump_buttons[2]=arrowbox; pump_button_ink[2]=al;
  lv_obj_set_style_text_align(al,LV_TEXT_ALIGN_CENTER,0);
  pump_info=label(pump_window,40,284,780,ui_text::link_blocked,&lv_font_montserrat_20,UI_MUTED);
  auto* off=panel(pump_window,28,340,300,52,UI_ACCENT);
  pump_buttons[0]=off;
  pump_button_ink[0]=centred_label(off,268,ui_text::manual_zero,&lv_font_montserrat_20,UI_BG);
  label(pump_window,352,359,480,ui_text::zero_note,&lv_font_montserrat_14,UI_MUTED);
  lv_obj_add_flag(pump_window,LV_OBJ_FLAG_HIDDEN);

  soc_window=panel(root,80,100,864,414,UI_PANEL);
  lv_obj_set_style_radius(soc_window,0,0);
  lv_obj_set_style_border_width(soc_window,1,0);
  lv_obj_set_style_border_color(soc_window,lv_color_hex(UI_ACCENT),0);
  label(soc_window,28,22,600,ui_text::soc_window,&lv_font_montserrat_24);
  auto* cancel=panel(soc_window,708,14,128,48,0x3A4B58);
  centred_label(cancel,112,ui_text::soc_cancel,&lv_font_montserrat_20);
  label(soc_window,40,94,260,ui_text::soc_min,&lv_font_montserrat_20,UI_MUTED);
  soc_min_value=label(soc_window,40,133,260,"",&lv_font_montserrat_40);
  label(soc_window,440,94,260,ui_text::soc_max,&lv_font_montserrat_20,UI_MUTED);
  soc_max_value=label(soc_window,440,133,260,"",&lv_font_montserrat_40);
  for(unsigned i=0;i<4;++i){
    auto* button=panel(soc_window,i<2?300:700,(i%2)==0?92:158,120,56,UI_ACCENT);
    centred_label(button,110,(i%2)==0?LV_SYMBOL_UP:LV_SYMBOL_DOWN,&lv_font_montserrat_24,UI_BG);
  }
  label(soc_window,40,234,780,ui_text::soc_range,&lv_font_montserrat_14,UI_MUTED);
  label(soc_window,40,261,780,ui_text::soc_note,&lv_font_montserrat_14,UI_MUTED);
  soc_info=label(soc_window,40,294,780,ui_text::soc_edit,&lv_font_montserrat_20,UI_MUTED);
  auto* preset=panel(soc_window,28,340,200,52,0x3A4B58);
  centred_label(preset,190,ui_text::soc_preset,&lv_font_montserrat_20);
  auto* save=panel(soc_window,636,340,200,52,UI_ACCENT);
  centred_label(save,190,ui_text::soc_save,&lv_font_montserrat_20,UI_BG);
  lv_obj_add_flag(soc_window,LV_OBJ_FLAG_HIDDEN);
  refresh_soc();
}

void toggle_pump_local() {
  pump_notice=nullptr;
  pump_pulse=-1;
  refresh_pump();  // Cheap snapshot only, no Wi-Fi or battery-page refresh.
  pump_window_open=true;
  lv_obj_remove_flag(pump_window,LV_OBJ_FLAG_HIDDEN);
}

void pump_step(int action) {
  auto s=pump_link::get();unsigned value=s.manual;
  if(action==10)value=0;
  else if(action==11)value=pump_next_raw(value,true);
  else if(action==12)value=pump_next_raw(value,false);
  bool accepted=pump_link::set_manual(uint8_t(value));
  pump_notice=accepted?ui_text::accepted:ui_text::refused;
  pump_notice_at=millis();
  if (accepted) {
    pump_pulse=action-10;
    pump_pulse_at=millis();
  }
  refresh_pump();
  // Do not fabricate a new snapshot. The visual pulse acknowledges this tap;
  // the number follows the worker, separately from sent/feedback/current.
  pump_refresh_requested=true;
}

void paint_pump_controls(bool ready_for_command, unsigned manual) {
  static int old_style[3] = {-1,-1,-1};
  if (pump_pulse >= 0 && uint32_t(millis()-pump_pulse_at) >= 90) pump_pulse=-1;
  for (int i=0;i<3;++i) {
    bool enabled=i==0 || (ready_for_command && (i==1 ? manual<153 : manual>0));
    int style=!enabled ? 0 : pump_pulse==i ? 2 : 1;
    if (style==old_style[i]) continue;
    old_style[i]=style;
    lv_obj_set_style_bg_color(pump_buttons[i],
                             lv_color_hex(style==0 ? 0x3A4B58 : style==2 ? UI_TEXT : UI_ACCENT),0);
    lv_obj_set_style_text_color(pump_button_ink[i],lv_color_hex(style==0 ? UI_MUTED : UI_BG),0);
  }
}

void refresh_pump() {
  auto s=pump_link::get();char b[160];
  bool online=s.online && uint32_t(millis()-s.reply_at)<=1000;
  bool valid=s.enabled && online && s.pump_ready;
  paint_pump_controls(valid,s.manual);
  snprintf(b,sizeof(b),"%u%%",pump_percent(s.manual));text(pump_value,b);
  if(!valid) {
    text(pump_amps,"-- A");
    const char* state=!s.enabled?ui_text::link_off:!online?ui_text::no_link:ui_text::no_feedback;
    text(pump_label,state);text(pump_mode,state);
    text(pump_card_status,!s.enabled || !online?ui_text::pump_unknown:ui_text::pump_check);
    text(pump_info,!s.enabled?ui_text::link_disabled:!online?ui_text::link_blocked:ui_text::feedback_blocked);
    pump_notice=nullptr;
  }
  else {
    const char* mode=s.automatic?ui_text::automatic:ui_text::manual;
    snprintf(b,sizeof(b),"%s %u%%",mode,pump_percent(s.desired));text(pump_label,b);
    text(pump_mode,s.temperature_valid?mode:ui_text::temperature_missing);
    snprintf(b,sizeof(b),"%.1f A",s.feedback_current/10.0);text(pump_amps,b);text(pump_card_status,b);
    if(pump_notice && uint32_t(millis()-pump_notice_at)<1200)text(pump_info,pump_notice);
    else {pump_notice=nullptr;snprintf(b,sizeof(b),ui_text::sent,pump_percent(s.sent));text(pump_info,b);}
  }
}

void change_page() {
  cells_visible = !cells_visible;
  if (cells_visible) {
    lv_obj_add_flag(page_main, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(page_cells, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(page_cells, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(page_main, LV_OBJ_FLAG_HIDDEN);
  }
  text(navigation, cells_visible ? ui_text::dashboard : ui_text::cells);
  refresh_requested = true;
}

void adjust_limit(bool charge, bool increase) {
  auto& settings = datalayer.battery_settings;
  uint16_t& target = charge ? settings.max_user_set_charge_dA : settings.max_user_set_discharge_dA;
  int old = target;
  // Values from other interfaces may be off-grid: preserve exact +/-5 A,
  // clamp the result, never wrap uint16_t. Do not alter anything on boot.
  int next = old + (increase ? 50 : -50);
  if (next < 0) next = 0;
  if (next > 400) next = 400;
  if (next == old) return;
  target = static_cast<uint16_t>(next);
  // Reflect the local setting immediately, without polling Wi-Fi/all metrics.
  char b[24];
  snprintf(b,sizeof(b),"%.1f A",target/10.0);
  text(charge ? charge_label : discharge_label,b);
}

int hit_target(int x, int y) {
  if(soc_pending)return 0;
  if(soc_window_open){
    if(x>=788&&x<916&&y>=114&&y<162)return 15;
    if(x>=716&&x<916&&y>=440&&y<492)return 16;
    if(x>=108&&x<308&&y>=440&&y<492)return 21;
    bool low=x>=380&&x<500,high=x>=780&&x<900;
    if(low||high){
      if(y>=192&&y<248)return low?17:19;
      if(y>=258&&y<314)return low?18:20;
    }
    return 0;
  }
  if(pump_window_open) {
    if(x>=788&&x<916&&y>=114&&y<162)return 9;
    if(x>=108&&x<408&&y>=440&&y<492)return 10;
    if(x>=680&&x<900&&y>=186&&y<258)return 11;
    if(x>=680&&x<900&&y>=276&&y<348)return 12;
    return 0;
  }
  if (x >= 800 && x < 1000 && y >= 12 && y < 64) return 1;
  if (x >= 588 && x < 788 && y >= 12 && y < 64) return 14;
  if (cells_visible) {
    return 0;
  }
  if (x >= 684 && x < 998 && y >= 400 && y < 542) return 2;
  if (x >= 24 && x < 338 && y >= 84 && y < 226) return 13;
  bool charge = x >= 254 && x < 318;
  bool discharge = x >= 584 && x < 648;
  if (charge || discharge) {
    if (y >= 432 && y < 480) return charge ? 3 : 5;
    if (y >= 486 && y < 534) return charge ? 4 : 6;
  }
  return 0;
}

bool touch_read(uint16_t reg, uint8_t* bytes, size_t len) {
  uint8_t a[2] = {uint8_t(reg >> 8), uint8_t(reg)};
  return i2c_master_transmit_receive(touch_device, a, 2, bytes, len, 20) == ESP_OK;
}
bool touch_ack() {
  uint8_t a[3] = {0x81, 0x4E, 0};
  return i2c_master_transmit(touch_device, a, 3, 20) == ESP_OK;
}

bool init_touch() {
  if (!esp32hal->alloc_pins("GT911", GPIO_NUM_4)) return false;
  for (int attempt = 0; attempt < 2; ++attempt) {
    const uint8_t address = attempt ? 0x14 : 0x5D;
    pinMode(4, OUTPUT);
    digitalWrite(4, attempt ? HIGH : LOW);
    if (!write_byte(expander_io, 0xFD)) return false;
    delay(100);
    if (!write_byte(expander_io, 0xFF)) return false;
    delay(10);
    pinMode(4, INPUT);
    delay(50);
    esp_task_wdt_reset();
    i2c_device_config_t cfg = {};
    cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    cfg.device_address = address;
    cfg.scl_speed_hz = 100000;
    if (i2c_master_bus_add_device(bus_handle, &cfg, &touch_device) != ESP_OK) continue;
    uint8_t id[4] = {}, range[4] = {};
    if (touch_read(0x8140, id, 4) && id[0] == '9' && id[1] == '1' && id[2] == '1' &&
        touch_read(0x8048, range, 4)) {
      touch_width = uint16_t(range[0]) | (uint16_t(range[1]) << 8);
      touch_height = uint16_t(range[2]) | (uint16_t(range[3]) << 8);
      if (touch_width && touch_height && touch_ack()) {
        Serial.printf("LCD5B: GT911 at 0x%02X; range=%ux%u\n", address, touch_width, touch_height);
        return true;
      }
    }
    i2c_master_bus_rm_device(touch_device);
    touch_device = nullptr;
  }
  Serial.println("LCD5B: GT911 unavailable; display remains read-only");
  return false;
}

// One action per confirmed contact; no auto-repeat, drag-through or release action.
// Debounce only the SAME target, so opening a modal never delays its close button.
void poll_touch(uint32_t now) {
  static uint32_t last_poll = 0, last_action = 0;
  static int last_target = 0;
  static bool latched = true;
  if (!touch_ready || uint32_t(now - last_poll) < 15) return;
  last_poll = now;
  uint8_t status = 0;
  if (!touch_read(0x814E, &status, 1)) {
    latched=true; ++ui_metrics.errors; return;
  }
  if (!(status & 0x80)) return;
  uint8_t count = status & 15;
  if (!count) {
    if (touch_ack()) latched = false;
    else {latched=true; ++ui_metrics.errors;}
    return;
  }
  uint8_t point[8] = {};
  bool ok = count == 1 && touch_read(0x814F, point, sizeof(point));
  ok = touch_ack() && ok;
  if (!ok) { latched = true; ++ui_metrics.errors; return; }
  if (latched) return;
  latched = true;
  uint16_t rx = uint16_t(point[1]) | (uint16_t(point[2]) << 8);
  uint16_t ry = uint16_t(point[3]) | (uint16_t(point[4]) << 8);
  if (rx >= touch_width || ry >= touch_height) return;
  int x = uint32_t(rx) * LCD_WIDTH / touch_width;
  int y = uint32_t(ry) * LCD_HEIGHT / touch_height;
  int target = hit_target(x, y);
  if (!target) return;
  if (target==last_target && uint32_t(now-last_action)<80) {
    ++ui_metrics.debounce; return;
  }
  last_action = now;
  last_target = target;
  ++ui_metrics.taps;
  action_started_us=micros();
  action_pending=true;
  if (target == 1) change_page();
  else if (target == 2) toggle_pump_local();
  else if (target >= 3 && target <= 6) adjust_limit(target <= 4, target == 3 || target == 5);
  else if (target == 9) {
    pump_window_open=false; pump_pulse=-1;
    lv_obj_add_flag(pump_window,LV_OBJ_FLAG_HIDDEN);
  }
  else if (target >= 10 && target <= 12) pump_step(target);
  else if (target == 13) open_soc();
  else if (target == 14) enable_soc_scaling();
  else if (target >= 15 && target <= 21) soc_action(target);
  render_requested = true;
}

void refresh_wifi() {
  // Only query existing Wi-Fi state: no scans, reconnects or credential writes.
  bool connected = WiFi.status() == WL_CONNECTED;
  int level = 0;
  String name;
  if (connected) {
    int rssi = WiFi.RSSI();
    level = rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : 1;
    name = WiFi.SSID();
  } else {
    auto mode = WiFi.getMode();
    name = (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) ? ui_text::access_point : ui_text::no_wifi;
  }
  String caption = String("| ") + name;
  text(wifi_label, caption.c_str());
  static int previous = -1;
  if (level != previous) {
    for (unsigned i = 0; i < 4; ++i)
      lv_obj_set_style_bg_color(wifi_bars[i], lv_color_hex(int(i) < level ? UI_ACCENT : 0x3A4B58), 0);
    lv_obj_set_style_text_color(wifi_icon, lv_color_hex(connected ? UI_ACCENT : UI_MUTED), 0);
    previous = level;
  }
}

void refresh_data() {
  refresh_soc();
  refresh_wifi();
  // Read-only best-effort snapshot, NOT an atomic/validated BMS measurement.
  const auto& b = datalayer.battery;
  bool live = battery_detected && b.status.CAN_battery_still_alive > 0;
  text(status_label, live ? ui_text::can_live : ui_text::can_missing);
  char s[200];
  if (!cells_visible) {
    if (live) {
      snprintf(s, sizeof(s), "%.1f %%", b.status.real_soc / 100.0); text(soc_label, s);
      if (b.status.soh_available) {
        snprintf(s, sizeof(s), "%.1f %%", b.status.soh_pptt / 100.0); text(soh_label, s);
      } else text(soh_label, "-- %");
      snprintf(s, sizeof(s), "%.1f C", b.status.temperature_max_dC / 10.0); text(temp_label, s);
      snprintf(s, sizeof(s), "%.1f V", b.status.voltage_dV / 10.0); text(voltage_label, s);
      snprintf(s, sizeof(s), "%+.1f A", b.status.current_dA / 10.0); text(current_label, s);
      snprintf(s, sizeof(s), "%+.2f kW", b.status.active_power_W / 1000.0); text(power_label, s);
      snprintf(s, sizeof(s), ui_text::available, b.status.max_charge_current_dA / 10.0); text(charge_effective, s);
      snprintf(s, sizeof(s), ui_text::available, b.status.max_discharge_current_dA / 10.0); text(discharge_effective, s);
    } else {
      text(soc_label, "-- %"); text(soh_label, "-- %"); text(temp_label, "-- C");
      text(voltage_label, "-- V"); text(current_label, "-- A"); text(power_label, "-- kW");
      text(charge_effective, ui_text::available_missing); text(discharge_effective, ui_text::available_missing);
    }
    snprintf(s, sizeof(s), "%.1f A", datalayer.battery_settings.max_user_set_charge_dA / 10.0); text(charge_label, s);
    snprintf(s, sizeof(s), "%.1f A", datalayer.battery_settings.max_user_set_discharge_dA / 10.0); text(discharge_label, s);
  } else {
    unsigned n = b.info.number_of_cells;
    if (n > MAX_AMOUNT_CELLS) n = MAX_AMOUNT_CELLS;
    unsigned valid_count = 0, mini = 0, maxi = 0, below = 0, above = 0;
    int32_t lo = INT32_MAX, hi = 0;
    bool changed[2] = {refresh_requested,refresh_requested};
    for (unsigned i = 0; i < PACK_CELLS; ++i) {
      uint16_t mv = (live && i < n) ? b.status.cell_voltages_mV[i] : 0;
      int32_t v = mv ? mv : LV_CHART_POINT_NONE;
      unsigned row=i/CELLS_PER_ROW, col=i%CELLS_PER_ROW;
      // Preserve raw values outside the scale. Statistics expose clipping.
      if(plotted_cells[row][col]!=v){plotted_cells[row][col]=v;changed[row]=true;}
      if (mv) {
        ++valid_count;
        if (mv < lo) { lo = mv; mini = i + 1; }
        if (mv > hi) { hi = mv; maxi = i + 1; }
        if(mv<3500)++below;
        if(mv>4200)++above;
      }
    }
    snprintf(s, sizeof(s), ui_text::readings, valid_count);
    text(cell_count_label, s);
    if (n && n != PACK_CELLS) {
      snprintf(s,sizeof(s),ui_text::count_mismatch,unsigned(b.info.number_of_cells));
      text(cell_count_label,s);
    }
    if (valid_count) {
      snprintf(s,sizeof(s),ui_text::cell_stats,
               lo/1000.0,mini,hi/1000.0,maxi,long(hi-lo));text(cell_stats,s);
    } else {
      text(cell_stats,ui_text::cell_stats_missing);
    }
    snprintf(s,sizeof(s),ui_text::cell_range,below,above,PACK_CELLS-valid_count);
    text(cell_scale,s);
    for(unsigned row=0;row<2;++row)if(changed[row])lv_chart_refresh(cell_charts[row]);
  }
  // Dala datalayer: positive W = charging, negative W = discharging.
  // CAN timeout is shared with the base: not per-sample freshness.
  const int32_t watts = b.status.active_power_W;
  const char* state = !live ? ui_text::no_data :
                      watts > 0 ? ui_text::charging :
                      watts < 0 ? ui_text::discharging : ui_text::idle;
  text(footer_label, state);
}
}  // namespace

void init_display() {
  if (attempted) return;
  attempted = true;
  Serial.println("LCD5B: UI-200 EN / SOC window / rescale enable / pump RX fix external");

  // Register all LCD/I2C pins with the base's conflict detector.
  if (!esp32hal->alloc_pins("LCD5B", 14,38,18,17,10,39,0,45,48,47,21,
                           1,2,42,41,40,5,3,46,7,8,9)) return;
  if (!psramFound()) {
    Serial.println("LCD5B: PSRAM not detected; display disabled");
    return;
  }

  i2c_master_bus_config_t cfg = {};
  cfg.i2c_port = I2C_NUM_0;
  cfg.sda_io_num = GPIO_NUM_8;
  cfg.scl_io_num = GPIO_NUM_9;
  cfg.clk_source = I2C_CLK_SRC_DEFAULT;
  cfg.glitch_ignore_cnt = 7;
  cfg.flags.enable_internal_pullup = true;
  if (!checked(i2c_new_master_bus(&cfg, &bus_handle), "I2C bus")) return;
  if (!add_device(0x24, &expander_mode) || !add_device(0x38, &expander_io)) return;

  // CH422G WR_IO: EXIO1 touch reset, EXIO2 backlight, EXIO3 LCD reset.
  // No writes to the separate open-drain DO output register.
  if (!write_byte(expander_mode, 0x01)) return;
  if (!write_byte(expander_io, 0xF1)) return; // resets low, backlight off
  delay(10);
  if (!write_byte(expander_io, 0xF9)) return; // release LCD reset only
  delay(100);
  esp_task_wdt_reset();
  if (!lcd.init()) {
    Serial.println("LCD5B: RGB initialization failed");
    return;
  }
  lcd.setRotation(0);
  lcd.fillScreen(0x0000);
  esp_task_wdt_reset();

  lv_init();
  lv_tick_set_cb(tick_ms);
  constexpr size_t bytes = LCD_WIDTH * 10 * sizeof(uint16_t);
  draw_buffer = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  if (!draw_buffer) {
    Serial.println("LCD5B: cannot allocate 20480-byte draw buffer");
    return;
  }
  display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
  if (!display) {
    heap_caps_free(draw_buffer);
    draw_buffer = nullptr;
    Serial.println("LCD5B: cannot create LVGL display");
    return;
  }
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(display, draw_buffer, nullptr, bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, flush);

  build_ui();
  // First enable backlight, then release/select the GT911 address.
  if (!write_byte(expander_io, 0xFD)) return;
  touch_ready = init_touch();
  refresh_pump();
  refresh_data();
  ready = true;
  Serial.printf("LCD5B: ready; buffer=%u, free internal=%u, free PSRAM=%u\n",
                unsigned(bytes), unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
                unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
}

void update_display() {
  // Existing connectivity_loop owns both init_display and update_display.
  // No other task may call LVGL in this driver.
  if (!ready) return;
  uint32_t now = millis();
  uint32_t started=micros();
  static uint32_t last_entry_us=0;
  static bool have_entry=false;
  if(have_entry)record_max(ui_metrics.gap,uint32_t(started-last_entry_us));
  last_entry_us=started; have_entry=true;
  poll_touch(now);
  record_max(ui_metrics.input,uint32_t(micros()-started));
  static uint32_t last_update = 0;
  static uint32_t last_pump_update = 0;
  const bool urgent=render_requested;
  // A page change needs its data before painting. Local pump/limit actions do
  // not. Defer a coincident periodic update ONE iteration, never for a whole tap.
  if (refresh_requested || (!urgent && uint32_t(now-last_update)>=1000)) {
    started=micros();
    refresh_data();
    record_max(ui_metrics.data,uint32_t(micros()-started));
    last_update = now;
    refresh_requested = false;
  }
  // Pump state is independent of the one-second dashboard cadence.
  // Allow the worker to publish, rather than rereading it immediately in a
  // higher-priority UI loop. A pending update never delays the normal cadence.
  if (!urgent && ((pump_refresh_requested && uint32_t(now-pump_notice_at)>=20) ||
                 uint32_t(now-last_pump_update)>=(pump_window_open ? 100U : 250U))) {
    refresh_pump();
    last_pump_update=now;
    pump_refresh_requested=false;
  }
  if (!urgent && pump_window_open && pump_pulse>=0 &&
      uint32_t(now-pump_pulse_at)>=90) {
    refresh_pump();
  }
  frame_copy_us=0;
  started=micros();
  if (urgent) {
    // Public LVGL 9.3 API. Same task, outside timer/flush callbacks.
    // Only dirty regions, no full-screen invalidation or forced animation.
    lv_refr_now(display);
    render_requested=false;
  } else {
    lv_timer_handler();
  }
  record_max(ui_metrics.draw,uint32_t(micros()-started));
  record_max(ui_metrics.copy,frame_copy_us);
  if (action_pending) {
    record_max(ui_metrics.action,uint32_t(micros()-action_started_us));
    action_pending=false;
  }
  if (!urgent) report_ui(now);
  // Paint the acknowledgment first. No writes on boot, opening, arrows or Cancel.
  // NVM write may briefly stall; it is explicit, infrequent and not in core_loop.
  if (!urgent && soc_pending) service_soc_save();
}
#endif
