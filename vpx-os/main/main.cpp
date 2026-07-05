/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "boost/thread.hpp"
#include "bsp/esp-bsp.h"
#include "esp_brookesia.hpp"
#include "phone/widgets/status_bar/esp_brookesia_status_bar.hpp"
#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "Main"
#include "./dark/stylesheet.hpp"
#include "brookesia/service_helper/wifi.hpp"
#include "brookesia/service_manager.hpp"
#include "esp_lib_utils.h"
#include "esp_wifi.h"

using namespace esp_brookesia;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;
using WifiHelper = service::helper::Wifi;
using NVSHelper = service::helper::NVS; // to test NVS roundtrip

#define LVGL_PORT_INIT_CONFIG()                                                \
  {                                                                            \
    .task_priority = 4, .task_stack = 10 * 1024, .task_affinity = -1,          \
    .task_max_sleep_ms = 500, .timer_period_ms = 5,                            \
  }

#define TOSTR BROOKESIA_DESCRIBE_TO_STR

/* The binding must outlive setUpWiFiService() so we stay bound to the service
 * for the whole program. The subscription itself is made permanent with
 * .release() below, so it needs no storage. */
namespace {
service::ServiceBinding g_wifi_binding;
} // namespace

esp_brookesia::systems::phone::StatusBar::WifiState
getWifiStateFromRSSI(int8_t rssi) {
  return (rssi >= -55)   ? StatusBar::WifiState::SIGNAL_3
         : (rssi >= -70) ? StatusBar::WifiState::SIGNAL_2
         : (rssi >= -85) ? StatusBar::WifiState::SIGNAL_1
                         : StatusBar::WifiState::DISCONNECTED;
}

void updateWifiSignalStrengthIcon(Phone *phone) {
  LvLockGuard gui_guard;

  ESP_UTILS_CHECK_NULL_EXIT(phone, "Invalid phone");
  auto *status_bar = phone->getDisplay().getStatusBar();

  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    ESP_UTILS_LOGD("Wi-Fi RSSI: %d", ap_info.rssi);
    ESP_UTILS_CHECK_FALSE_EXIT(
        status_bar->setWifiIconState(getWifiStateFromRSSI(ap_info.rssi)),
        "Refresh status bar failed");
  } else {
    // handle disconnection or error case
    status_bar->setWifiIconState(StatusBar::WifiState::DISCONNECTED);
  }
}

void getLastConnectedApInfoFromNVS() {
  auto &manager = service::ServiceManager::get_instance();
  auto service = manager.bind(WifiHelper::get_name().data()).get_service();
  ESP_UTILS_CHECK_NULL_EXIT(service, "Wi-Fi service is null");

  std::string nvs_namespace = service->get_attributes().name;
  std::string key = "LastAp";

  auto lastApInfo =
      NVSHelper::get_key_value<WifiHelper::ConnectApInfo>(nvs_namespace, key);

  if (lastApInfo) {
    ESP_UTILS_LOGI("Got this from NVS: %s - %s", lastApInfo->ssid.c_str(),
                   lastApInfo->password.c_str());
    // TODO: now that we have the last connected AP info, we can use it to
    // connect to the Wi-Fi network automatically or display it in the UI.

  } else {
    ESP_UTILS_LOGE("Failed to read back from non-volatile storage: %s",
                   lastApInfo.error().c_str());
    return;
  }
}

void setUpWiFiService(Phone *phone) {
  auto &manager = service::ServiceManager::get_instance();

  if (!manager.start()) {
    ESP_UTILS_LOGE("Failed to start service manager");
    return;
  }

  g_wifi_binding = manager.bind(WifiHelper::get_name().data());

  if (!g_wifi_binding.is_valid()) {
    ESP_UTILS_LOGE("Failed to bind Wi-Fi service");
    return;
  }

  auto service = g_wifi_binding.get_service();
  if (service == nullptr) {
    ESP_UTILS_LOGE("Wi-Fi service is null");
    return;
  }
  /*
  FOR THE FUTURE: WHY THIS WAS WRONG

    service_->subscribe_event(
      TOSTR(WifiHelper::GeneralEvent::Connected),
      [](const std::string &, const service::EventItemMap &) {
        ESP_UTILS_LOGI("FROM MAIN.CPP: Wi-Fi connected");
      });

    This is wrong because the event name is not "Connected". Connected is a
    value, but not an event name.
    The correct event name is "GeneralEventHappened", and the value of the
  event is "Connected". So we need to subscribe to the event
  "GeneralEventHappened" and then check the value of the event to see if it
  is "Connected".
*/

  service
      ->subscribe_event(
          TOSTR(
              WifiHelper::EventId::GeneralEventHappened), // <-- the event NAME
                                                          // (the mailbox)
          [phone](const std::string &, const service::EventItemMap &items) {
            auto it = items.find(
                TOSTR(WifiHelper::EventGeneralEventHappenedParam::
                          Event)); // Search for the envelope: a "letter"
                                   // from WiFi regarding an event
            if (it == items.end())
              return;

            // Open the envelope(s) and check if the letter says
            // "Connected"!
            auto *ev = std::get_if<std::string>(&it->second);
            if (ev && *ev == TOSTR(WifiHelper::GeneralEvent::Connected)) {
              ESP_UTILS_LOGI("Updating status bar: Wi-Fi connected");
              // This runs on the Wi-Fi service task, not the LVGL task,
              // so take the GUI lock before touching the (shared)
              // status bar.
              updateWifiSignalStrengthIcon(phone);
            }
          })
      .release(); // make the subscription permanent; nothing to store
}

