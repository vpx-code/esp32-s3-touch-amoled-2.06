// esp_err.h must precede the BSP header, which uses esp_err_t without including
// it. The blank line keeps clang-format from reordering the two.
#include "esp_err.h"

#include "WifiPage.hpp"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "lvgl.h"

#include "../main/dark/theme_constants.hpp"
#include "boost/json.hpp"

using esp_brookesia::systems::theme::COLOR_PURE_WHITE;

#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:WifiPage"
#include "esp_lib_utils.h"

#include <string>

/* Shorthand for the (verbose) enum-to-string macro used everywhere below. */
#define TOSTR BROOKESIA_DESCRIBE_TO_STR

namespace esp_brookesia::apps {

namespace {

/* The device's provisioning access point. The phone joins this, then the
 * captive portal pops up for entering the real network + password. */
constexpr const char *kSoftApSsid = "VPX_WATCH";
constexpr const char *kSoftApPassword = "12345678"; // WPA2 needs >= 8 chars

/* RAII lock for the LVGL/display mutex. Use when touching LVGL objects from a
 * thread other than the LVGL task (i.e. from Wi-Fi service event callbacks). */
struct LvGuard {
  LvGuard() {
    bsp_display_lock(0); // 0 == wait forever
  }
  ~LvGuard() { bsp_display_unlock(); }
};

} // namespace

void WifiPage::create(lv_obj_t *page) {
  /* Status line at the top of the page. */
  status_label_ = lv_label_create(page);
  lv_obj_set_style_text_font(status_label_, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(status_label_, lv_color_hex(COLOR_PURE_WHITE), 0);
  lv_label_set_text(status_label_, "Starting Wi-Fi...");

  /* Centered instructions the user reads while setting up from their phone. */
  info_label_ = lv_label_create(page);
  lv_obj_set_width(info_label_, lv_pct(90));
  lv_obj_set_style_text_font(info_label_, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(info_label_, lv_color_hex(COLOR_PURE_WHITE), 0);
  lv_obj_set_style_text_align(info_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(info_label_, LV_LABEL_LONG_WRAP);
  lv_obj_align(info_label_, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(info_label_, "Preparing phone setup...");

  bringUp();
}

void WifiPage::bringUp() {
  auto &manager = service::ServiceManager::get_instance();
  if (!manager.start()) {
    ESP_UTILS_LOGE("Failed to start service manager");
    LvGuard lock;
    setStatus("Wi-Fi unavailable (manager)");
    return;
  }

  binding_ = manager.bind(WifiHelper::get_name().data());
  if (!binding_.is_valid()) {
    ESP_UTILS_LOGE("Failed to bind Wi-Fi service");
    LvGuard lock;
    setStatus("Wi-Fi unavailable (bind)");
    return;
  }
  service_ = binding_.get_service();
  if (service_ == nullptr) {
    ESP_UTILS_LOGE("Wi-Fi service is null");
    LvGuard lock;
    setStatus("Wi-Fi unavailable (service)");
    return;
  }

  /* Subscribe before driving the state machine so we don't miss events. */
  connections_.push_back(service_->subscribe_event(
      TOSTR(WifiHelper::EventId::GeneralEventHappened),
      [this](const std::string &, const service::EventItemMap &items) {
        onGeneralEvent(items);
      }));
  connections_.push_back(service_->subscribe_event(
      TOSTR(WifiHelper::EventId::SoftApEventHappened),
      [this](const std::string &, const service::EventItemMap &items) {
        onSoftApEvent(items);
      }));

  /* Kick off Init; Start and provisioning are chained off the events. */
  callAction(WifiHelper::GeneralAction::Init);
}

void WifiPage::callAction(WifiHelper::GeneralAction action) {
  if (service_ == nullptr) {
    return;
  }
  service_->call_function_async(
      TOSTR(WifiHelper::FunctionId::TriggerGeneralAction),
      boost::json::object{
          {TOSTR(WifiHelper::FunctionTriggerGeneralActionParam::Action),
           TOSTR(action)}});
}

void WifiPage::startProvisioning() {
  if (service_ == nullptr || provisioning_started_) {
    return;
  }
  provisioning_started_ = true;

  /* Name the device's provisioning AP via SetSoftApParams (its only param is
   * "Param", a nested object matching the SoftApParams struct), THEN start
   * provisioning (which itself takes no parameters). */
  boost::json::object softap_params;
  softap_params["ssid"] = kSoftApSsid;
  softap_params["password"] = kSoftApPassword;

  service_->call_function_async(
      TOSTR(WifiHelper::FunctionId::SetSoftApParams),
      boost::json::object{
          {TOSTR(WifiHelper::FunctionSetSoftApParamsParam::Param),
           softap_params}});
  service_->call_function_async(
      TOSTR(WifiHelper::FunctionId::TriggerSoftApProvisionStart),
      boost::json::object{});
}

void WifiPage::onGeneralEvent(const service::EventItemMap &items) {
  auto it =
      items.find(TOSTR(WifiHelper::EventGeneralEventHappenedParam::Event));
  if (it == items.end()) {
    return;
  }
  auto *event = std::get_if<std::string>(&it->second);
  if (event == nullptr) {
    return;
  }
  ESP_UTILS_LOGI("Wi-Fi general event: %s", event->c_str());

  if (*event == TOSTR(WifiHelper::GeneralEvent::Inited)) {
    callAction(WifiHelper::GeneralAction::Start);
  } else if (*event == TOSTR(WifiHelper::GeneralEvent::Started)) {
    /* Wi-Fi is up; begin phone provisioning. */
    startProvisioning();
  } else if (*event == TOSTR(WifiHelper::GeneralEvent::Connected)) {
    LvGuard lock;
    setStatus("Connected");
    if (info_label_ != nullptr) {
      lv_label_set_text(info_label_, "Your device is now on Wi-Fi.");
    }
  } else if (*event == TOSTR(WifiHelper::GeneralEvent::Disconnected)) {
    LvGuard lock;
    setStatus("Disconnected");
  }
}

void WifiPage::onSoftApEvent(const service::EventItemMap &items) {
  auto it = items.find(TOSTR(WifiHelper::EventSoftApEventHappenedParam::Event));
  if (it == items.end()) {
    return;
  }
  auto *event = std::get_if<std::string>(&it->second);
  if (event == nullptr) {
    return;
  }
  ESP_UTILS_LOGI("Wi-Fi SoftAP event: %s", event->c_str());

  /* One lock for the whole handler; create the popup ONLY on Started, keep its
   * handle in a member, and close that exact popup on Stopped. */
  LvGuard lock;

  if (*event == TOSTR(WifiHelper::SoftApEvent::Started)) {
    setStatus("Ready for phone setup");

    const std::string instructions =
        std::string("On your phone, join Wi-Fi network:\n\n\"") + kSoftApSsid +
        "\"\n\nThen follow the page that opens.";
    if (info_label_ != nullptr) {
      lv_label_set_text(info_label_, instructions.c_str());
    }
  } else if (*event == TOSTR(WifiHelper::SoftApEvent::Stopped)) {
    setStatus("Setup ended");
  }
}

void WifiPage::setStatus(const std::string &text) {
  if (status_label_ != nullptr) {
    lv_label_set_text(status_label_, text.c_str());
  }
}

} // namespace esp_brookesia::apps

#undef TOSTR
