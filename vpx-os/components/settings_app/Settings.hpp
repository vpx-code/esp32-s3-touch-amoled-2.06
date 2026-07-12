#pragma once

#include "brookesia/service_helper/nvs.hpp"
#include "brookesia/service_manager.hpp"
#include "lvgl.h"
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {

class SettingsApp : public systems::phone::App {
public:
  static SettingsApp *requestInstance(bool use_status_bar = false,
                                      bool use_navigation_bar = false);

  ~SettingsApp();

protected:
  SettingsApp(bool use_status_bar, bool use_navigation_bar);

  bool init(void) override;
  bool run(void) override;
  bool back(void) override;

private:
  using NVSHelper = service::helper::NVS;
  void save_to_nvs(const std::string key, uint32_t value);

  static SettingsApp *_instance;
  service::ServiceBinding binding_;
  std::shared_ptr<service::ServiceBase> service_;
};

} // namespace esp_brookesia::apps