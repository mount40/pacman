#pragma once
#include <functional>
#include <cmath>

// NOTE: Refactor to naming convention, check copy move semantics
enum class TimerMode { OneShot, Repeating };

class Timer {
 public:
  explicit Timer(double seconds = 1.0, TimerMode mode = TimerMode::OneShot)
    : m_duration(seconds), m_remaining(seconds), m_mode(mode) {}

  void start()  { m_running = true; m_paused = false; }
  void stop()   { m_running = false; }
  void pause()  { m_paused = true; }
  void resume() { m_paused = false; }
  void reset()  { m_remaining = m_duration; m_fired_this_frame = false; }

  void set_duration(double seconds) {
    m_duration = seconds;
    // Keep remaining consistent if not running yet.
    if (!m_running) m_remaining = m_duration;
  }

  double duration()  const { return m_duration; }
  double remaining() const { return m_remaining; }
  bool   running()   const { return m_running && !m_paused; }

  // Returns true on any frame the timer fires. If repeating, it can fire multiple times
  // when dt is large; this will still return true (at least once).
  bool update(double dt) {
    m_fired_this_frame = false;
    if (!running() || m_duration <= 0.0) return false;

    m_remaining -= dt;
    // Catch up if we overshot (robust to frame hiccups)
    while (m_remaining <= 0.0) {
      m_fired_this_frame = true;
      if (on_tick) on_tick();

      if (m_mode == TimerMode::Repeating) {
        m_remaining += m_duration;            // preserve leftover time
      } else {
        m_running = false;
        m_remaining = 0.0;
        break;
      }
    }
    return m_fired_this_frame;
  }

  // Optional callback invoked on fire
  std::function<void()> on_tick;

 private:
  double m_duration{1.0};
  double m_remaining{1.0};
  TimerMode m_mode{TimerMode::OneShot};
  bool m_running{false};
  bool m_paused{false};
  bool m_fired_this_frame{false};
};
