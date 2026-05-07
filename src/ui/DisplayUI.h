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
  static void showSelectSpoolScreen();
  static void showExtendedInfoScreen();
  static void showExtendedEditScreen();

  static bool isScanScreenActive();

  static bool isWritePending() { return writePending; }
  static void setWritePending(bool pending) { writePending = pending; }
  static const OpenSpoolData& getPendingData();
  static void showWritingOverlay();
  static void hideWritingOverlay();
  static void showFetchingOverlay();
  static void hideFetchingOverlay();
  static uint32_t getWriteStartTime() { return writeStartTime; }
  static bool isEditing();

  static void showToast(const char *msg, bool is_error = false);
  static void updateWiFiStatus(bool connected);

private:
  static void buildScanScreen();
  static void buildInfoScreen();
  static void buildToolSelectionScreen();
  static void buildEditScreen();
  static void buildWritingOverlay();
  static void buildFetchingOverlay();
  static void buildSelectSpoolScreen();
  static void buildExtendedInfoScreen();
  static void buildExtendedEditScreen();
  static void updateSelectSpoolList(const std::vector<SpoolmanItem>& items, int total_count);

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
  static void onSelectSpoolButtonClicked(lv_event_t *e);
  static void onPrevPageClicked(lv_event_t *e);
  static void onNextPageClicked(lv_event_t *e);
  static void onMoreInfoButtonClicked(lv_event_t *e);
  static void onMoreEditButtonClicked(lv_event_t *e);

  // Validation
  static bool validateField(lv_obj_t *ta);
  static bool validateAll();
  static void updateSaveButtonState();

  // Notifications
  static void onToastTimer(lv_timer_t *t);

  // Screen pointers
  static lv_obj_t *scanScreen;
  static lv_obj_t *wifiIcon;
  static lv_obj_t *infoScreen;
  static lv_obj_t *toolSelectionScreen;
  static lv_obj_t *editScreen;
  static lv_obj_t *writingOverlay;
  static lv_obj_t *fetchingOverlay;
  static lv_obj_t *selectSpoolScreen;
  static lv_obj_t *extendedInfoScreen;
  static lv_obj_t *extendedEditScreen;
  static lv_obj_t *spoolListCont;
  static lv_obj_t *pageLabel;
  static lv_obj_t *selectSpoolPrevBtn;
  static lv_obj_t *selectSpoolNextBtn;
  static int currentSpoolPage;
  static OpenSpoolData backupLoadedData;
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
  static lv_obj_t *labelDiameter;
  static lv_obj_t *keyDiameter;
  static lv_obj_t *labelTemp;
  static lv_obj_t *labelBedTemp;
  static lv_obj_t *labelColorHex;
  
  // Containers for scrolling
  static lv_obj_t *infoCard;
  static lv_obj_t *extCard;
  static lv_obj_t *editCont;
  static lv_obj_t *extendedEditCont;
  static lv_obj_t *formatCont;
  static lv_obj_t *toolGrid;
  static lv_obj_t *spoolListCont_ignored; // Dummy to avoid conflict if I misread header

  static lv_obj_t *loadBtn;
  static lv_obj_t *spoolmanBtn;
  static lv_obj_t *createNewBtn;
  static lv_obj_t *moreInfoBtn;

  // Extended Info Elements
  static lv_obj_t *labelDensity;
  static lv_obj_t *labelActualWeight;
  static lv_obj_t *labelEmptyWeight;
  static lv_obj_t *labelDryTemp;
  static lv_obj_t *labelDryTime;
  static lv_obj_t *labelTD;
  static lv_obj_t *labelShore;
  static lv_obj_t *labelTags;
  static lv_obj_t *labelLocation;
  static lv_obj_t *labelPrice;
  static lv_obj_t *labelNotes;
  static lv_obj_t *labelFirstUsed;
  static lv_obj_t *labelLastUsed;
  static lv_obj_t *noDataLabel;
  static lv_obj_t *weightBar;

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
  static lv_obj_t *editDiameterTextArea;
  static lv_obj_t *moreEditBtn;

  // Extended Edit Elements
  static lv_obj_t *editActualWeightTextArea;
  static lv_obj_t *editEmptyWeightTextArea;
  static lv_obj_t *editDensityTextArea;
  static lv_obj_t *editDryTempTextArea;
  static lv_obj_t *editDryTimeTextArea;
  static lv_obj_t *editTDTextArea;
  static lv_obj_t *editShoreTextArea;
  static lv_obj_t *editTagsTextArea;
  static lv_obj_t *editLocationTextArea;
  static lv_obj_t *editSubtypeTextArea;
  static lv_obj_t *editNotesTextArea;
  static lv_obj_t *keyboardCore;
  static lv_obj_t *keyboardExtended;

  // Spoolman enrichment UI
  static lv_obj_t *keyFilament;
  static lv_obj_t *labelFilamentName;
  static lv_obj_t *keyWeight;
  static lv_obj_t *labelWeight;

  static void showFormatSelectionModal();

  // Event callbacks
  static void onFormatSelected(lv_event_t *e);
  static void onDontAskCheckChanged(lv_event_t *e);

  // UI Elements
  static lv_obj_t *formatModal;
  static lv_obj_t *dontAskCheckbox;

  static OpenSpoolData currentLoadedData;
};

#endif // DISPLAY_UI_H
