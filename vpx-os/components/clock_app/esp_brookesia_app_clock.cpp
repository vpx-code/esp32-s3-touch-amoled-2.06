/*
 * Clock app — minimal Brookesia app showing the local time.
 *
 * How it works:
 *   1. run() creates two LVGL labels on the default screen.
 *   2. An lv_timer fires every second and calls updateDisplay().
 *   3. updateDisplay() reads the POSIX clock and formats HH:MM:SS + date
 * string.
 *   4. When the user presses back(), notifyCoreClosed() tells the Phone shell
 *      to close the app.  Brookesia then auto-deletes the screen and the timer.
 *
 * NOTE: time() returns 0 (Unix epoch, 1970-01-01 00:00:00) until an SNTP sync
 * sets the real-world clock.  Add an SNTP component to get accurate time.
 */

#include <cstdio>
#include <ctime>

#include "esp_brookesia.hpp"
#include "lvgl.h"

/* Redefine the log tag so serial output shows "BS:Clock" instead of "Main" */
#ifdef ESP_UTILS_LOG_TAG
#undef ESP_UTILS_LOG_TAG
#endif
#define ESP_UTILS_LOG_TAG "BS:Clock"
#include "esp_lib_utils.h"

#include "esp_brookesia_app_clock.hpp"

/* ------------------------------------------------------------------
 * App identity
 * ------------------------------------------------------------------ */
#define APP_NAME "Clock"

using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems;

/* Launcher icon — 112×112 pixel image stored as a C array.
 * Copied from brookesia_app_squareline_demo/assets/ as a temporary placeholder.
 * Replace this with your own converted PNG when you have a real clock icon. */
LV_IMG_DECLARE(clock_app_icon_112_112);

