#include "DisplayUI.h"
#include "../config/ConfigManager.h"
#include "../network/NetworkManager.h"
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Screen dimensions
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

// Hardware drivers
static TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight);
XPT2046_Touchscreen ts(TOUCH_CS);

// LVGL static objects
lv_obj_t *DisplayUI::scanScreen = nullptr;
lv_obj_t *DisplayUI::infoScreen = nullptr;
lv_obj_t *DisplayUI::toolSelectionScreen = nullptr;
lv_obj_t *DisplayUI::editScreen = nullptr;

lv_obj_t *DisplayUI::labelBrand = nullptr;
lv_obj_t *DisplayUI::labelType = nullptr;
lv_obj_t *DisplayUI::colorBox = nullptr;
lv_obj_t *DisplayUI::labelSpoolId = nullptr;

// We need to keep track of the spool ID locally for the webhook
static std::string currentLoadedSpoolId = "";

/* Display flushing */
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

void DisplayUI::init() {
  // Init TFT
  tft.begin();
  tft.setRotation(1); // Landscape
  tft.fillScreen(TFT_BLACK);

  // Init Touch
  ts.begin();
  ts.setRotation(1);

  // Init LVGL
  lv_init();

  static lv_color_t buf[screenWidth * 10]; // 10 lines buffer
  lv_display_t *disp = lv_display_create(screenWidth, screenHeight);
  lv_display_set_buffers(disp, buf, NULL, sizeof(buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  // Build screens
  buildScanScreen();
  buildInfoScreen();
  buildToolSelectionScreen();
  buildEditScreen();

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

  lv_obj_t *label = lv_label_create(scanScreen);
  lv_label_set_text(label, "Ready to Scan\nPlease hold tag to Reader");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *spinner = lv_spinner_create(scanScreen);
  lv_obj_set_size(spinner, 60, 60);
  lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -50);
}

void DisplayUI::buildInfoScreen() {
  infoScreen = lv_obj_create(NULL);
  lv_obj_set_scroll_dir(infoScreen, LV_DIR_NONE);

  // Header
  lv_obj_t *header = lv_label_create(infoScreen);
  lv_label_set_text(header, "Spool Information");
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

  // Content container
  lv_obj_t *cont = lv_obj_create(infoScreen);
  lv_obj_set_size(cont, 280, 150);
  lv_obj_align(cont, LV_ALIGN_CENTER, 0, -10);

  labelBrand = lv_label_create(cont);
  lv_label_set_text(labelBrand, "Brand: ?");
  lv_obj_align(labelBrand, LV_ALIGN_TOP_LEFT, 5, 5);

  labelType = lv_label_create(cont);
  lv_label_set_text(labelType, "Type: ?");
  lv_obj_align(labelType, LV_ALIGN_TOP_LEFT, 5, 30);

  labelSpoolId = lv_label_create(cont);
  lv_label_set_text(labelSpoolId, "ID: ?");
  lv_obj_align(labelSpoolId, LV_ALIGN_TOP_LEFT, 5, 55);

  lv_obj_t *colorLabel = lv_label_create(cont);
  lv_label_set_text(colorLabel, "Color:");
  lv_obj_align(colorLabel, LV_ALIGN_TOP_LEFT, 5, 80);

  colorBox = lv_obj_create(cont);
  lv_obj_set_size(colorBox, 40, 40);
  lv_obj_align(colorBox, LV_ALIGN_TOP_LEFT, 60, 75);
  // Note: color will be set dynamically

  // Buttons row
  lv_obj_t *btnCont = lv_obj_create(infoScreen);
  lv_obj_set_size(btnCont, 300, 60);
  lv_obj_align(btnCont, LV_ALIGN_BOTTOM_MID, 0, -5);
  lv_obj_set_style_bg_opa(btnCont, 0, 0); // Transparent
  lv_obj_set_style_border_width(btnCont, 0, 0);

  lv_obj_t *loadBtn = lv_btn_create(btnCont);
  lv_obj_align(loadBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(loadBtn, onLoadSpoolButtonClicked, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_t *loadLbl = lv_label_create(loadBtn);
  lv_label_set_text(loadLbl, "Load Spool");

  lv_obj_t *editBtn = lv_btn_create(btnCont);
  lv_obj_align(editBtn, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(editBtn, onEditButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *editLbl = lv_label_create(editBtn);
  lv_label_set_text(editLbl, "Edit");

  lv_obj_t *backBtn = lv_btn_create(btnCont);
  lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(backBtn, onBackButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *backLbl = lv_label_create(backBtn);
  lv_label_set_text(backLbl, "Back");
}

void DisplayUI::buildToolSelectionScreen() {
  toolSelectionScreen = lv_obj_create(NULL);

  lv_obj_t *header = lv_label_create(toolSelectionScreen);
  lv_label_set_text(header, "Select Printer Toolhead");
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

  // Grid for 4 toolheads
  lv_obj_t *grid = lv_obj_create(toolSelectionScreen);
  lv_obj_set_size(grid, 280, 140);
  lv_obj_align(grid, LV_ALIGN_CENTER, 0, -10);
  lv_obj_set_style_bg_opa(grid, 0, 0);
  lv_obj_set_style_border_width(grid, 0, 0);

  for (int i = 0; i < 4; i++) {
    lv_obj_t *tBtn = lv_btn_create(grid);
    lv_obj_set_size(tBtn, 120, 50);

    int col = i % 2;
    int row = i / 2;
    int x_offset = col == 0 ? -65 : 65;
    int y_offset = row == 0 ? -30 : 30;

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
  lv_label_set_text_fmt(labelBrand, "Brand: %s", brand);
  lv_label_set_text_fmt(labelType, "Type: %s", type);
  lv_label_set_text_fmt(labelSpoolId, "ID: %s", spool_id);

  // Convert hex string to lv_color_t
  // We assume color_hex is like "#FF0000"
  if (color_hex && color_hex[0] == '#' && strlen(color_hex) == 7) {
    uint32_t color_val = strtol(&color_hex[1], NULL, 16);
    lv_obj_set_style_bg_color(colorBox, lv_color_hex(color_val), 0);
  }

  // Cache spool ID globally for webhook
  currentLoadedSpoolId = spool_id;

  lv_scr_load(infoScreen);
}

void DisplayUI::showToolSelectionScreen() { lv_scr_load(toolSelectionScreen); }

void DisplayUI::showEditScreen() { lv_scr_load(editScreen); }

void DisplayUI::onLoadSpoolButtonClicked(lv_event_t *e) {
  // Only show tool selection if Webhook is actually configured
  if (ConfigManager::getWebhookUrl().length() > 0) {
    showToolSelectionScreen();
  } else {
    Serial.println("No Webhook URL configured. Load Spool action disabled.");
  }
}

void DisplayUI::onToolButtonClicked(lv_event_t *e) {
  int toolhead_id = (int)(intptr_t)lv_event_get_user_data(e);
  Serial.printf("User selected Tool %d for spool %s\n", toolhead_id,
                currentLoadedSpoolId.c_str());

  // Fire Webhook synchronously (simple implementation, can be async later)
  NetworkManager::sendWebhookPayload(currentLoadedSpoolId, toolhead_id);

  // Return to main scanning screen after loading
  showScanScreen();
}

void DisplayUI::onEditButtonClicked(lv_event_t *e) { showEditScreen(); }

void DisplayUI::onSaveButtonClicked(lv_event_t *e) {
  // Triggers write to NFC
}

void DisplayUI::onBackButtonClicked(lv_event_t *e) { showScanScreen(); }
