#include "DisplayUI.h"
#include "../config/ConfigManager.h"
#include "../network/NetworkManager.h"

#ifndef USE_SDL2
#include "../nfc/NFCReader.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Bitbang.h>
#else
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <string>
#endif

// External assets
extern "C" {
extern const lv_image_dsc_t img_openspool_logo;
LV_FONT_DECLARE(lv_font_german_14);
LV_FONT_DECLARE(lv_font_german_20);
LV_FONT_DECLARE(lv_font_montserrat_14);
}

// Combined fonts pointers
static const lv_font_t *font_combined_14;
static const lv_font_t *font_combined_20;

static void setup_combined_fonts() {
  // Use our generated German fonts directly (contains 0x20-0x7F, 0xA0-0xFF)
  font_combined_14 = &lv_font_german_14;
  font_combined_20 = &lv_font_german_20;
}

// Screen dimensions
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 320;

#ifndef USE_SDL2
// Hardware drivers
static TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight);
XPT2046_Bitbang ts(TOUCH_MOSI, TOUCH_MISO, TOUCH_CLK, TOUCH_CS);
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
lv_obj_t *DisplayUI::labelSubtype = nullptr;
lv_obj_t *DisplayUI::labelLotNr = nullptr;
lv_obj_t *DisplayUI::keyLotNr = nullptr;
lv_obj_t *DisplayUI::labelTemp = nullptr;
lv_obj_t *DisplayUI::labelBedTemp = nullptr;
lv_obj_t *DisplayUI::labelColorHex = nullptr;
lv_obj_t *DisplayUI::loadBtn = nullptr;
lv_obj_t *DisplayUI::createNewBtn = nullptr;

lv_obj_t *DisplayUI::editBrandDropdown = nullptr;
lv_obj_t *DisplayUI::editCustomBrandRow = nullptr;
lv_obj_t *DisplayUI::editCustomBrandTextArea = nullptr;
lv_obj_t *DisplayUI::editTypeDropdown = nullptr;
lv_obj_t *DisplayUI::editColorHexTextArea = nullptr;
lv_obj_t *DisplayUI::editColorPreview = nullptr;
lv_obj_t *DisplayUI::editSaveBtn = nullptr;
lv_obj_t *DisplayUI::editSpoolIdTextArea = nullptr;
lv_obj_t *DisplayUI::editLotNrTextArea = nullptr;
lv_obj_t *DisplayUI::editMinTempTextArea = nullptr;
lv_obj_t *DisplayUI::editMaxTempTextArea = nullptr;
lv_obj_t *DisplayUI::editBedMinTextArea = nullptr;
lv_obj_t *DisplayUI::editBedMaxTextArea = nullptr;
lv_obj_t *DisplayUI::keyboard = nullptr;
lv_obj_t *DisplayUI::writingOverlay = nullptr;
lv_obj_t *DisplayUI::fetchingOverlay = nullptr;
lv_obj_t *DisplayUI::spoolIdPromptScreen = nullptr;
lv_obj_t *DisplayUI::promptSpoolIdTextArea = nullptr;
lv_obj_t *DisplayUI::promptLoadBtn = nullptr;
bool DisplayUI::writePending = false;
uint32_t DisplayUI::writeStartTime = 0;
lv_obj_t *DisplayUI::toastObj = nullptr;
lv_timer_t *DisplayUI::toastTimer = nullptr;

lv_obj_t *DisplayUI::keyFilament = nullptr;
lv_obj_t *DisplayUI::labelFilamentName = nullptr;
lv_obj_t *DisplayUI::keyWeight = nullptr;
lv_obj_t *DisplayUI::labelWeight = nullptr;
lv_obj_t *DisplayUI::toolGrid = nullptr;

// Static member initialization
OpenSpoolData DisplayUI::currentLoadedData;

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
  TouchPoint p = ts.getTouch();
  if (p.zRaw > 0) {
    // CYD typical portrait mapping:
    // X and Y are swapped.
    // X axis comes from yRaw
    // Y axis comes from xRaw
    data->point.x = map(p.yRaw, 3800, 240, 0, screenWidth);
    data->point.y = map(p.xRaw, 200, 3700, 0, screenHeight);

    // Clamp to screen bounds
    if (data->point.x < 0)
      data->point.x = 0;
    if (data->point.x >= screenWidth)
      data->point.x = screenWidth - 1;
    if (data->point.y < 0)
      data->point.y = 0;
    if (data->point.y >= screenHeight)
      data->point.y = screenHeight - 1;

    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}
#endif

#ifndef USE_SDL2
/* Tick providing for LVGL */
static uint32_t my_tick_get_cb(void) { return millis(); }
#else
extern uint32_t millis(void);
static uint32_t my_tick_get_cb(void) { return millis(); }
#endif

void DisplayUI::init() {
#ifndef USE_SDL2
  tft.begin();
  tft.invertDisplay(true);
  tft.setRotation(0); // Portrait
  tft.fillScreen(TFT_BLACK);

  // Init Touch
  ts.begin();
#endif

  // Init LVGL
  lv_init();
  lv_tick_set_cb(my_tick_get_cb);

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
  setup_combined_fonts();
  buildScanScreen();
  buildInfoScreen();
  buildToolSelectionScreen();
  buildEditScreen();
  buildSpoolIdPromptScreen();
  buildWritingOverlay();
  buildFetchingOverlay();

  // Set default dark background for all screens and hide scrollbars
  auto set_premium_scr = [](lv_obj_t *scr) {
    if (!scr)
      return;
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0f1118), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(scr, font_combined_14, 0);
  };
  set_premium_scr(scanScreen);
  set_premium_scr(infoScreen);
  set_premium_scr(toolSelectionScreen);
  set_premium_scr(editScreen);

  // Show initial screen
  showScanScreen();
}

void DisplayUI::tick() {
  lv_timer_handler(); // let the GUI do its work
}

