#pragma once
#include <cstdint>
#include <functional>
#include <cmath>

enum class TIMER_MODE : std::uint8_t { 
    ONE_SHOT, 
    REPEATING 
};

class Timer {
public:
  explicit Timer(double seconds = 0.0, TIMER_MODE mode = TIMER_MODE::ONE_SHOT)
    : m_duration(seconds), m_remaining(seconds), m_mode(mode) {}

  Timer(const Timer&) noexcept = default;
  Timer(Timer&&) noexcept = default;
  Timer& operator=(const Timer&) noexcept = default;
  Timer& operator=(Timer&&) noexcept = default;
  
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

  bool update(double dt) {
    m_fired_this_frame = false;
    if (!running() || m_duration <= 0.0) return false;

    m_remaining -= dt;
    // Catch up if we overshot (robust to frame hiccups)
    while (m_remaining <= 0.0) {
      m_fired_this_frame = true;

      if (m_mode == TIMER_MODE::REPEATING) {
        m_remaining += m_duration;            // preserve leftover time
      } else {
        m_running = false;
        m_remaining = 0.0;
        break;
      }
    }
    return m_fired_this_frame;
  }

private:
  double m_duration{1.0};
  double m_remaining{1.0};
  TIMER_MODE m_mode{TIMER_MODE::ONE_SHOT};
  bool m_running{false};
  bool m_paused{false};
  bool m_fired_this_frame{false};
};
