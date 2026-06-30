#include "esp_brookesia.hpp"
#include "lvgl.h"
#include <cstdio>
#include <ctime>

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Settings"
#include "esp_lib_utils.h"

#include "Settings.hpp"

/* ------------------------------------------------------------------
 * App identity
 * ------------------------------------------------------------------ */
#define APP_NAME "Settings"

using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;

/* Launcher icon — 112×112 pixel image stored as a C array. */
LV_IMG_DECLARE(img_app_setting);

namespace esp_brookesia::apps {
SettingsApp *SettingsApp::_instance = nullptr;

SettingsApp *SettingsApp::requestInstance(bool use_status_bar,
                                          bool use_navigation_bar) {
  if (_instance == nullptr) {
    _instance = new SettingsApp(use_status_bar, use_navigation_bar);
  }
  return _instance;
}

// CONSTRUCTOR AND DESTRUCTOR
SettingsApp::SettingsApp(bool use_status_bar, bool use_navigation_bar)
    : App(APP_NAME, &img_app_setting, /*use_default_screen=*/true,
          use_status_bar, use_navigation_bar) {}

SettingsApp::~SettingsApp() {}

bool SettingsApp::run(void) {
  ESP_UTILS_LOGD("Settings run()");

  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57),
                            LV_PART_MAIN);

  /*Create a white label, set its text and align it to the center*/
  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Settings");
  lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff),
                              LV_PART_MAIN);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  return true;
}

bool SettingsApp::back(void) {
  ESP_UTILS_LOGD("Settings back()");
  notifyCoreClosed();
  return true;
}

ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(
    systems::base::App, SettingsApp, APP_NAME, []() {
      return std::shared_ptr<SettingsApp>(SettingsApp::requestInstance(),
                                          [](SettingsApp *) {});
    })
} // namespace esp_brookesia::apps