uint32_t DisplayUI::getLastInteractionTime() {
  lv_display_t *disp = lv_display_get_default();
  if (!disp)
    return 0;
  uint32_t inactive = lv_display_get_inactive_time(disp);
  uint32_t now = millis();
  if (inactive > now)
    return 0;
  return now - inactive;
}

/* --- Premium Helpers --- */
static void apply_glass_style(lv_obj_t *obj) {
  lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_10, 0);
  lv_obj_set_style_border_color(obj, lv_color_white(), 0);
  lv_obj_set_style_border_opa(obj, LV_OPA_20, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_radius(obj, 16, 0);
  lv_obj_set_style_shadow_width(obj, 0, 0);
  lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

static void apply_indigo_btn_style(lv_obj_t *obj) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x5266ff), 0);
  lv_obj_set_style_radius(obj, 12, 0);
  lv_obj_set_style_shadow_width(obj, 0, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
}

void DisplayUI::buildScanScreen() {
  scanScreen = lv_obj_create(NULL);
  lv_obj_set_scroll_dir(scanScreen, LV_DIR_NONE);

  // OpenSpool Logo with glow effect
  lv_obj_t *logo = lv_image_create(scanScreen);
  lv_image_set_src(logo, &img_openspool_logo);
  lv_obj_align(logo, LV_ALIGN_CENTER, 0, -50);

  lv_obj_t *header = lv_label_create(scanScreen);
  lv_label_set_text(header, "Ready to Scan");
  lv_obj_set_style_text_font(header, font_combined_20, 0);
  lv_obj_set_style_text_color(header, lv_color_white(), 0);
  lv_obj_align(header, LV_ALIGN_CENTER, 0, 40);

  lv_obj_t *desc = lv_label_create(scanScreen);
  lv_label_set_text(desc, "Hold reader over tag");
  lv_obj_set_style_text_color(desc, lv_color_hex(0x9ca3af), 0);
  lv_obj_align(desc, LV_ALIGN_CENTER, 0, 75);

  createNewBtn = lv_btn_create(scanScreen);
  lv_obj_set_size(createNewBtn, 160, 42);
  lv_obj_align(createNewBtn, LV_ALIGN_BOTTOM_MID, 0, -20);
  apply_indigo_btn_style(createNewBtn);
  lv_obj_add_event_cb(createNewBtn, onCreateNewButtonClicked, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_t *createLbl = lv_label_create(createNewBtn);
  lv_label_set_text(createLbl, "Create New Tag");
  lv_obj_center(createLbl);
}

void DisplayUI::buildInfoScreen() {
  infoScreen = lv_obj_create(NULL);
  lv_obj_set_scroll_dir(infoScreen, LV_DIR_NONE);

  // Glass Card Content
  lv_obj_t *card = lv_obj_create(infoScreen);
  lv_obj_set_size(card, 220, 205);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 10);
  apply_glass_style(card);
  lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_ACTIVE);
  lv_obj_set_scroll_dir(card, LV_DIR_VER);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_set_style_pad_row(card, 8, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);

  auto create_row = [&](const char *key, lv_obj_t **val_label,
                        lv_obj_t **key_label = nullptr) {
    lv_obj_t *row_cont = lv_obj_create(card);
    lv_obj_set_size(row_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_cont, 0, 0);
    lv_obj_set_style_border_width(row_cont, 0, 0);
    lv_obj_set_style_pad_all(row_cont, 0, 0);
    lv_obj_clear_flag(row_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *k = lv_label_create(row_cont);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, lv_color_hex(0x9ca3af), 0);
    lv_obj_align(k, LV_ALIGN_TOP_LEFT, 0, 0);
    if (key_label)
      *key_label = row_cont;

    *val_label = lv_label_create(row_cont);
    lv_label_set_text(*val_label, "---");
    lv_obj_set_style_text_color(*val_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(*val_label, font_combined_14, 0);
    lv_obj_align(*val_label, LV_ALIGN_TOP_LEFT, 80, 0);
  };

  create_row("Brand", &labelBrand);
  create_row("Filament", &labelFilamentName, &keyFilament);
  create_row("Material", &labelType);

  // Color row
  lv_obj_t *color_row = lv_obj_create(card);
  lv_obj_set_size(color_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(color_row, 0, 0);
  lv_obj_set_style_border_width(color_row, 0, 0);
  lv_obj_set_style_pad_all(color_row, 0, 0);
  lv_obj_clear_flag(color_row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *cLabel = lv_label_create(color_row);
  lv_label_set_text(cLabel, "Color");
  lv_obj_set_style_text_color(cLabel, lv_color_hex(0x9ca3af), 0);
  lv_obj_align(cLabel, LV_ALIGN_TOP_LEFT, 0, 3);

  colorBox = lv_obj_create(color_row);
  lv_obj_clear_flag(colorBox, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(colorBox, 90, 22);
  lv_obj_align(colorBox, LV_ALIGN_TOP_LEFT, 80, 0);
  lv_obj_set_style_radius(colorBox, 6, 0);
  lv_obj_set_style_border_width(colorBox, 1, 0);
  lv_obj_set_style_border_color(colorBox, lv_color_white(), 0);
  lv_obj_set_style_border_opa(colorBox, LV_OPA_40, 0);

  lv_obj_t *hex_row = lv_obj_create(card);
  lv_obj_set_size(hex_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(hex_row, 0, 0);
  lv_obj_set_style_border_width(hex_row, 0, 0);
  lv_obj_set_style_pad_all(hex_row, 0, 0);
  lv_obj_clear_flag(hex_row, LV_OBJ_FLAG_SCROLLABLE);

  labelColorHex = lv_label_create(hex_row);
  lv_label_set_text(labelColorHex, "#------");
  lv_obj_set_style_text_font(labelColorHex, font_combined_14, 0);
  lv_obj_set_style_text_color(labelColorHex, lv_color_white(), 0);
  lv_obj_align(labelColorHex, LV_ALIGN_TOP_LEFT, 80, 0);

  create_row("Weight", &labelWeight, &keyWeight);
  create_row("Spool ID", &labelSpoolId);
  create_row("Subtype", &labelSubtype);
  create_row("Nozzle T.", &labelTemp);
  create_row("Bed T.", &labelBedTemp);
  create_row("Lot Nr", &labelLotNr, &keyLotNr);

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
  lv_obj_set_style_text_font(header, font_combined_20, 0);
  lv_obj_set_style_text_color(header, lv_color_white(), 0);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 15);

  toolGrid = lv_obj_create(toolSelectionScreen);
  lv_obj_set_size(toolGrid, 220, 200);
  lv_obj_align(toolGrid, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_bg_opa(toolGrid, 0, 0);
  lv_obj_set_style_border_width(toolGrid, 0, 0);
  lv_obj_set_scroll_dir(toolGrid, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(toolGrid, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_remove_flag(toolGrid, LV_OBJ_FLAG_SCROLL_ELASTIC);

  // Placeholder - buttons are built dynamically in showToolSelectionScreen()

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
  lv_label_set_text(header, "Edit Spool Data");
  lv_obj_set_style_text_font(header, font_combined_20, 0);
  lv_obj_set_style_text_color(header, lv_color_white(), 0);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_remove_flag(editScreen, LV_OBJ_FLAG_SCROLLABLE);

  // Scrollable container for form (Main content area)
  lv_obj_t *cont = lv_obj_create(editScreen);
  lv_obj_set_size(cont, 240, 220); // Width back to 240, Height to fit between header and footer
  lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_set_style_bg_opa(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_all(cont, 5, 0);
  lv_obj_set_style_pad_row(cont, 4, 0); // Reduced gap for more compactness

  auto create_label = [&](lv_obj_t *parent, const char *txt) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, lv_color_hex(0x9ca3af), 0);
    lv_obj_set_style_margin_top(l, 5, 0);
    return l;
  };

  create_label(cont, "Brand");
  editBrandDropdown = lv_dropdown_create(cont);
  lv_obj_set_width(editBrandDropdown, LV_PCT(95));
  lv_dropdown_set_options(
      editBrandDropdown,
      "Generic\nBambu Lab\nHatchbox\neSun\nOverture\nSUNLU\nPolymaker\n"
      "Prusament\nSnapmaker\nJayo\nDas Filament\nRecyclingFabrik\nCustom");
  lv_obj_set_style_text_font(editBrandDropdown, &lv_font_montserrat_14,
                             LV_PART_INDICATOR);
  lv_obj_add_event_cb(editBrandDropdown, onBrandDropdownChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  editCustomBrandRow = lv_obj_create(cont);
  lv_obj_set_size(editCustomBrandRow, LV_PCT(95), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(editCustomBrandRow, 0, 0);
  lv_obj_set_style_border_width(editCustomBrandRow, 0, 0);
  lv_obj_set_style_pad_all(editCustomBrandRow, 0, 0);
  lv_obj_set_flex_flow(editCustomBrandRow, LV_FLEX_FLOW_COLUMN);
  lv_obj_add_flag(editCustomBrandRow, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *customBrandLabel =
      create_label(editCustomBrandRow, "Custom Brand Name");
  editCustomBrandTextArea = lv_textarea_create(editCustomBrandRow);
  lv_obj_set_width(editCustomBrandTextArea, LV_PCT(100));
  lv_textarea_set_one_line(editCustomBrandTextArea, true);
  lv_obj_add_event_cb(editCustomBrandTextArea, onTextAreaFocused,
                      LV_EVENT_FOCUSED, NULL);
  lv_obj_add_event_cb(editCustomBrandTextArea, onTextAreaChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  create_label(cont, "Material Type");
  editTypeDropdown = lv_dropdown_create(cont);
  lv_obj_set_width(editTypeDropdown, LV_PCT(95));
  lv_dropdown_set_options(editTypeDropdown,
                          "PLA\nPETG\nABS\nASA\nTPU\nPA\nPA12\nPC\nPEEK\nPVA\n"
                          "HIPS\nPCTG\nPLA-CF\nPETG-CF\nPA-CF");
  lv_obj_set_style_text_font(editTypeDropdown, &lv_font_montserrat_14,
                             LV_PART_INDICATOR);
  create_label(cont, "Hex Color (#RRGGBB)");
  lv_obj_t *hexRow = lv_obj_create(cont);
  lv_obj_set_size(hexRow, LV_PCT(95), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(hexRow, 0, 0);
  lv_obj_set_style_border_width(hexRow, 0, 0);
  lv_obj_set_style_pad_all(hexRow, 0, 0);
  lv_obj_set_flex_flow(hexRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(hexRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_t *prefix = lv_label_create(hexRow);
  lv_label_set_text(prefix, "#");
  lv_obj_set_style_text_color(prefix, lv_color_hex(0x9ca3af), 0);
  lv_obj_set_style_pad_right(prefix, 5, 0);

  editColorHexTextArea = lv_textarea_create(hexRow);
  lv_obj_set_flex_grow(editColorHexTextArea, 1);
  lv_textarea_set_one_line(editColorHexTextArea, true);
  lv_textarea_set_max_length(editColorHexTextArea, 6);
  lv_obj_add_event_cb(editColorHexTextArea, onTextAreaFocused, LV_EVENT_FOCUSED,
                      NULL);
  lv_obj_add_event_cb(editColorHexTextArea, onColorHexChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  editColorPreview = lv_obj_create(hexRow);
  lv_obj_set_size(editColorPreview, 40, 40);
  lv_obj_set_style_radius(editColorPreview, 5, 0);
  lv_obj_set_style_border_width(editColorPreview, 1, 0);
  lv_obj_set_style_border_color(editColorPreview, lv_color_hex(0x374151), 0);
  lv_obj_set_style_bg_color(editColorPreview, lv_color_hex(0xFFFFFF), 0);

  create_label(cont, "Spool ID");
  editSpoolIdTextArea = lv_textarea_create(cont);
  lv_obj_set_width(editSpoolIdTextArea, LV_PCT(95));
  lv_textarea_set_one_line(editSpoolIdTextArea, true);
  lv_obj_add_event_cb(editSpoolIdTextArea, onTextAreaFocused, LV_EVENT_FOCUSED,
                      NULL);
  lv_obj_add_event_cb(editSpoolIdTextArea, onTextAreaChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  create_label(cont, "Lot Number");
  editLotNrTextArea = lv_textarea_create(cont);
  lv_obj_set_width(editLotNrTextArea, LV_PCT(95));
  lv_textarea_set_one_line(editLotNrTextArea, true);
  lv_obj_add_event_cb(editLotNrTextArea, onTextAreaFocused, LV_EVENT_FOCUSED,
                      NULL);
  lv_obj_add_event_cb(editLotNrTextArea, onTextAreaChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  create_label(cont, "Nozzle Temp Min/Max");
  lv_obj_t *tempRow = lv_obj_create(cont);
  lv_obj_set_size(tempRow, LV_PCT(95), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(tempRow, 0, 0);
  lv_obj_set_style_border_width(tempRow, 0, 0);
  lv_obj_set_style_pad_all(tempRow, 0, 0);
  lv_obj_set_flex_flow(tempRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(tempRow, 10, 0);

  editMinTempTextArea = lv_textarea_create(tempRow);
  lv_obj_set_width(editMinTempTextArea, 70);
  lv_textarea_set_one_line(editMinTempTextArea, true);
  lv_obj_add_event_cb(editMinTempTextArea, onTextAreaFocused, LV_EVENT_FOCUSED,
                      NULL);
  lv_obj_add_event_cb(editMinTempTextArea, onTextAreaChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  editMaxTempTextArea = lv_textarea_create(tempRow);
  lv_obj_set_width(editMaxTempTextArea, 70);
  lv_textarea_set_one_line(editMaxTempTextArea, true);
  lv_obj_add_event_cb(editMaxTempTextArea, onTextAreaFocused, LV_EVENT_FOCUSED,
                      NULL);
  lv_obj_add_event_cb(editMaxTempTextArea, onTextAreaChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  create_label(cont, "Bed Temp Min/Max");
  lv_obj_t *bedTempRow = lv_obj_create(cont);
  lv_obj_set_size(bedTempRow, LV_PCT(95), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(bedTempRow, 0, 0);
  lv_obj_set_style_border_width(bedTempRow, 0, 0);
  lv_obj_set_style_pad_all(bedTempRow, 0, 0);
  lv_obj_set_flex_flow(bedTempRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(bedTempRow, 10, 0);

  editBedMinTextArea = lv_textarea_create(bedTempRow);
  lv_obj_set_width(editBedMinTextArea, 70);
  lv_textarea_set_one_line(editBedMinTextArea, true);
  lv_obj_add_event_cb(editBedMinTextArea, onTextAreaFocused, LV_EVENT_FOCUSED,
                      NULL);
  lv_obj_add_event_cb(editBedMinTextArea, onTextAreaChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  editBedMaxTextArea = lv_textarea_create(bedTempRow);
  lv_obj_set_width(editBedMaxTextArea, 70);
  lv_textarea_set_one_line(editBedMaxTextArea, true);
  lv_obj_add_event_cb(editBedMaxTextArea, onTextAreaFocused, LV_EVENT_FOCUSED,
                      NULL);
  lv_obj_add_event_cb(editBedMaxTextArea, onTextAreaChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  // Bottom Buttons (Fixed Footer)
  lv_obj_t *btnCont = lv_obj_create(editScreen);
  lv_obj_set_size(btnCont, 240, 50); // Width back to 240
  lv_obj_align(btnCont, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(btnCont, lv_color_hex(0x111827), 0);
  lv_obj_set_style_bg_opa(btnCont, LV_OPA_COVER, 0);
  lv_obj_set_style_border_side(btnCont, LV_BORDER_SIDE_NONE, 0);
  lv_obj_set_style_border_width(btnCont, 0, 0);
  lv_obj_set_style_border_color(btnCont, lv_color_hex(0x374151), 0);
  lv_obj_set_style_radius(btnCont, 0, 0);
  lv_obj_set_flex_flow(btnCont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btnCont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_gap(btnCont, 20, 0);
  lv_obj_remove_flag(btnCont, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *cancelBtn = lv_btn_create(btnCont);
  lv_obj_set_size(cancelBtn, 100, 38);
  apply_glass_style(cancelBtn);
  lv_obj_add_event_cb(cancelBtn, onCancelEditButtonClicked, LV_EVENT_CLICKED,
                      NULL);
  lv_obj_t *cancelLbl = lv_label_create(cancelBtn);
  lv_label_set_text(cancelLbl, "Cancel");
  lv_obj_center(cancelLbl);

  editSaveBtn = lv_btn_create(btnCont);
  lv_obj_set_size(editSaveBtn, 100, 38);
  apply_indigo_btn_style(editSaveBtn);
  lv_obj_add_event_cb(editSaveBtn, onSaveButtonClicked, LV_EVENT_CLICKED, NULL);
  lv_obj_t *saveLbl = lv_label_create(editSaveBtn);
  lv_label_set_text(saveLbl, "Save Tag");
  lv_obj_center(saveLbl);

  // Keyboard (initially hidden, on top layer to be shared)
  keyboard = lv_keyboard_create(lv_layer_top());
  lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(keyboard, onKeyboardEvent, LV_EVENT_ALL, NULL);
}

void DisplayUI::buildWritingOverlay() {
  writingOverlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(writingOverlay, screenWidth, screenHeight);
  lv_obj_set_style_bg_color(writingOverlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(writingOverlay, LV_OPA_70, 0);
  lv_obj_add_flag(writingOverlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(writingOverlay, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *spinner = lv_spinner_create(writingOverlay);
  lv_obj_set_size(spinner, 60, 60);
  lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -20);

  lv_obj_t *msg = lv_label_create(writingOverlay);
  lv_label_set_text(msg, "Writing to tag...\nPlease hold still");
  lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(msg, lv_color_white(), 0);
  lv_obj_align(msg, LV_ALIGN_CENTER, 0, 40);
}

void DisplayUI::showScanScreen() { lv_scr_load(scanScreen); }

void DisplayUI::showInfoScreen(const OpenSpoolData &spool) {
  OpenSpoolData displayData = spool;

#ifdef USE_SDL2
  // Simulator: If no spoolman data present, inject mock data for UI testing
  if (displayData.filament_name.empty()) {
    displayData.filament_name = "Mock Spoolman Filament";
    displayData.remaining_weight = "123.4g";
    displayData.total_weight = "1,000g";
  }
#endif

  lv_label_set_text(labelBrand, displayData.brand.c_str());
  lv_label_set_text(labelType, displayData.type.c_str());
  lv_label_set_text(labelSpoolId, spool.spool_id.c_str());
  lv_label_set_text(labelSubtype, spool.subtype.c_str());

  if (spool.lot_nr.empty()) {
    lv_obj_add_flag(labelLotNr, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(keyLotNr, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text(labelLotNr, spool.lot_nr.c_str());
    lv_obj_clear_flag(labelLotNr, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(keyLotNr, LV_OBJ_FLAG_HIDDEN);
  }

  std::string tempStr = spool.min_temp + " - " + spool.max_temp;
  if (tempStr == " - ")
    tempStr = "";
  lv_label_set_text(labelTemp, tempStr.c_str());

  std::string bedTempStr = spool.bed_min_temp + " - " + spool.bed_max_temp;
  if (bedTempStr == " - ")
    bedTempStr = "";
  lv_label_set_text(labelBedTemp, bedTempStr.c_str());

  // Spoolman enrichment updates
  if (spool.filament_name.empty()) {
    lv_obj_add_flag(labelFilamentName, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(keyFilament, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text(labelFilamentName, spool.filament_name.c_str());
    lv_obj_clear_flag(labelFilamentName, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(keyFilament, LV_OBJ_FLAG_HIDDEN);
  }

  if (spool.remaining_weight.empty() && spool.total_weight.empty()) {
    lv_obj_add_flag(labelWeight, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(keyWeight, LV_OBJ_FLAG_HIDDEN);
  } else {
    std::string weightCombined = spool.remaining_weight;
    if (!spool.total_weight.empty()) {
      if (!weightCombined.empty()) {
        weightCombined += " / ";
      }
      weightCombined += spool.total_weight;
    }
    lv_label_set_text(labelWeight, weightCombined.c_str());
    lv_obj_clear_flag(labelWeight, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(keyWeight, LV_OBJ_FLAG_HIDDEN);
  }

  // Convert hex string to lv_color_t
  // We assume color_hex is like "#FF0000"
  if (!spool.color_hex.empty() && spool.color_hex[0] == '#' &&
      spool.color_hex.length() == 7) {
    uint32_t color_val = strtol(&spool.color_hex[1], NULL, 16);
    lv_obj_set_style_bg_color(colorBox, lv_color_hex(color_val), 0);
    lv_label_set_text(labelColorHex, spool.color_hex.c_str());
  } else {
    lv_obj_set_style_bg_color(colorBox, lv_color_hex(0x000000), 0);
    lv_label_set_text(labelColorHex, "Unknown");
  }

  // Cache spool data globally for webhook/U1
  currentLoadedData = spool;

#ifndef USE_SDL2
  // Hide the Load button on physical device if no webhook or U1 is configured
  if (ConfigManager::getWebhook().empty() &&
      ConfigManager::getU1Host().empty()) {
    lv_obj_add_flag(loadBtn, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(loadBtn, LV_OBJ_FLAG_HIDDEN);
  }
#endif

  lv_scr_load(infoScreen);
}

void DisplayUI::showToolSelectionScreen() {
  // Always rebuild the grid to reflect latest num_tools config
  lv_obj_clean(toolGrid);

  uint8_t num_tools = ConfigManager::getNumTools();
  if (num_tools <= 4) {
    lv_obj_set_scrollbar_mode(toolGrid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(toolGrid, LV_OBJ_FLAG_SCROLLABLE);
  } else {
    lv_obj_set_scrollbar_mode(toolGrid, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(toolGrid, LV_OBJ_FLAG_SCROLLABLE);
  }

  for (int i = 0; i < num_tools; i++) {
    lv_obj_t *tBtn = lv_btn_create(toolGrid);
    lv_obj_set_size(tBtn, 95, 85);

    int col = i % 2;
    int row = i / 2;
    lv_obj_align(tBtn, LV_ALIGN_TOP_LEFT, col * 105, row * 95);
    apply_indigo_btn_style(tBtn);

    lv_obj_add_event_cb(tBtn, onToolButtonClicked, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);

    lv_obj_t *tLbl = lv_label_create(tBtn);
    lv_label_set_text_fmt(tLbl, "T %d", i);
    lv_obj_set_style_text_font(tLbl, font_combined_20, 0);
    lv_obj_center(tLbl);
  }

  lv_scr_load(toolSelectionScreen);
}

void DisplayUI::buildSpoolIdPromptScreen() {
  spoolIdPromptScreen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(spoolIdPromptScreen,
                            lv_palette_main(LV_PALETTE_GREY), 0);

  lv_obj_t *cont = lv_obj_create(spoolIdPromptScreen);
  lv_obj_set_size(cont, 280, 180);
  lv_obj_center(cont);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, "Load from Spoolman:");

  promptSpoolIdTextArea = lv_textarea_create(cont);
  lv_textarea_set_one_line(promptSpoolIdTextArea, true);
  lv_obj_set_width(promptSpoolIdTextArea, 200);
  lv_textarea_set_placeholder_text(promptSpoolIdTextArea, "ID");
  lv_obj_add_event_cb(promptSpoolIdTextArea, onTextAreaFocused,
                      LV_EVENT_FOCUSED, NULL);
  lv_obj_add_event_cb(promptSpoolIdTextArea, onTextAreaFocused,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(promptSpoolIdTextArea, onTextAreaChanged,
                      LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t *btn_cont = lv_obj_create(cont);
  lv_obj_set_size(btn_cont, 240, 60);
  lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(btn_cont, 0, 0);
  lv_obj_set_style_border_width(btn_cont, 0, 0);
  lv_obj_set_style_bg_opa(btn_cont, 0, 0);

  promptLoadBtn = lv_button_create(btn_cont);
  lv_obj_t *load_lbl = lv_label_create(promptLoadBtn);
  lv_label_set_text(load_lbl, "Load");
  lv_obj_add_event_cb(promptLoadBtn, onLoadPrefilledButtonClicked,
                      LV_EVENT_CLICKED, NULL);
  lv_obj_add_state(promptLoadBtn, LV_STATE_DISABLED);

  lv_obj_t *skip_btn = lv_button_create(btn_cont);
  lv_obj_t *skip_lbl = lv_label_create(skip_btn);
  lv_label_set_text(skip_lbl, "Skip");
  lv_obj_add_event_cb(skip_btn, onSkipPrefilledButtonClicked, LV_EVENT_CLICKED,
                      NULL);
}

void DisplayUI::buildFetchingOverlay() {
  fetchingOverlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(fetchingOverlay, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(fetchingOverlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(fetchingOverlay, LV_OPA_70, 0);

  lv_obj_t *label = lv_label_create(fetchingOverlay);
  lv_label_set_text(label, "Fetching from Spoolman...");
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_center(label);

  lv_obj_add_flag(fetchingOverlay, LV_OBJ_FLAG_HIDDEN);
}

void DisplayUI::showEditScreen() {
  // Populate from currentLoadedData
  lv_dropdown_set_selected_highlight(editBrandDropdown, false);
  // Manual search for brand in list
  const char *opts = lv_dropdown_get_options(editBrandDropdown);
  std::string options(opts);
  size_t pos = options.find(currentLoadedData.brand);

  if (!currentLoadedData.brand.empty() && pos != std::string::npos) {
    int idx = 0;
    size_t current_pos = 0;
    bool found = false;
    while (current_pos != std::string::npos) {
      size_t next_pos = options.find('\n', current_pos);
      std::string opt =
          (next_pos == std::string::npos)
              ? options.substr(current_pos)
              : options.substr(current_pos, next_pos - current_pos);
      if (opt == currentLoadedData.brand) {
        lv_dropdown_set_selected(editBrandDropdown, idx);
        lv_obj_add_flag(editCustomBrandRow, LV_OBJ_FLAG_HIDDEN);
        found = true;
        break;
      }
      if (next_pos == std::string::npos)
        break;
      current_pos = next_pos + 1;
      idx++;
    }

    if (!found) {
      // It's a custom brand even if the string exists as a substring elsewhere
      lv_dropdown_set_selected(editBrandDropdown, 12); // "Custom"
      lv_obj_clear_flag(editCustomBrandRow, LV_OBJ_FLAG_HIDDEN);
      lv_textarea_set_text(editCustomBrandTextArea,
                           currentLoadedData.brand.c_str());
    }
  } else if (!currentLoadedData.brand.empty()) {
    // Brand not in list
    lv_dropdown_set_selected(editBrandDropdown, 12); // "Custom"
    lv_obj_clear_flag(editCustomBrandRow, LV_OBJ_FLAG_HIDDEN);
    lv_textarea_set_text(editCustomBrandTextArea,
                         currentLoadedData.brand.c_str());
  } else {
    lv_dropdown_set_selected(editBrandDropdown, 0); // Default Generic
    lv_obj_add_flag(editCustomBrandRow, LV_OBJ_FLAG_HIDDEN);
  }

  lv_dropdown_set_selected_highlight(editTypeDropdown, false);
  const char *type_opts = lv_dropdown_get_options(editTypeDropdown);
  std::string type_options(type_opts);
  pos = type_options.find(currentLoadedData.type);
  if (pos != std::string::npos) {
    int idx = 0;
    for (size_t i = 0; i < pos; i++) {
      if (type_options[i] == '\n')
        idx++;
    }
    lv_dropdown_set_selected(editTypeDropdown, idx);
  } else {
    lv_dropdown_set_selected(editTypeDropdown, 0); // Default PLA
  }

  if (currentLoadedData.color_hex[0] == '#') {
    lv_textarea_set_text(editColorHexTextArea,
                         currentLoadedData.color_hex.c_str() + 1);
  } else {
    lv_textarea_set_text(editColorHexTextArea,
                         currentLoadedData.color_hex.c_str());
  }
  lv_textarea_set_text(editSpoolIdTextArea, currentLoadedData.spool_id.c_str());
  lv_textarea_set_text(editLotNrTextArea, currentLoadedData.lot_nr.c_str());
  lv_textarea_set_text(editMinTempTextArea, currentLoadedData.min_temp.c_str());
  lv_textarea_set_text(editMaxTempTextArea, currentLoadedData.max_temp.c_str());
  lv_textarea_set_text(editBedMinTextArea,
                       currentLoadedData.bed_min_temp.c_str());
  lv_textarea_set_text(editBedMaxTextArea,
                       currentLoadedData.bed_max_temp.c_str());

  // Sync color preview
  if (currentLoadedData.color_hex[0] == '#' &&
      currentLoadedData.color_hex.length() == 7) {
    uint32_t color = strtol(currentLoadedData.color_hex.c_str() + 1, NULL, 16);
    lv_obj_set_style_bg_color(editColorPreview, lv_color_hex(color), 0);
  }

  // Initial validation
  updateSaveButtonState();

  lv_scr_load(editScreen);
}

void DisplayUI::showEditScreenForNew() {
  currentLoadedData = OpenSpoolData();
  currentLoadedData.brand = "Generic";
  currentLoadedData.type = "PLA";
  currentLoadedData.color_hex = "#FFFFFF";
  showEditScreen();
}

bool DisplayUI::isScanScreenActive() { return lv_scr_act() == scanScreen; }

void DisplayUI::onLoadSpoolButtonClicked(lv_event_t *e) {
  uint8_t tools = ConfigManager::getNumTools();

  if (tools == 1) {
    // Immediate Webhook Fire
    NetworkManager::sendWebhookPayload(currentLoadedData, 0);
    lv_scr_load(infoScreen); // Stay on Info screen
  } else {
    // Show tool grid popup
    showToolSelectionScreen();
  }
}

void DisplayUI::onToolButtonClicked(lv_event_t *e) {
  int toolhead_id = (int)(intptr_t)lv_event_get_user_data(e);
  // Fire Webhook synchronously (simple implementation, can be async later)
  NetworkManager::sendWebhookPayload(currentLoadedData, toolhead_id);

  // Return to the Info screen after loading
  lv_scr_load(infoScreen);
}

void DisplayUI::onEditButtonClicked(lv_event_t *e) { showEditScreen(); }

void DisplayUI::onCreateNewButtonClicked(lv_event_t *e) {
  if (!ConfigManager::getSpoolmanUrl().empty()) {
    lv_textarea_set_text(promptSpoolIdTextArea, "");
    lv_obj_add_state(promptLoadBtn, LV_STATE_DISABLED);
    lv_scr_load(spoolIdPromptScreen);
  } else {
    currentLoadedData.reset();
    showEditScreen();
  }
}

void DisplayUI::onLoadPrefilledButtonClicked(lv_event_t *e) {
  const char *id = lv_textarea_get_text(promptSpoolIdTextArea);
  if (strlen(id) == 0) {
    showToast("Enter a Spool ID", true);
    return;
  }

  // Show "Fetching..."
  lv_obj_clear_flag(fetchingOverlay, LV_OBJ_FLAG_HIDDEN);
  lv_refr_now(NULL);

  currentLoadedData.reset();
  currentLoadedData.spool_id = id;

  bool success = NetworkManager::fetchSpoolmanData(currentLoadedData);

  lv_obj_add_flag(fetchingOverlay, LV_OBJ_FLAG_HIDDEN);

  if (success) {
    showToast("Data prefilled from Spoolman");
  } else {
    showToast("Fetch failed, starting fresh", true);
  }

  showEditScreen();
}

void DisplayUI::onSkipPrefilledButtonClicked(lv_event_t *e) {
  currentLoadedData.reset();
  showEditScreen();
}

void DisplayUI::onColorHexChanged(lv_event_t *e) {
  const char *txt = lv_textarea_get_text(editColorHexTextArea);
  if (txt && strlen(txt) == 6) {
    uint32_t color = strtol(txt, NULL, 16);
    lv_obj_set_style_bg_color(editColorPreview, lv_color_hex(color), 0);
  }
}

void DisplayUI::onBrandDropdownChanged(lv_event_t *e) {
  uint32_t selected = lv_dropdown_get_selected(editBrandDropdown);
  if (selected == 12) { // "Custom"
    lv_obj_clear_flag(editCustomBrandRow, LV_OBJ_FLAG_HIDDEN);
    validateField(editCustomBrandTextArea);
  } else {
    lv_obj_add_flag(editCustomBrandRow, LV_OBJ_FLAG_HIDDEN);
    updateSaveButtonState();
  }
}

void DisplayUI::onKeyboardEvent(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    if (code == LV_EVENT_READY) {
      lv_obj_t *ta = lv_keyboard_get_textarea(keyboard);
      if (ta == promptSpoolIdTextArea) {
        onLoadPrefilledButtonClicked(NULL);
      }
    }
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  }
}

void DisplayUI::onSaveButtonClicked(lv_event_t *e) {
  if (!validateAll()) {
    return; // Don't save if validation fails
  }
  // Extract values from UI
  uint32_t selectedBrand = lv_dropdown_get_selected(editBrandDropdown);
  if (selectedBrand == 12) { // Custom
    currentLoadedData.brand = lv_textarea_get_text(editCustomBrandTextArea);
  } else {
    char brand[64];
    lv_dropdown_get_selected_str(editBrandDropdown, brand, sizeof(brand));
    currentLoadedData.brand = brand;
  }

  char type[32];
  lv_dropdown_get_selected_str(editTypeDropdown, type, sizeof(type));
  currentLoadedData.type = type;

  std::string hexText = lv_textarea_get_text(editColorHexTextArea);
  currentLoadedData.color_hex = "#" + hexText;
  currentLoadedData.spool_id = lv_textarea_get_text(editSpoolIdTextArea);
  currentLoadedData.lot_nr = lv_textarea_get_text(editLotNrTextArea);
  currentLoadedData.min_temp = lv_textarea_get_text(editMinTempTextArea);
  currentLoadedData.max_temp = lv_textarea_get_text(editMaxTempTextArea);
  currentLoadedData.bed_min_temp = lv_textarea_get_text(editBedMinTextArea);
  currentLoadedData.bed_max_temp = lv_textarea_get_text(editBedMaxTextArea);
  currentLoadedData.protocol = "openspool"; // Ensure protocol is set

#ifndef USE_SDL2
  // Hardware: Trigger non-blocking write
  if (writingOverlay)
    lv_obj_clear_flag(writingOverlay, LV_OBJ_FLAG_HIDDEN);

  writePending = true;
  writeStartTime = millis();

  // Return for hardware, polling will happen in main loop
  return;
#else
  // Simulator: Update mock tag file (keep synchronous)
  std::string json = OpenSpoolParser::toJson(currentLoadedData);
  FILE *fp = fopen("simulator/spool.json", "w");
  bool success = false;
  if (fp) {
    fputs(json.c_str(), fp);
    fclose(fp);
    success = true;
  }
  if (success) {
    showToast("Tag written successfully!", false);
    showInfoScreen(currentLoadedData);
  } else {
    showToast("Failed to write tag!", true);
  }
#endif
}

void DisplayUI::onCancelEditButtonClicked(lv_event_t *e) {
  if (currentLoadedData.protocol.empty()) {
    showScanScreen();
  } else {
    showInfoScreen(currentLoadedData);
  }
}

void DisplayUI::onBackButtonClicked(lv_event_t *e) {
  lv_obj_t *scr = lv_scr_act();
  if (scr == toolSelectionScreen || scr == editScreen) {
    lv_scr_load(infoScreen);
  } else {
    lv_scr_load(scanScreen);
  }
}

void DisplayUI::onTextAreaFocused(lv_event_t *e) {
  lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
  lv_keyboard_set_textarea(keyboard, ta);
  lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  // Auto switch to numeric mode for temps and Spool ID
  if (ta == editMinTempTextArea || ta == editMaxTempTextArea ||
      ta == editBedMinTextArea || ta == editBedMaxTextArea ||
      ta == editSpoolIdTextArea || ta == promptSpoolIdTextArea) {
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
  } else if (ta == editColorHexTextArea) {
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_UPPER);
  } else {
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
  }
}

void DisplayUI::onTextAreaChanged(lv_event_t *e) { updateSaveButtonState(); }

bool DisplayUI::validateField(lv_obj_t *ta) {
  const char *txt = lv_textarea_get_text(ta);
  bool valid = true;

  if (ta == editColorHexTextArea) {
    if (strlen(txt) != 6) {
      valid = false;
    } else {
      for (int i = 0; i < 6; i++) {
        char c = toupper(txt[i]);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
          valid = false;
          break;
        }
      }
    }
  } else if (ta == editSpoolIdTextArea) {
    if (strlen(txt) > 0) {
      long val = strtol(txt, NULL, 10);
      if (val < 0)
        valid = false;
    }
  } else if (ta == editMinTempTextArea || ta == editMaxTempTextArea) {
    long val = strtol(txt, NULL, 10);
    if (val < 0 || val > 450)
      valid = false;
  } else if (ta == editBedMinTextArea || ta == editBedMaxTextArea) {
    long val = strtol(txt, NULL, 10);
    if (val < 0 || val > 150)
      valid = false;
  } else if (ta == editCustomBrandTextArea) {
    uint32_t selected = lv_dropdown_get_selected(editBrandDropdown);
    if (selected == 12 && strlen(txt) == 0)
      valid = false;
  }

  if (valid) {
    lv_obj_set_style_border_color(ta, lv_color_hex(0x374151),
                                  0); // Default dark border
    lv_obj_set_style_border_width(ta, 1, 0);
  } else {
    lv_obj_set_style_border_color(ta, lv_color_hex(0xef4444), 0); // Red border
    lv_obj_set_style_border_width(ta, 2, 0);
  }

  return valid;
}

bool DisplayUI::validateAll() {
  bool all_valid = true;
  if (!validateField(editColorHexTextArea))
    all_valid = false;
  if (!validateField(editSpoolIdTextArea))
    all_valid = false;
  if (!validateField(editMinTempTextArea))
    all_valid = false;
  if (!validateField(editMaxTempTextArea))
    all_valid = false;
  if (!validateField(editBedMinTextArea))
    all_valid = false;
  if (!validateField(editBedMaxTextArea))
    all_valid = false;
  if (lv_dropdown_get_selected(editBrandDropdown) == 12) {
    if (!validateField(editCustomBrandTextArea))
      all_valid = false;
  }
  return all_valid;
}

void DisplayUI::updateSaveButtonState() {
  bool isValid = validateAll();
  if (isValid) {
    lv_obj_remove_state(editSaveBtn, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(editSaveBtn, LV_STATE_DISABLED);
  }

  // Update prompt load button state
  if (promptSpoolIdTextArea && promptLoadBtn) {
    const char *text = lv_textarea_get_text(promptSpoolIdTextArea);
    if (strlen(text) > 0) {
      lv_obj_remove_state(promptLoadBtn, LV_STATE_DISABLED);
    } else {
      lv_obj_add_state(promptLoadBtn, LV_STATE_DISABLED);
    }
  }
}

void DisplayUI::showToast(const char *msg, bool is_error) {
  if (toastObj) {
    lv_obj_del(toastObj);
    toastObj = nullptr;
  }
  if (toastTimer) {
    lv_timer_del(toastTimer);
    toastTimer = nullptr;
  }

  toastObj = lv_obj_create(lv_layer_top());
  lv_obj_set_size(toastObj, LV_PCT(80), LV_SIZE_CONTENT);
  lv_obj_align(toastObj, LV_ALIGN_TOP_MID, 0, 20);

  if (is_error) {
    lv_obj_set_style_bg_color(toastObj, lv_color_hex(0x7f1d1d), 0); // Dark red
  } else {
    lv_obj_set_style_bg_color(toastObj, lv_color_hex(0x064e3b),
                              0); // Dark green
  }

  lv_obj_set_style_bg_opa(toastObj, LV_OPA_90, 0);
  lv_obj_set_style_border_width(toastObj, 1, 0);
  lv_obj_set_style_border_color(toastObj, lv_color_white(), 0);
  lv_obj_set_style_radius(toastObj, 10, 0);
  lv_obj_set_style_shadow_width(toastObj, 20, 0);
  lv_obj_set_style_shadow_color(toastObj, lv_color_black(), 0);

  lv_obj_t *l = lv_label_create(toastObj);
  lv_label_set_text(l, msg);
  lv_obj_set_style_text_color(l, lv_color_white(), 0);
  lv_obj_set_width(l, LV_PCT(100));
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(l);

  toastTimer = lv_timer_create(onToastTimer, 3000, NULL);
}

void DisplayUI::onToastTimer(lv_timer_t *t) {
  if (toastObj) {
    lv_obj_del(toastObj);
    toastObj = nullptr;
  }
  lv_timer_del(t);
  toastTimer = nullptr;
}

void DisplayUI::hideWritingOverlay() {
  if (writingOverlay) {
    lv_obj_add_flag(writingOverlay, LV_OBJ_FLAG_HIDDEN);
  }
}

const OpenSpoolData &DisplayUI::getPendingData() { return currentLoadedData; }

bool DisplayUI::isEditing() {
  return lv_scr_act() == editScreen || lv_scr_act() == spoolIdPromptScreen;
}
