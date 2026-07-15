// esp_err.h must be included before bsp/display.h, which uses esp_err_t without
// including it itself. The blank line keeps clang-format from reordering them.
#include "esp_err.h"

#include "bsp/display.h"
#include "esp_brookesia.hpp"
#include "log/esp_utils_log.h"
#include "lvgl.h"
#include "misc/lv_types.h"
#include <cstdint>
#include <cstdio>
#include <ctime>

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Settings"
#include "esp_lib_utils.h"

#include "../main/dark/theme_constants.hpp"
#include "Settings.hpp"
#include "WifiPage.hpp"

#include "brookesia/service_helper/nvs.hpp"

#include <memory>
/* ------------------------------------------------------------------
 * App identity
 * ------------------------------------------------------------------ */
#define APP_NAME "Settings"

/* Backlight brightness is expressed as a percentage (0-100). */
#define Backlight_MAX 100
#define DEFAULT_BACKLIGHT 50

using namespace std;
using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;
using NVSHelper = service::helper::NVS;

const std::string NVS_NAMESPACE = "settings";

static lv_obj_t *Backlight_slider;

/* Recreated each time the Settings screen is built. Assigning a fresh instance
 * destroys the previous one, whose scoped subscriptions unsubscribe cleanly. */
static std::unique_ptr<esp_brookesia::apps::WifiPage> wifi_page;

/* Launcher icon — 112×112 pixel image stored as a C array. */
LV_IMG_DECLARE(img_app_setting);

namespace esp_brookesia::apps {

template <typename T> void save_to_nvs(const std::string key, T value) {
  auto didSetttingSave = NVSHelper::save_key_value(NVS_NAMESPACE, key, value);
  if (didSetttingSave.has_value()) {
    ESP_UTILS_LOGI("Saved %s to NVS", key.c_str());
  } else {
    ESP_UTILS_LOGE("Failed to save value. %s", didSetttingSave.error().c_str());
  }
}

static void Backlight_adjustment_event_cb(lv_event_t *e) {
  uint8_t Backlight = lv_slider_get_value((lv_obj_t *)lv_event_get_target(e));
  if (Backlight <= 100) {
    lv_slider_set_value(Backlight_slider, Backlight, LV_ANIM_ON);
    bsp_display_brightness_set(Backlight);
  } else
    printf("Backlight out of range: %d\n", Backlight);
}

static void Backlight_slider_stopped_event_cb(lv_event_t *e) {
  uint8_t Backlight = lv_slider_get_value((lv_obj_t *)lv_event_get_target(e));
  if (Backlight <= 100) {
    save_to_nvs("brightness", Backlight);
  } else
    printf("Backlight out of range: %d\n", Backlight);
}

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

bool SettingsApp::init(void) {
  ESP_UTILS_LOGD("Settings init()");
  return true;
}

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

  /* Header: a back arrow (no "Back" text) on the left, and the page title
   * (Wi-Fi / BLE / Display) shown white and centered so it's actually legible.
   * The menu keeps a single title label in the main header and only swaps its
   * text when navigating, so styling it once here sticks for every page. */
  lv_obj_t *header = lv_menu_get_main_header(menu);
  lv_obj_t *back_btn = lv_menu_get_main_header_back_button(menu);

