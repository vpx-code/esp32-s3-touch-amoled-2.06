#include "esp_brookesia.hpp"
#include "lvgl.h"
#include <cstdio>
#include <ctime>

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Settings"
#include "esp_lib_utils.h"

#include "brookesia/lib_utils.hpp"
#include "brookesia/service_helper/wifi.hpp"
#include "brookesia/service_manager.hpp"

#include "../main/dark/theme_constants.hpp"
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

  /*
  ============================MENU===========================
  */
  // Create menu
  /*Create a menu object*/
  lv_obj_t *menu = lv_menu_create(lv_screen_active());

  lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL),
                  lv_display_get_vertical_resolution(NULL));

  lv_obj_set_style_pad_top(menu, theme::PADDING, 0);
  lv_obj_set_style_pad_bottom(menu, theme::PADDING, 0);
  lv_obj_set_style_pad_left(menu, theme::PADDING, 0);
  lv_obj_set_style_pad_right(menu, theme::PADDING, 0);

  lv_obj_set_style_bg_color(menu, lv_color_hex(theme::COLOR_BG_LAUNCHER), 0);

  lv_obj_center(menu);

  /*Modify the header*/
  lv_obj_t *back_btn = lv_menu_get_main_header_back_button(menu);
  lv_obj_t *back_button_label = lv_label_create(back_btn);
  lv_label_set_text(back_button_label, "Back");
  lv_obj_set_style_text_font(back_button_label, &lv_font_montserrat_32, 0);

  lv_obj_t *cont;
  lv_obj_t *label;

  /*Create sub pages*/ // TODO: Let's abstract this or it'll become a mess in no
                       // time
  lv_obj_t *sub_1_page = lv_menu_page_create(menu, "Wi-Fi");

  cont = lv_menu_cont_create(sub_1_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  lv_label_set_text(label, "Scan Networks");

  lv_obj_t *sub_2_page =
      lv_menu_page_create(menu, "BLE"); // TODO: can we make the title bigger?

  cont = lv_menu_cont_create(sub_2_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  lv_label_set_text(label, "Connect to Bluetooth");

  lv_obj_t *sub_3_page = lv_menu_page_create(menu, "Display");
  lv_obj_set_style_text_font(sub_3_page, &lv_font_montserrat_24, 0);

  cont = lv_menu_cont_create(sub_3_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  lv_label_set_text(label, "Brightness");

  /*Create a main page*/
  lv_obj_t *main_page = lv_menu_page_create(menu, NULL);

  cont = lv_menu_cont_create(main_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  lv_label_set_text(label, "Wi-Fi");
  lv_menu_set_load_page_event(menu, cont, sub_1_page);

  cont = lv_menu_cont_create(main_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  lv_label_set_text(label, "BLE");
  lv_menu_set_load_page_event(menu, cont, sub_2_page);

  cont = lv_menu_cont_create(main_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  lv_label_set_text(label, "Display");
  lv_menu_set_load_page_event(menu, cont, sub_3_page);

  lv_menu_set_page(menu, main_page);

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