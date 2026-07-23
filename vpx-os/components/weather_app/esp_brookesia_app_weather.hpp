#pragma once

#include "esp_http_client.h"
#include "lvgl.h"
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {

class WeatherApp : public systems::phone::App {
public:
  static WeatherApp *requestInstance(bool use_status_bar = false,
                                     bool use_navigation_bar = false);

  ~WeatherApp();

  using systems::phone::App::endRecordResource;
  using systems::phone::App::startRecordResource;

protected:
  WeatherApp(bool use_status_bar, bool use_navigation_bar);

  bool run(void) override;

  bool back(void) override;

  bool init(void) override;

private:
  static WeatherApp *_instance;

  lv_obj_t *_json_label;
  esp_http_client_handle_t _httpHandler;
  void updateDisplay(void);
};

} // namespace esp_brookesia::apps
