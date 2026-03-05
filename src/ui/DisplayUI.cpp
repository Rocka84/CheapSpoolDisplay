#include "DisplayUI.h"
#include "../config/ConfigManager.h"
#include "../network/NetworkManager.h"

#ifndef USE_SDL2
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#else
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <string>
#endif

// External assets
extern "C" {
extern const lv_image_dsc_t img_openspool_logo;
}

// Screen dimensions
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 320;

#ifndef USE_SDL2
// Hardware drivers
static TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight);
XPT2046_Touchscreen ts(TOUCH_CS);
#endif

// LVGL static objects
lv_obj_t *DisplayUI::scanScreen = nullptr;
lv_obj_t *DisplayUI::infoScreen = nullptr;
lv_obj_t *DisplayUI::toolSelectionScreen = nullptr;
lv_obj_t *DisplayUI::editScreen = nullptr;

lv_obj_t *DisplayUI::labelBrand = nullptr;
lv_obj_t *DisplayUI::labelType = nullptr;
lv_obj_t *DisplayUI::colorBox = nullptr;
lv_obj_t *DisplayUI::labelSpoolId = nullptr;
lv_obj_t *DisplayUI::labelColorHex = nullptr;
lv_obj_t *DisplayUI::loadBtn = nullptr;

// We need to keep track of the spool ID locally for the webhook
static std::string currentLoadedSpoolId = "";

/* Display flushing */
#ifndef USE_SDL2
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

/* Touch reading */
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    // Calibration values for CYD 2.8" touch screen
    // These may need tweaking per device
    data->point.x = map(p.x, 200, 3700, 0, screenWidth);
    data->point.y = map(p.y, 240, 3800, 0, screenHeight);
    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}
#endif

void DisplayUI::init() {
#ifndef USE_SDL2
  tft.begin();
  tft.invertDisplay(true);
  tft.setRotation(0); // Portrait
  tft.fillScreen(TFT_BLACK);

  // Init Touch
  ts.begin();
  ts.setRotation(0);
#endif

  // Init LVGL
  lv_init();

#ifndef USE_SDL2
  static lv_color_t
      buf[screenWidth * 20]; // Larger buffer for smoother gradients
  lv_display_t *disp = lv_display_create(screenWidth, screenHeight);
  lv_display_set_buffers(disp, buf, NULL, sizeof(buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
#else
  lv_sdl_window_create(screenWidth, screenHeight);
  lv_sdl_mouse_create();
  lv_sdl_keyboard_create();
  lv_sdl_mousewheel_create();
#endif

  // Build screens
  buildScanScreen();
  buildInfoScreen();
  buildToolSelectionScreen();
  buildEditScreen();

  // Set default dark background for all screens and hide scrollbars
  auto set_premium_bg = [](lv_obj_t *scr) {
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0f1118), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
  };
  set_premium_bg(scanScreen);
  set_premium_bg(infoScreen);
  set_premium_bg(toolSelectionScreen);
  set_premium_bg(editScreen);

  // Show initial screen
  showScanScreen();
}

void DisplayUI::tick() {
  lv_timer_handler(); // let the GUI do its work
}

/* --- Premium Helpers --- */
static void apply_glass_style(lv_obj_t *obj) {
  lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_10, 0);
  lv_obj_set_style_border_color(obj, lv_color_white(), 0);
  lv_obj_set_style_border_opa(obj, LV_OPA_20, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_radius(obj, 16, 0);
  lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

static void apply_indigo_btn_style(lv_obj_t *obj) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x5266ff), 0);
  lv_obj_set_style_radius(obj, 12, 0);
  lv_obj_set_style_shadow_width(obj, 15, 0);
  lv_obj_set_style_shadow_color(obj, lv_color_hex(0x000000), 0);
  lv_obj_set_style_shadow_opa(obj, LV_OPA_40, 0);
  lv_obj_set_style_shadow_ofs_y(obj, 5, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
}

void DisplayUI::buildScanScreen() {
  scanScreen = lv_obj_create(NULL);
  lv_obj_set_scroll_dir(scanScreen, LV_DIR_NONE);

  // OpenSpool Logo with glow effect
  lv_obj_t *logo = lv_image_create(scanScreen);
  lv_image_set_src(logo, &img_openspool_logo);
  lv_obj_align(logo, LV_ALIGN_CENTER, 0, -50);
  lv_obj_set_style_shadow_width(logo, 20, 0);
  lv_obj_set_style_shadow_color(logo, lv_color_hex(0xff4d4d), 0);
  lv_obj_set_style_shadow_opa(logo, LV_OPA_30, 0);

  lv_obj_t *header = lv_label_create(scanScreen);
  lv_label_set_text(header, "Ready to Scan");
  lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(header, lv_color_white(), 0);
  lv_obj_align(header, LV_ALIGN_CENTER, 0, 40);

  lv_obj_t *desc = lv_label_create(scanScreen);
  lv_label_set_text(desc, "Hold reader over tag");
  lv_obj_set_style_text_color(desc, lv_color_hex(0x9ca3af), 0);
  lv_obj_align(desc, LV_ALIGN_CENTER, 0, 75);
}

