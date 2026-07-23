/*
 * Weather app — test simple HTTP requests.
 */

#include <cstdio>
#include <ctime>

#include "esp_brookesia.hpp"
#include "esp_log.h"

#include "lvgl.h"

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Weather"
#include "esp_lib_utils.h"

#include "esp_brookesia_app_weather.hpp"
#include "esp_crt_bundle.h"

/* ------------------------------------------------------------------
 * App identity
 * ------------------------------------------------------------------ */
#define APP_NAME "Weather"

// using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;
using namespace esp_brookesia;

LV_IMG_DECLARE(weather_app_icon_112_112);

namespace esp_brookesia::apps {

WeatherApp *WeatherApp::_instance = nullptr;

WeatherApp *WeatherApp::requestInstance(bool use_status_bar,
                                        bool use_navigation_bar) {
  if (_instance == nullptr) {
    _instance = new WeatherApp(use_status_bar, use_navigation_bar);
  }
  return _instance;
}

WeatherApp::WeatherApp(bool use_status_bar, bool use_navigation_bar)
    : App(APP_NAME, &weather_app_icon_112_112, /*use_default_screen=*/true,
          use_status_bar, use_navigation_bar),
      _json_label(nullptr) {}

WeatherApp::~WeatherApp() {}

bool WeatherApp::init(void) { return true; }

bool WeatherApp::run(void) {
  ESP_UTILS_LOGD("Weather run()");

  esp_http_client_config_t config = {
      .url = "https://api.open-meteo.com/v1/"
             "forecast?latitude=41.38723049750421"
             "&longitude=2.169775940223115&hourly="
             "temperature_2m&current=temperature_2m,apparent_temperature,is_"
             "day,weather_code&forecast_days=1",
      .crt_bundle_attach = esp_crt_bundle_attach,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    ESP_LOGI(ESP_UTILS_LOG_TAG,
             "HTTP GET Status = %d, content_length = %" PRId64,
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
  } else {
    ESP_LOGE(ESP_UTILS_LOG_TAG, "HTTP GET request failed: %s",
             esp_err_to_name(err));
  }

  esp_http_client_cleanup(client);

  lv_obj_t *scr = lv_scr_act();

  /* ---- Background -------------------------------------------- */
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  _json_label = lv_label_create(scr);
  lv_obj_set_style_text_color(_json_label, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(_json_label, &lv_font_montserrat_44, LV_PART_MAIN);

  lv_obj_align(_json_label, LV_ALIGN_CENTER, 0, -30);

  updateDisplay();

  return true;
}

void WeatherApp::updateDisplay(void) {}

bool WeatherApp::back(void) {
  ESP_UTILS_LOGD("Weather back()");
  ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false,
                               "Notify core closed failed");
  return true;
}

ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(systems::base::App, WeatherApp,
                                           APP_NAME, []() {
                                             return std::shared_ptr<WeatherApp>(
                                                 WeatherApp::requestInstance(),
                                                 [](WeatherApp *) {});
                                           })

} // namespace esp_brookesia::apps
