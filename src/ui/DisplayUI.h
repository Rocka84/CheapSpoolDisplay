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
  static uint32_t getLastInteractionTime();

  // Screens
  static void showScanScreen();
  static void showInfoScreen(const OpenSpoolData &spool);
  static void showToolSelectionScreen();
  static void showEditScreen();
  static void showEditScreenForNew();

  static bool isScanScreenActive();

  static bool isWritePending() { return writePending; }
  static void setWritePending(bool pending) { writePending = pending; }
  static const OpenSpoolData& getPendingData();
  static void hideWritingOverlay();
  static uint32_t getWriteStartTime() { return writeStartTime; }
  static bool isEditing();

  static void showToast(const char *msg, bool is_error = false);

private:
  static void buildScanScreen();
  static void buildInfoScreen();
  static void buildToolSelectionScreen();
  static void buildEditScreen();
  static void buildWritingOverlay();

  // Event callbacks
  static void onLoadSpoolButtonClicked(lv_event_t *e);
  static void onToolButtonClicked(lv_event_t *e);
  static void onEditButtonClicked(lv_event_t *e);
  static void onCreateNewButtonClicked(lv_event_t *e);
  static void onSaveButtonClicked(lv_event_t *e);
  static void onCancelEditButtonClicked(lv_event_t *e);
  static void onBackButtonClicked(lv_event_t *e);
  static void onTextAreaFocused(lv_event_t *e);
  static void onTextAreaChanged(lv_event_t *e);
  static void onColorHexChanged(lv_event_t *e);
  static void onBrandDropdownChanged(lv_event_t *e);
  static void onKeyboardEvent(lv_event_t *e);

  // Validation
  static bool validateField(lv_obj_t *ta);
  static bool validateAll();
  static void updateSaveButtonState();

  // Notifications
  static void onToastTimer(lv_timer_t *t);

  // Screen pointers
  static lv_obj_t *scanScreen;
  static lv_obj_t *infoScreen;
  static lv_obj_t *toolSelectionScreen;
  static lv_obj_t *editScreen;
  static lv_obj_t *writingOverlay;
  static bool writePending;
  static uint32_t writeStartTime;

  // Toast management
  static lv_obj_t *toastObj;
  static lv_timer_t *toastTimer;

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
  static lv_obj_t *createNewBtn;

  // Edit Screen Elements
  static lv_obj_t *editBrandDropdown;
  static lv_obj_t *editCustomBrandRow;
  static lv_obj_t *editCustomBrandTextArea;
  static lv_obj_t *editTypeDropdown;
  static lv_obj_t *editColorHexTextArea;
  static lv_obj_t *editColorPreview;
  static lv_obj_t *editSaveBtn;
  static lv_obj_t *editSpoolIdTextArea;
  static lv_obj_t *editLotNrTextArea;
  static lv_obj_t *editMinTempTextArea;
  static lv_obj_t *editMaxTempTextArea;
  static lv_obj_t *editBedMinTextArea;
  static lv_obj_t *editBedMaxTextArea;
  static lv_obj_t *keyboard;

  // Spoolman enrichment UI
  static lv_obj_t *keyFilament;
  static lv_obj_t *labelFilamentName;
  static lv_obj_t *keyWeight;
  static lv_obj_t *labelWeight;
  static lv_obj_t *toolGrid;

  static OpenSpoolData currentLoadedData;
};

#endif // DISPLAY_UI_H