extern "C" void app_main(void) {
  ESP_UTILS_LOGI("Display ESP-Brookesia phone demo");

  bsp_display_cfg_t cfg = {
      .lvgl_port_cfg = LVGL_PORT_INIT_CONFIG(),
  };
  ESP_UTILS_CHECK_NULL_EXIT(bsp_display_start_with_config(&cfg),
                            "Start display failed");
  ESP_UTILS_CHECK_ERROR_EXIT(bsp_display_backlight_on(),
                             "Turn on display backlight failed");

  /* Configure GUI lock */
  LvLock::registerCallbacks(
      [](int timeout_ms) {
        if (timeout_ms < 0) {
          timeout_ms = 0;
        } else if (timeout_ms == 0) {
          timeout_ms = 1;
        }
        ESP_UTILS_CHECK_FALSE_RETURN(bsp_display_lock(timeout_ms), false,
                                     "Lock failed");

        return true;
      },
      []() {
        bsp_display_unlock();

        return true;
      });

  /* Create a phone object */
  Phone *phone = new (std::nothrow) Phone();
  ESP_UTILS_CHECK_NULL_EXIT(phone, "Create phone failed");

  /* Try using a stylesheet that corresponds to the resolution */
  if ((BSP_LCD_H_RES == 410) && (BSP_LCD_V_RES == 502)) {
    Stylesheet *stylesheet =
        new (std::nothrow) Stylesheet(STYLESHEET_410_502_DARK);
    ESP_UTILS_CHECK_NULL_EXIT(stylesheet, "Create stylesheet failed");

    ESP_UTILS_LOGI("Using stylesheet (%s)", stylesheet->core.name);
    ESP_UTILS_CHECK_FALSE_EXIT(phone->addStylesheet(stylesheet),
                               "Add stylesheet failed");
    ESP_UTILS_CHECK_FALSE_EXIT(phone->activateStylesheet(stylesheet),
                               "Activate stylesheet failed");
    delete stylesheet;
  }

  // When operating on non-GUI tasks, should acquire a lock before operating
  // on LVGL
  LvLockGuard
      gui_guard; // why does messing up with this cause a stack overflow?

  /* Begin the phone */
  ESP_UTILS_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");
  // assert(phone->getDisplay().showContainerBorder() && "Show container
  // border failed");

  /* Init and install apps from registry */
  std::vector<systems::base::Manager::RegistryAppInfo> inited_apps;
  ESP_UTILS_CHECK_FALSE_EXIT(phone->initAppFromRegistry(inited_apps),
                             "Init app registry failed");
  ESP_UTILS_CHECK_FALSE_EXIT(phone->installAppFromRegistry(inited_apps),
                             "Install app registry failed");

  // Subscribe to Wi-Fi status to update status bar
  setUpWiFiService(phone);

  /* Create a timer to update the clock */
  lv_timer_create(
      [](lv_timer_t *t) {
        time_t now;
        struct tm timeinfo;
        Phone *phone = (Phone *)t->user_data;

        ESP_UTILS_CHECK_NULL_EXIT(phone, "Invalid phone");

        time(&now);
        localtime_r(&now, &timeinfo);

        ESP_UTILS_CHECK_FALSE_EXIT(phone->getDisplay().getStatusBar()->setClock(
                                       timeinfo.tm_hour, timeinfo.tm_min),
                                   "Refresh status bar failed");
      },
      1000, phone);

  /* Create a timer to update Wi-Fi signal strength */
  lv_timer_create(
      [](lv_timer_t *t) {
        Phone *phone = (Phone *)t->user_data;
        updateWifiSignalStrengthIcon(phone);
      },
      5000, phone);
}