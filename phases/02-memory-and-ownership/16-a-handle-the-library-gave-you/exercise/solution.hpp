#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <memory>

// ---------------------------------------------------------------------------
// A C library, pretended. This is what the library gave you, and it is given.
//
// An opaque device, an open that can fail, a read, and a close that must be called exactly once on
// everything that was opened. The counters say what was done, so the test can see leaks and
// double closes that would otherwise be silent.
// ---------------------------------------------------------------------------
struct Device {
  int id;
};

struct DeviceLog {
  int opened = 0;
  int closed = 0;
  int bad_closes = 0;    // a null, or a device that was already closed
  int live = 0;
};

inline DeviceLog& device_log() {
  static DeviceLog log;
  return log;
}

inline void reset_device_log() { device_log() = DeviceLog{}; }

// Fails for a negative id, as opening a port that does not exist would.
inline Device* device_open(int id) {
  if (id < 0) return nullptr;
  Device* d = new Device{id};
  ++device_log().opened;
  ++device_log().live;
  return d;
}

inline double device_read(Device* d) { return d == nullptr ? -1.0 : d->id * 0.5; }

inline void device_close(Device* d) {
  if (d == nullptr || d->id == -1) {
    ++device_log().bad_closes;
    return;
  }
  d->id = -1;           // closed: any second close is noticed
  ++device_log().closed;
  --device_log().live;
  delete d;
}

// ---------------------------------------------------------------------------
// What you write.
// ---------------------------------------------------------------------------

// The close that unique_ptr will call.
struct DeviceCloser {
  void operator()(Device* d) const {
    // TODO: call device_close(d), but only when d is not null. unique_ptr never calls it for a null
    // pointer, and a null passed to device_close is counted as a bad close.
    (void)d;
  }
};

using DevicePtr = std::unique_ptr<Device, DeviceCloser>;

// Open a device and own it from the first moment. An empty DevicePtr means the open failed.
inline DevicePtr open_checked(int id) {
  // TODO: hand the pointer from device_open straight to a DevicePtr in the same statement.
  (void)id;
  return DevicePtr();
}

// Read two devices and add them up. False, and nothing owned left behind, if either cannot be opened.
inline bool read_pair(int first_id, int second_id, double* sum) {
  // TODO: open both with open_checked. If the first fails return false. If the second fails return
  // false: the first closes itself. Otherwise *sum = device_read(first.get()) + device_read(second.get())
  // and return true. No call to device_close anywhere in this function.
  (void)first_id;
  (void)second_id;
  (void)sum;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
