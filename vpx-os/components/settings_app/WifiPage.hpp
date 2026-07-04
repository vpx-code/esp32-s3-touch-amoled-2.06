#pragma once

#include "lvgl.h"

#include "brookesia/service_helper/nvs.hpp"
#include "brookesia/service_helper/wifi.hpp"
#include "brookesia/service_manager.hpp"

#include <memory>
#include <string>

namespace esp_brookesia::apps {

/**
 * WifiPage
 *
 * Phone-driven Wi-Fi setup. There is no on-device network list or keyboard:
 * the page brings the Wi-Fi service up and starts SoftAP provisioning, so the
 * user connects their phone to the device's access point and enters the target
 * network + password in the captive-portal page that pops up on the phone.
 *
 * Threading: the Wi-Fi service delivers events on its own task, NOT the LVGL
 * task. Every LVGL mutation triggered from an event callback is wrapped in the
 * display lock.
 */
class WifiPage {
public:
  /**
   * Build the UI into `page` and start the bring-up -> provisioning chain.
   * `page` must outlive this object (it does: the app owns both).
   */
  void create(lv_obj_t *page);

private:
  using WifiHelper = service::helper::Wifi;
  using NVSHelper = service::helper::NVS;

  void bringUp();
  void callAction(WifiHelper::GeneralAction action);
  void startProvisioning();

  /* Event handlers — invoked on the Wi-Fi service task. */
  void onGeneralEvent(const service::EventItemMap &items);
  void onSoftApEvent(const service::EventItemMap &items);

  /* setStatus assumes the caller holds the LVGL lock. */
  void setStatus(const std::string &text);

  service::ServiceBinding binding_;
  std::shared_ptr<service::ServiceBase> service_;
  std::vector<service::EventRegistry::SignalConnection> connections_;

  lv_obj_t *status_label_ = nullptr; /*!< Top status line. */
  lv_obj_t *info_label_ =
      nullptr; /*!< Instructions shown while provisioning. */
  lv_obj_t *softap_popup_ = nullptr; /*!< SoftAP-provisioning message box. */

  bool provisioning_started_ = false; /*!< Guards against re-triggering. */
};

} // namespace esp_brookesia::apps