  /* The menu already ships a back arrow (an lv_image of the LV_SYMBOL_LEFT
   * glyph, child 0 of the back button) but the theme draws it small and dim.
   * Enlarge + whiten that original instead of adding a second arrow. */
  lv_obj_t *back_icon = lv_obj_get_child(back_btn, 0);
  if (back_icon != nullptr) {
    lv_obj_set_style_text_font(back_icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(back_icon,
                                lv_color_hex(theme::COLOR_PURE_WHITE), 0);
    lv_obj_set_style_image_recolor(back_icon,
                                   lv_color_hex(theme::COLOR_PURE_WHITE), 0);
    lv_obj_set_style_image_recolor_opa(back_icon, LV_OPA_COVER, 0);
  }

  /* The built-in title is the header child that isn't the back button. */
  lv_obj_t *header_title = nullptr;
  for (uint32_t i = 0; i < lv_obj_get_child_count(header); i++) {
    lv_obj_t *child = lv_obj_get_child(header, i);
    if (child != back_btn) {
      header_title = child;
      break;
    }
  }
  if (header_title != nullptr) {
    lv_obj_set_style_text_font(header_title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(header_title,
                                lv_color_hex(theme::COLOR_PURE_WHITE), 0);
    /* Take the title out of the header's flex row and pin it to the header's
     * centre, so it's centred across the FULL width rather than just the space
     * beside the back button. IGNORE_LAYOUT stops flex from repositioning it;
     * set_align is sticky, so it re-centres when the text changes per page. */
    lv_obj_add_flag(header_title, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_align(header_title, LV_ALIGN_CENTER);
  }

  lv_obj_t *cont;
  lv_obj_t *label;

  /*Create sub pages*/ // TODO: Let's abstract this or it'll become a mess in no
                       // time
  lv_obj_t *sub_1_page = lv_menu_page_create(menu, "Wi-Fi");

  /* Fresh WifiPage each build; the previous one is destroyed here, which
   * unsubscribes its Wi-Fi service events. */
  wifi_page = std::make_unique<WifiPage>();
  wifi_page->create(sub_1_page);

  lv_obj_t *sub_2_page =
      lv_menu_page_create(menu, "BLE"); // TODO: can we make the title bigger?

  cont = lv_menu_cont_create(sub_2_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  lv_label_set_text(label, "Connect to Bluetooth");

  lv_obj_t *sub_3_page = lv_menu_page_create(menu, "Display");

  lv_obj_t *panel1 = lv_menu_cont_create(sub_3_page);
  /* Stack the label above the slider instead of side-by-side, both
   * left-aligned within the container. */
  lv_obj_set_flex_flow(panel1, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(panel1, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_t *Backlight_label = lv_label_create(panel1);
  lv_label_set_text(Backlight_label, "Brightness:");
  lv_obj_set_style_text_font(Backlight_label, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(Backlight_label,
                              lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  Backlight_slider = lv_slider_create(panel1);
  lv_obj_add_flag(Backlight_slider, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(Backlight_slider, lv_pct(90), 30);
  lv_obj_set_style_radius(
      Backlight_slider, 3,
      LV_PART_KNOB); // Adjust the value for more or less rounding
  lv_obj_set_style_bg_opa(Backlight_slider, LV_OPA_TRANSP, LV_PART_KNOB);
  // lv_obj_set_style_pad_all(Backlight_slider, 0, LV_PART_KNOB);
  lv_obj_set_style_bg_color(Backlight_slider, lv_color_hex(0xAAAAAA),
                            LV_PART_KNOB);
  lv_obj_set_style_bg_color(Backlight_slider, lv_color_hex(0xFFFFFF),
                            LV_PART_INDICATOR);
  lv_obj_set_style_outline_width(Backlight_slider, 2, LV_PART_INDICATOR);
  lv_obj_set_style_outline_color(Backlight_slider, lv_color_hex(0xD3D3D3),
                                 LV_PART_INDICATOR);
  lv_slider_set_range(Backlight_slider, 5, Backlight_MAX);
  lv_slider_set_value(Backlight_slider, DEFAULT_BACKLIGHT, LV_ANIM_ON);
  lv_obj_add_event_cb(Backlight_slider, Backlight_adjustment_event_cb,
                      LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_add_event_cb(Backlight_slider, Backlight_slider_stopped_event_cb,
                      LV_EVENT_RELEASED, NULL);

  /*Create a main page*/
  lv_obj_t *main_page = lv_menu_page_create(menu, NULL);

  cont = lv_menu_cont_create(main_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  lv_label_set_text(label, "Wi-Fi");
  lv_menu_set_load_page_event(menu, cont, sub_1_page);

  cont = lv_menu_cont_create(main_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(theme::COLOR_PURE_WHITE), 0);
  lv_label_set_text(label, "BLE");
  lv_menu_set_load_page_event(menu, cont, sub_2_page);

  cont = lv_menu_cont_create(main_page);
  label = lv_label_create(cont);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
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