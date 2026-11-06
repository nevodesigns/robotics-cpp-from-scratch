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

// The close that unique_ptr will call. A unique_ptr normally calls delete, which is wrong for a handle
// the library made: its memory belongs to the library, and only its own close releases it properly.
struct DeviceCloser {
  void operator()(Device* d) const {
    if (d != nullptr) device_close(d);
  }
};

using DevicePtr = std::unique_ptr<Device, DeviceCloser>;

// Open a device and own it from the first moment. An empty DevicePtr means the open failed.
inline DevicePtr open_checked(int id) { return DevicePtr(device_open(id)); }

// Read two devices and add them up. False, and nothing owned left behind, if either cannot be opened.
//
// The early return is where a raw handle leaks: the first device was opened, the second was not, and
// the return skips the close of the first.
inline bool read_pair(int first_id, int second_id, double* sum) {
  DevicePtr first = open_checked(first_id);
  if (!first) return false;
  DevicePtr second = open_checked(second_id);
  if (!second) return false;   // first is closed here by its destructor, with nothing written
  *sum = device_read(first.get()) + device_read(second.get());
  return true;
}

#endif  // LESSON_SOLUTION_HPP
