#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#ifndef USE_SDL2
#include <XPT2046_Bitbang.h>
#endif
#include "../data/OpenSpool.h"
#include <lvgl.h>

#ifndef USE_SDL2
extern XPT2046_Bitbang ts;
#endif

class DisplayUI {
public:
  static void init();
  static void tick();

  // Screens
  static void showScanScreen();
  static void showInfoScreen(const OpenSpoolData &spool);
  static void showToolSelectionScreen();
  static void showEditScreen();

  static bool isScanScreenActive();

private:
  static void buildScanScreen();
  static void buildInfoScreen();
  static void buildToolSelectionScreen();
  static void buildEditScreen();

  // Event callbacks
  static void onLoadSpoolButtonClicked(lv_event_t *e);
  static void onToolButtonClicked(lv_event_t *e);
  static void onEditButtonClicked(lv_event_t *e);
  static void onSaveButtonClicked(lv_event_t *e);
  static void onBackButtonClicked(lv_event_t *e);

  // Screen pointers
  static lv_obj_t *scanScreen;
  static lv_obj_t *infoScreen;
  static lv_obj_t *toolSelectionScreen;
  static lv_obj_t *editScreen;

  // UI Elements for Info
  static lv_obj_t *labelBrand;
  static lv_obj_t *labelType;
  static lv_obj_t *colorBox;
  static lv_obj_t *labelSpoolId;
  static lv_obj_t *labelSubtype;
  static lv_obj_t *labelLotNr;
  static lv_obj_t *keyLotNr;
  static lv_obj_t *labelTemp;
  static lv_obj_t *labelBedTemp;
  static lv_obj_t *labelColorHex;
  static lv_obj_t *loadBtn;
};

#endif // DISPLAY_UI_H
