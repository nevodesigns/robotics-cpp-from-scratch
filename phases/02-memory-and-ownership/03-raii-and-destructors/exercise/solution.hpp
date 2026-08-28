#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <set>

// ---------------------------------------------------------------------------
// A fake device, provided for you.
//
// Shaped like a real driver API on purpose: it hands out integer ids, it
// complains about nothing, and it will happily let you close the same id twice
// or never close it at all. Making that safe is your job, not the device's.
// ---------------------------------------------------------------------------

inline constexpr int kNoDevice = -1;

inline std::set<int>& fake_open_ids() {
  static std::set<int> ids;
  return ids;
}

inline int fake_open() {
  static int next_id = 1;
  const int id = next_id++;
  fake_open_ids().insert(id);
  return id;
}

inline void fake_close(int id) { fake_open_ids().erase(id); }

inline bool fake_is_open(int id) { return fake_open_ids().count(id) > 0; }

inline int fake_open_count() { return static_cast<int>(fake_open_ids().size()); }

// ---------------------------------------------------------------------------
// Your work starts here.
// ---------------------------------------------------------------------------

class DeviceHandle {
 public:
  // TODO: open a device and remember its id.
  DeviceHandle() {}

  // TODO: close it, but only if this handle actually holds one.
  ~DeviceHandle() {}

  // TODO: refuse copying. Two handles holding one id would close it twice, and
  // a double release is worse than a leak.
  //
  // The syntax you need is:
  //   DeviceHandle(const DeviceHandle&) = delete;
  //   DeviceHandle& operator=(const DeviceHandle&) = delete;

  bool is_open() const {
    // TODO
    return false;
  }

  int id() const {
    // TODO: the id held, or kNoDevice when this handle holds none.
    return kNoDevice;
  }

  // Closes early. Must be safe to call more than once, so that calling it and
  // then letting the destructor run does not close the device twice.
  void close() {
    // TODO
  }

 private:
  int id_ = kNoDevice;
};

#endif  // LESSON_SOLUTION_HPP
