#include "DisplayUI.h"
#ifndef USE_SDL2
#include "../config/ConfigManager.h"
#include "../network/NetworkManager.h"
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
  static lv_color_t buf[screenWidth * 10]; // 10 lines buffer
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
  auto set_dark = [](lv_obj_t *scr) {
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
  };
  set_dark(scanScreen);
  set_dark(infoScreen);
  set_dark(toolSelectionScreen);
  set_dark(editScreen);

  // Show initial screen
  showScanScreen();
}

void DisplayUI::tick() {
  lv_timer_handler(); // let the GUI do its work
}

// ... [Screen building logic will be implemented here] ...

void DisplayUI::buildScanScreen() {
  scanScreen = lv_obj_create(NULL);
  lv_obj_set_scroll_dir(scanScreen, LV_DIR_NONE);

  lv_obj_t *header = lv_label_create(scanScreen);
  lv_label_set_text(header, "Ready to Scan");
  lv_obj_set_style_text_color(header, lv_color_white(), 0);
  lv_obj_align(header, LV_ALIGN_CENTER, 0, 30);

  lv_obj_t *desc = lv_label_create(scanScreen);
  lv_label_set_text(desc, "Please hold tag to Reader");
  lv_obj_set_style_text_color(desc, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
  lv_obj_align(desc, LV_ALIGN_CENTER, 0, 85);

  // OpenSpool Logo
  lv_obj_t *logo = lv_image_create(scanScreen);
  lv_image_set_src(logo, &img_openspool_logo);
  lv_obj_align(logo, LV_ALIGN_CENTER, 0, -40);
}

void DisplayUI::buildInfoScreen() {
  infoScreen = lv_obj_create(NULL);
  lv_obj_set_scroll_dir(infoScreen, LV_DIR_NONE);

  // Header
  lv_obj_t *header = lv_label_create(infoScreen);
  lv_label_set_text(header, "Spool Information");
  lv_obj_set_style_text_color(header, lv_palette_main(LV_PALETTE_AMBER), 0);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 15);

  // Content container (table-like structure)
  lv_obj_t *cont = lv_obj_create(infoScreen);
  lv_obj_set_size(cont, 220, 160);
  lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 45);
  lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_set_style_bg_opa(cont, 30, 0); // Subtle background
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 10, 0);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

  // Helper function to create a "row"
  auto create_row = [&](int y, const char *key, lv_obj_t **val_label) {
    lv_obj_t *k = lv_label_create(cont);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
    lv_obj_align(k, LV_ALIGN_TOP_LEFT, 0, y);

    *val_label = lv_label_create(cont);
    lv_label_set_text(*val_label, "?");
    lv_obj_set_style_text_color(*val_label, lv_color_white(), 0);
    lv_obj_align(*val_label, LV_ALIGN_TOP_LEFT, 70, y);
  };

  create_row(0, "Brand:", &labelBrand);
  create_row(30, "Type:", &labelType);
  create_row(60, "ID:", &labelSpoolId);

  // Color row
  lv_obj_t *cLabel = lv_label_create(cont);
  lv_label_set_text(cLabel, "Color:");
  lv_obj_set_style_text_color(cLabel, lv_palette_lighten(LV_PALETTE_GREY, 2),
                              0);
  lv_obj_align(cLabel, LV_ALIGN_TOP_LEFT, 0, 90);

  colorBox = lv_obj_create(cont);
  lv_obj_set_size(colorBox, 60, 40);
  lv_obj_align(colorBox, LV_ALIGN_TOP_LEFT, 70, 90);
  lv_obj_set_style_border_color(colorBox, lv_color_white(), 0);
  lv_obj_set_style_border_width(colorBox, 1, 0);

  labelColorHex = lv_label_create(cont);
  lv_label_set_text(labelColorHex, "#??????");
  lv_obj_set_style_text_color(labelColorHex, lv_color_white(), 0);
  lv_obj_align(labelColorHex, LV_ALIGN_TOP_LEFT, 140, 100);

  // Buttons row - Vertical stack for better ergonomics in portrait
  lv_obj_t *btnCont = lv_obj_create(infoScreen);
  lv_obj_set_size(btnCont, 220, 110);
  lv_obj_align(btnCont, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_opa(btnCont, 0, 0);
  lv_obj_set_style_border_width(btnCont, 0, 0);
  lv_obj_set_scrollbar_mode(btnCont, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_flex_flow(btnCont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(btnCont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(btnCont, 10, 0);

  lv_obj_t *loadBtn = lv_btn_create(btnCont);
  lv_obj_set_size(loadBtn, 180, 45);
  lv_obj_add_event_cb(loadBtn, onLoadSpoolButtonClicked, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_t *loadLbl = lv_label_create(loadBtn);
  lv_label_set_text(loadLbl, "Load Spool");
  lv_obj_center(loadLbl);

  lv_obj_t *bottomBtns = lv_obj_create(btnCont);
  lv_obj_set_size(bottomBtns, 180, 45);
  lv_obj_set_style_bg_opa(bottomBtns, 0, 0);
  lv_obj_set_style_border_width(bottomBtns, 0, 0);
  lv_obj_set_style_pad_all(bottomBtns, 0, 0);
  lv_obj_set_flex_flow(bottomBtns, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bottomBtns, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *backBtn = lv_btn_create(bottomBtns);
  lv_obj_set_size(backBtn, 85, 45);
  lv_obj_add_event_cb(backBtn, onBackButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *backLbl = lv_label_create(backBtn);
  lv_label_set_text(backLbl, "Back");
  lv_obj_center(backLbl);

  lv_obj_t *editBtn = lv_btn_create(bottomBtns);
  lv_obj_set_size(editBtn, 85, 45);
  lv_obj_add_event_cb(editBtn, onEditButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *editLbl = lv_label_create(editBtn);
  lv_label_set_text(editLbl, "Edit");
  lv_obj_center(editLbl);
}

void DisplayUI::buildToolSelectionScreen() {
  toolSelectionScreen = lv_obj_create(NULL);

  lv_obj_t *header = lv_label_create(toolSelectionScreen);
  lv_label_set_text(header, "Select Tool");
  lv_obj_set_style_text_color(header, lv_color_white(), 0);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 15);

  // Grid for 4 toolheads
  lv_obj_t *grid = lv_obj_create(toolSelectionScreen);
  lv_obj_set_size(grid, 220, 200);
  lv_obj_align(grid, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_opa(grid, 0, 0);
  lv_obj_set_style_border_width(grid, 0, 0);
  lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_OFF);

  for (int i = 0; i < 4; i++) {
    lv_obj_t *tBtn = lv_btn_create(grid);
    lv_obj_set_size(tBtn, 90, 80);

    int col = i % 2;
    int row = i / 2;
    int x_offset = col == 0 ? -50 : 50;
    int y_offset = row == 0 ? -45 : 45;

    lv_obj_align(tBtn, LV_ALIGN_CENTER, x_offset, y_offset);

    // Pass the tool ID as the long form user_data
    lv_obj_add_event_cb(tBtn, onToolButtonClicked, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);

    lv_obj_t *tLbl = lv_label_create(tBtn);
    lv_label_set_text_fmt(tLbl, "Tool %d", i);
    lv_obj_center(tLbl);
  }

  lv_obj_t *backBtn = lv_btn_create(toolSelectionScreen);
  lv_obj_align(backBtn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  // Reusing back button to simply return to scan screen for now
  lv_obj_add_event_cb(backBtn, onBackButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *backLbl = lv_label_create(backBtn);
  lv_label_set_text(backLbl, "Cancel");
}

void DisplayUI::buildEditScreen() {
  editScreen = lv_obj_create(NULL);
  // Similar structure to info screen, but with dropdowns/inputs
  // We will expand this as needed
  lv_obj_t *label = lv_label_create(editScreen);
  lv_label_set_text(label, "Edit Screen (Coming Soon)");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *backBtn = lv_btn_create(editScreen);
  lv_obj_align(backBtn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  lv_obj_add_event_cb(backBtn, onBackButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *backLbl = lv_label_create(backBtn);
  lv_label_set_text(backLbl, "Cancel");
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

  lv_scr_load(infoScreen);
}

void DisplayUI::showToolSelectionScreen() { lv_scr_load(toolSelectionScreen); }

void DisplayUI::showEditScreen() { lv_scr_load(editScreen); }

void DisplayUI::onLoadSpoolButtonClicked(lv_event_t *e) {
  // Only show tool selection if Webhook is actually configured
#ifndef USE_SDL2
  if (ConfigManager::getWebhook().length() > 0) {
    showToolSelectionScreen();
  } else {
    Serial.println("No Webhook URL configured. Load Spool action disabled.");
  }
#else
  showToolSelectionScreen();
#endif
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

  // Return to main scanning screen after loading
  showScanScreen();
}

void DisplayUI::onEditButtonClicked(lv_event_t *e) { showEditScreen(); }

void DisplayUI::onSaveButtonClicked(lv_event_t *e) {
  // Triggers write to NFC
}

void DisplayUI::onBackButtonClicked(lv_event_t *e) { showScanScreen(); }
