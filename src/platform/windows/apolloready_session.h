#pragma once

#include <cstdint>
#include <mutex>

namespace apolloready {
  enum class phase { idle, waiting, ready, closing };
  struct session {
    std::uint64_t generation = 0;
    std::uint64_t started = 0;  // Windows FILETIME, set before launching commands
    std::int32_t app_id = 0;
    bool active = false;
    std::uint32_t launched_process_id = 0;
    std::uintptr_t window = 0;
    std::uint32_t process_id = 0;
    std::uint64_t process_started = 0;
    phase status = phase::idle;
  };

  // Capture reinitialization must not select another app after the game closes.
  // A launch generation also prevents stale frames/callbacks changing a new session.
  class session_store {
    std::mutex mutex;
    session current;
  public:
    session read() {
      std::lock_guard lock(mutex);
      return current;
    }
    void begin(std::uint64_t started, std::int32_t app_id = 0) {
      std::lock_guard lock(mutex);
      auto generation = current.generation + 1;
      current = {};
      current.generation = generation;
      current.started = started;
      current.app_id = app_id;
      current.active = true;
      current.status = phase::waiting;
    }
    void end() {
      std::lock_guard lock(mutex);
      current.active = false;
      current.status = phase::closing;
      ++current.generation;
    }
    bool track_launch(std::uint64_t generation, std::uint32_t process_id) {
      std::lock_guard lock(mutex);
      if (generation != current.generation || !current.active ||
          current.status == phase::closing || !process_id ||
          (current.launched_process_id && current.launched_process_id != process_id)) return false;
      current.launched_process_id = process_id;
      return true;
    }
    bool bind(std::uint64_t generation, std::uintptr_t window,
              std::uint32_t process_id, std::uint64_t process_started) {
      std::lock_guard lock(mutex);
      if (generation != current.generation || !current.active ||
          current.status == phase::closing || current.window || !window ||
          !process_id || process_started < current.started) return false;
      current.window = window;
      current.process_id = process_id;
      current.process_started = process_started;
      return true;
    }
    bool publish(std::uint64_t generation, phase status) {
      std::lock_guard lock(mutex);
      if (generation != current.generation || !current.active || current.status == phase::closing) return false;
      if (status == phase::ready && !current.window) return false;
      current.status = status;
      return true;
    }
  };
  inline session_store sessions;
}
