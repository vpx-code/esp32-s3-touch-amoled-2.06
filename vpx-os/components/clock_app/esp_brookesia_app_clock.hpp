/*
 * Clock app — minimal Brookesia Phone app that displays local time.
 *
 * Inherit from systems::phone::App to plug into the Brookesia Phone shell.
 * The Phone shell handles the launcher icon, app lifecycle, screen resizing
 * for the status bar, and automatic resource cleanup.
 */
#pragma once

#include "lvgl.h"
#include "systems/phone/esp_brookesia_phone_app.hpp"

namespace esp_brookesia::apps {

class ClockApp : public systems::phone::App {
public:
    /*
     * Brookesia apps use a singleton pattern so the launcher always opens the
     * same instance.  requestInstance() creates it on first call and returns
     * the cached pointer on subsequent calls.
     *
     * use_status_bar:     show the system status bar inside this app
     * use_navigation_bar: show the bottom navigation bar inside this app
     */
    static ClockApp *requestInstance(bool use_status_bar = false,
                                     bool use_navigation_bar = false);

    ~ClockApp();

    /*
     * Expose the resource-recording helpers so the plugin registration lambda
     * (in the .cpp) can wrap animations/timers created outside run().
     * Not needed for this simple app, but included so the class is a complete template.
     */
    using systems::phone::App::startRecordResource;
    using systems::phone::App::endRecordResource;

protected:
    ClockApp(bool use_status_bar, bool use_navigation_bar);

    /*
     * run()  — called each time the user opens the app.
     *          Create all LVGL widgets here.
     *          Brookesia records screens/timers/animations created here and
     *          auto-deletes them when the app exits (enable_recycle_resource).
     */
    bool run(void) override;

    /*
     * back() — called on a back-gesture or back-button press.
     *          Call notifyCoreClosed() to tell Brookesia to close the app.
     */
    bool back(void) override;

    /*
     * init() runs once when the app is installed (at boot, before the user taps the icon).
     * We use it to configure the Spain timezone so localtime_r() always returns CET/CEST,
     * even without a network connection.
     */
    bool init(void) override;

    /*
     * The following overrides are optional.  Uncomment if needed:
     *
     * bool deinit(void) override;       // one-time teardown before uninstall
     * bool pause(void) override;        // app goes to background
     * bool resume(void) override;       // app comes to foreground
     * bool close(void) override;        // app about to close (before cleanup)
     * bool cleanResource(void) override;// manual cleanup for non-recorded resources
     */

private:
    static ClockApp *_instance;

    /* LVGL widget handles — kept as members so the timer callback can reach them. */
    lv_obj_t *_time_label;   // large HH:MM:SS display
    lv_obj_t *_date_label;   // smaller "Weekday, Month DD YYYY" line

    /* Reads the system clock and updates both labels.  Called by the timer. */
    void updateDisplay(void);
};

} // namespace esp_brookesia::apps