void DisplayUI::buildInfoScreen() {
  infoScreen = lv_obj_create(NULL);
  lv_obj_set_scroll_dir(infoScreen, LV_DIR_NONE);

  // Header Title
  lv_obj_t *header = lv_label_create(infoScreen);
  lv_label_set_text(header, "Spool Data");
  lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(header, lv_color_hex(0x5266ff), 0);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 15);

  // Glass Card Content
  lv_obj_t *card = lv_obj_create(infoScreen);
  lv_obj_set_size(card, 220, 165);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 50);
  apply_glass_style(card);
  lv_obj_set_style_pad_all(card, 12, 0);

  auto create_row = [&](int y, const char *key, lv_obj_t **val_label) {
    lv_obj_t *k = lv_label_create(card);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, lv_color_hex(0x9ca3af), 0);
    lv_obj_align(k, LV_ALIGN_TOP_LEFT, 0, y);

    *val_label = lv_label_create(card);
    lv_label_set_text(*val_label, "---");
    lv_obj_set_style_text_color(*val_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(*val_label, &lv_font_montserrat_14, 0);
    lv_obj_align(*val_label, LV_ALIGN_TOP_LEFT, 65, y);
  };

  create_row(0, "Brand", &labelBrand);
  create_row(28, "Type", &labelType);
  create_row(56, "ID", &labelSpoolId);

  // Color row with enhanced preview
  lv_obj_t *cLabel = lv_label_create(card);
  lv_label_set_text(cLabel, "Color");
  lv_obj_set_style_text_color(cLabel, lv_color_hex(0x9ca3af), 0);
  lv_obj_align(cLabel, LV_ALIGN_TOP_LEFT, 0, 90);

  colorBox = lv_obj_create(card);
  lv_obj_set_size(colorBox, 131, 24);
  lv_obj_align(colorBox, LV_ALIGN_TOP_LEFT, 65, 87);
  lv_obj_set_style_radius(colorBox, 6, 0);
  lv_obj_set_style_border_width(colorBox, 1, 0);
  lv_obj_set_style_border_color(colorBox, lv_color_white(), 0);
  lv_obj_set_style_border_opa(colorBox, LV_OPA_40, 0);

  labelColorHex = lv_label_create(card);
  lv_label_set_text(labelColorHex, "#------");
  lv_obj_set_style_text_font(labelColorHex, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(labelColorHex, lv_color_white(), 0);
  lv_obj_align(labelColorHex, LV_ALIGN_TOP_LEFT, 65, 116);

  // Bottom Buttons
  loadBtn = lv_btn_create(infoScreen);
  lv_obj_set_size(loadBtn, 200, 42);
  lv_obj_align(loadBtn, LV_ALIGN_BOTTOM_MID, 0, -55);
  apply_indigo_btn_style(loadBtn);
  lv_obj_add_event_cb(loadBtn, onLoadSpoolButtonClicked, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *loadLbl = lv_label_create(loadBtn);
  lv_label_set_text(loadLbl, "Load to Printer");
  lv_obj_center(loadLbl);

#ifndef USE_SDL2
  // Hide the Load button on physical device if no webhook is configured
  if (ConfigManager::getWebhook().empty()) {
    lv_obj_add_flag(loadBtn, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(loadBtn, LV_OBJ_FLAG_HIDDEN);
  }
#endif

  // Small sub-buttons
  lv_obj_t *backBtn = lv_btn_create(infoScreen);
  lv_obj_set_size(backBtn, 95, 38);
  lv_obj_align(backBtn, LV_ALIGN_BOTTOM_LEFT, 15, -12);
  apply_glass_style(backBtn);
  lv_obj_add_event_cb(backBtn, onBackButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *backLbl = lv_label_create(backBtn);
  lv_label_set_text(backLbl, "Back");
  lv_obj_center(backLbl);

  lv_obj_t *editBtn = lv_btn_create(infoScreen);
  lv_obj_set_size(editBtn, 95, 38);
  lv_obj_align(editBtn, LV_ALIGN_BOTTOM_RIGHT, -15, -12);
  apply_glass_style(editBtn);
  lv_obj_add_event_cb(editBtn, onEditButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *editLbl = lv_label_create(editBtn);
  lv_label_set_text(editLbl, "Edit Tag");
  lv_obj_center(editLbl);
}

void DisplayUI::buildToolSelectionScreen() {
  toolSelectionScreen = lv_obj_create(NULL);

  lv_obj_t *header = lv_label_create(toolSelectionScreen);
  lv_label_set_text(header, "Assign to Tool");
  lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(header, lv_color_white(), 0);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 15);

  lv_obj_t *grid = lv_obj_create(toolSelectionScreen);
  lv_obj_set_size(grid, 220, 200);
  lv_obj_align(grid, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_bg_opa(grid, 0, 0);
  lv_obj_set_style_border_width(grid, 0, 0);
  lv_obj_set_scroll_dir(grid, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_AUTO);

  uint8_t num_tools = ConfigManager::getNumTools();
  for (int i = 0; i < num_tools; i++) {
    lv_obj_t *tBtn = lv_btn_create(grid);
    lv_obj_set_size(tBtn, 95, 85);

    int col = i % 2;
    int row = i / 2;
    lv_obj_align(tBtn, LV_ALIGN_TOP_LEFT, col * 105, row * 95);
    apply_indigo_btn_style(tBtn);

    lv_obj_add_event_cb(tBtn, onToolButtonClicked, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);

    lv_obj_t *tLbl = lv_label_create(tBtn);
    lv_label_set_text_fmt(tLbl, "T %d", i);
    lv_obj_set_style_text_font(tLbl, &lv_font_montserrat_20, 0);
    lv_obj_center(tLbl);
  }

  lv_obj_t *backBtn = lv_btn_create(toolSelectionScreen);
  lv_obj_set_size(backBtn, 100, 35);
  lv_obj_align(backBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
  apply_glass_style(backBtn);
  lv_obj_add_event_cb(backBtn, onBackButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *backLbl = lv_label_create(backBtn);
  lv_label_set_text(backLbl, "Cancel");
  lv_obj_center(backLbl);
}

void DisplayUI::buildEditScreen() {
  editScreen = lv_obj_create(NULL);

  lv_obj_t *header = lv_label_create(editScreen);
  lv_label_set_text(header, "Edit Spool");
  lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(header, lv_color_white(), 0);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 15);

  lv_obj_t *msg = lv_label_create(editScreen);
  lv_label_set_text(msg, "Feature update pending...");
  lv_obj_set_style_text_color(msg, lv_color_hex(0x9ca3af), 0);
  lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *backBtn = lv_btn_create(editScreen);
  lv_obj_set_size(backBtn, 100, 35);
  lv_obj_align(backBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
  apply_glass_style(backBtn);
  lv_obj_add_event_cb(backBtn, onBackButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *backLbl = lv_label_create(backBtn);
  lv_label_set_text(backLbl, "Back");
  lv_obj_center(backLbl);
}

void DisplayUI::showScanScreen() { lv_scr_load(scanScreen); }

void DisplayUI::showInfoScreen(const char *brand, const char *type,
                               const char *color_hex, const char *spool_id) {
  lv_label_set_text(labelBrand, brand);
  lv_label_set_text(labelType, type);
  lv_label_set_text(labelSpoolId, spool_id);

  // Convert hex string to lv_color_t
  // We assume color_hex is like "#FF0000"
  if (color_hex && color_hex[0] == '#' && strlen(color_hex) == 7) {
    uint32_t color_val = strtol(&color_hex[1], NULL, 16);
    lv_obj_set_style_bg_color(colorBox, lv_color_hex(color_val), 0);
    lv_label_set_text(labelColorHex, color_hex);
  } else {
    lv_label_set_text(labelColorHex, "Unknown");
  }

  // Cache spool ID globally for webhook
  currentLoadedSpoolId = spool_id;

#ifndef USE_SDL2
  // Hide the Load button on physical device if no webhook is configured
  // We re-check this every time we open InfoScreen in case config changed
  if (ConfigManager::getWebhook().empty()) {
    lv_obj_add_flag(loadBtn, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(loadBtn, LV_OBJ_FLAG_HIDDEN);
  }
#endif

  lv_scr_load(infoScreen);
}

void DisplayUI::showToolSelectionScreen() { lv_scr_load(toolSelectionScreen); }

void DisplayUI::showEditScreen() { lv_scr_load(editScreen); }

void DisplayUI::onLoadSpoolButtonClicked(lv_event_t *e) {
  uint8_t tools = ConfigManager::getNumTools();

  if (tools == 1) {
    // Immediate Webhook Fire
#ifndef USE_SDL2
    Serial.printf("Single tool mode: Auto-selected Tool 0 for spool %s\n",
                  currentLoadedSpoolId.c_str());
    NetworkManager::sendWebhookPayload(currentLoadedSpoolId, 0);
#else
    printf("Simulator [Single Tool Mode]: Auto-selected Tool 0 for spool %s\n",
           currentLoadedSpoolId.c_str());
#endif
    lv_scr_load(infoScreen); // Stay on Info screen
  } else {
    // Show tool grid popup
    showToolSelectionScreen();
  }
}

void DisplayUI::onToolButtonClicked(lv_event_t *e) {
  int toolhead_id = (int)(intptr_t)lv_event_get_user_data(e);
#ifndef USE_SDL2
  Serial.printf("User selected Tool %d for spool %s\n", toolhead_id,
                currentLoadedSpoolId.c_str());

  // Fire Webhook synchronously (simple implementation, can be async later)
  NetworkManager::sendWebhookPayload(currentLoadedSpoolId, toolhead_id);
#else
  printf("User selected Tool %d for spool %s\n", toolhead_id,
         currentLoadedSpoolId.c_str());
#endif

  // Return to the Info screen after loading
  lv_scr_load(infoScreen);
}

void DisplayUI::onEditButtonClicked(lv_event_t *e) { showEditScreen(); }

void DisplayUI::onSaveButtonClicked(lv_event_t *e) {
  // Triggers write to NFC
}

void DisplayUI::onBackButtonClicked(lv_event_t *e) { showScanScreen(); }
