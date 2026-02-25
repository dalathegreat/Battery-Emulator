#include <atomic>

class Watchdog {
 public:
  // Call this periodically to reset the watchdog timer. The timer is inactive
  // until the first call.
  void update();
  // Panic if the time since the last update exceeds timeout_ms. If update has
  // never been called, this does nothing.
  void panic_if_exceeded_ms(uint32_t timeout_ms, const char* message);

 private:
  std::atomic<unsigned long> last_update_time_ms = 0;
};
