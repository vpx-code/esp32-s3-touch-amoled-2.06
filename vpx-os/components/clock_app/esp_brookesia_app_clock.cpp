/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_brookesia.hpp"
#include "lvgl.h"
#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Clock"
#include "esp_brookesia_app_clock.hpp"
#include "esp_lib_utils.h"

#define APP_NAME "Clock"

using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;

LV_IMG_DECLARE(esp_brookesia_app_icon_launcher_squareline_112_112);

namespace esp_brookesia::apps {

ClockDemo *ClockDemo::_instance = nullptr;

ClockDemo *ClockDemo::requestInstance(bool use_status_bar,
                                      bool use_navigation_bar) {
  if (_instance == nullptr) {
    _instance = new ClockDemo(use_status_bar, use_navigation_bar);
  }
  return _instance;
}

ClockDemo::ClockDemo(bool use_status_bar, bool use_navigation_bar)
    : App(APP_NAME, &esp_brookesia_app_icon_launcher_squareline_112_112, false,
          use_status_bar, use_navigation_bar) {}

ClockDemo::~ClockDemo() {}

bool ClockDemo::run(void) {
  ESP_UTILS_LOGD("Run");

  // Create all UI resources here
  // phone_app_clock_ui_init();

  return true;
}

bool ClockDemo::back(void) {
  ESP_UTILS_LOGD("Back");

  // If the app needs to exit, call notifyCoreClosed() to notify the core to
  // close the app
  ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false,
                               "Notify core closed failed");

  return true;
}

// bool ClockDemo::close(void)
// {
//     ESP_UTILS_LOGD("Close");

//     /* Do some operations here if needed */

//     return true;
// }

// bool ClockDemo::init()
// {
//     ESP_UTILS_LOGD("Init");

//     /* Do some initialization here if needed */

//     return true;
// }

// bool ClockDemo::deinit()
// {
//     ESP_UTILS_LOGD("Deinit");

//     /* Do some deinitialization here if needed */

//     return true;
// }

// bool ClockDemo::pause()
// {
//     ESP_UTILS_LOGD("Pause");

//     /* Do some operations here if needed */

//     return true;
// }

// bool ClockDemo::resume()
// {
//     ESP_UTILS_LOGD("Resume");

//     /* Do some operations here if needed */

//     return true;
// }

// bool ClockDemo::cleanResource()
// {
//     ESP_UTILS_LOGD("Clean resource");

//     /* Do some cleanup here if needed */

//     return true;
// }

ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(systems::base::App, ClockDemo,
                                           APP_NAME, []() {
                                             return std::shared_ptr<ClockDemo>(
                                                 ClockDemo::requestInstance(),
                                                 [](ClockDemo *p) {});
                                           })

} // namespace esp_brookesia::apps