namespace esp_brookesia::apps {

/* ------------------------------------------------------------------
 * Singleton bookkeeping
 * ------------------------------------------------------------------ */
ClockApp *ClockApp::_instance = nullptr;

ClockApp *ClockApp::requestInstance(bool use_status_bar,
                                    bool use_navigation_bar) {
  if (_instance == nullptr) {
    _instance = new ClockApp(use_status_bar, use_navigation_bar);
  }
  return _instance;
}

/* ------------------------------------------------------------------
 * Constructor
 *
 * Passes to the phone::App base:
 *   name              — shown in recents screen and task manager
 *   launcher_icon     — the 112×112 icon shown in the launcher grid;
 *                       currently using the squareline placeholder icon
 *   use_default_screen — true: Brookesia creates + loads a blank screen before
 *                        calling run(); we just call lv_scr_act() to get it.
 *                        false: we must create and lv_scr_load() our own
 * screen. use_status_bar    — passed through from requestInstance()
 *   use_navigation_bar — passed through from requestInstance()
 * ------------------------------------------------------------------ */
ClockApp::ClockApp(bool use_status_bar, bool use_navigation_bar)
    : App(APP_NAME, &clock_app_icon_112_112, /*use_default_screen=*/true,
          use_status_bar, use_navigation_bar),
      _time_label(nullptr), _date_label(nullptr) {}

ClockApp::~ClockApp() {}

/* ------------------------------------------------------------------
 * run()
 *
 * Called by the Phone shell every time the user opens the app.
 * Because use_default_screen=true, the screen already exists and is
 * active — we just add widgets to it.
 *
 * Everything created here (the screen, the timer) is automatically
 * recorded by Brookesia (enable_recycle_resource=true by default) and
 * deleted when the app exits.  No manual cleanup needed.
 * ------------------------------------------------------------------ */
/* ------------------------------------------------------------------
 * init()
 *
 * Called once at install time (boot), before the user ever opens the app.
 * Sets the process-wide timezone to Spain: CET (UTC+1 winter) / CEST (UTC+2
 * summer). localtime_r() uses this automatically — no NTP or internet needed
 * for the offset.
 * ------------------------------------------------------------------ */
bool ClockApp::init(void) {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  return true;
}

bool ClockApp::run(void) {
  ESP_UTILS_LOGD("Clock run()");

  lv_obj_t *scr = lv_scr_act();

  /* ---- Background -------------------------------------------- */
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  /* ---- Time label (large, centered) -------------------------- */
  /*
   * Montserrat fonts 8–44 are all compiled in (see sdkconfig).
   * Pick the largest one that fits the 410px-wide screen.
   * lv_font_montserrat_44 renders "00:00:00" at roughly 280px wide.
   */
  _time_label = lv_label_create(scr);
  lv_obj_set_style_text_color(_time_label, lv_color_hex(0xFFFFFF),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(_time_label, &lv_font_montserrat_44, LV_PART_MAIN);
  /* Place it slightly above centre to leave room for the date line */
  lv_obj_align(_time_label, LV_ALIGN_CENTER, 0, -30);

  /* ---- Date label (small, below time) ------------------------ */
  _date_label = lv_label_create(scr);
  lv_obj_set_style_text_color(_date_label, lv_color_hex(0x888888),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(_date_label, &lv_font_montserrat_20, LV_PART_MAIN);
  lv_obj_align(_date_label, LV_ALIGN_CENTER, 0, 40);

  /* ---- Show current time immediately (no blank flash) -------- */
  updateDisplay();

  /* ---- 1-second refresh timer -------------------------------- */
  /*
   * lv_timer_create(callback, period_ms, user_data)
   *
   * The lambda captures nothing — it receives `this` through user_data.
   * The timer is created inside run(), so Brookesia records it and will
   * call lv_timer_delete() automatically when the app closes.
   */
  lv_timer_create(
      [](lv_timer_t *t) {
        static_cast<ClockApp *>(t->user_data)->updateDisplay();
      },
      1000, this);

  return true;
}

/* ------------------------------------------------------------------
 * updateDisplay()
 *
 * Reads the POSIX system clock and writes formatted strings into the
 * two LVGL labels.  Called once in run() and then every second by the timer.
 * ------------------------------------------------------------------ */
void ClockApp::updateDisplay(void) {
  time_t now;
  struct tm ti;

  /* time() returns seconds since Unix epoch.
   * It stays at 0 until SNTP syncs the real-world clock. */
  time(&now);
  localtime_r(&now, &ti); /* thread-safe conversion to local struct tm */

  /* HH:MM:SS */
  char time_buf[16];
  snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", ti.tm_hour, ti.tm_min,
           ti.tm_sec);
  lv_label_set_text(_time_label, time_buf);

  /* "Friday, June 27 2026" — strftime handles locale-independent formatting */
  char date_buf[40];
  strftime(date_buf, sizeof(date_buf), "%A, %B %d %Y", &ti);
  lv_label_set_text(_date_label, date_buf);
}

/* ------------------------------------------------------------------
 * back()
 *
 * Called on a swipe-back gesture or hardware back button.
 * notifyCoreClosed() signals the Phone shell to run the close sequence:
 *   close() (optional) → resource cleanup → deinit() (optional)
 * ------------------------------------------------------------------ */
bool ClockApp::back(void) {
  ESP_UTILS_LOGD("Clock back()");
  ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false,
                               "Notify core closed failed");
  return true;
}

/* ------------------------------------------------------------------
 * Plugin registration
 *
 * This macro runs at static-initialisation time (before app_main).
 * It inserts a factory entry into the global App registry.
 * main.cpp calls phone->initAppFromRegistry() to discover all entries.
 *
 * The deleter is a no-op because ClockApp is a singleton managed by
 * _instance — we do not want shared_ptr to delete it.
 *
 * IMPORTANT: the component's CMakeLists.txt must use WHOLE_ARCHIVE or
 * the linker will discard this translation unit and the app will never
 * appear in the launcher.
 * ------------------------------------------------------------------ */
ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(systems::base::App, ClockApp,
                                           APP_NAME, []() {
                                             return std::shared_ptr<ClockApp>(
                                                 ClockApp::requestInstance(),
                                                 [](ClockApp *) {});
                                           })

} // namespace esp_brookesia::apps
