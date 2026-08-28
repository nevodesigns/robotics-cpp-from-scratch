#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <set>

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

class DeviceHandle {
 public:
  // Acquire in the constructor. After this line runs, the object either holds a
  // device or does not exist, and there is no partially built state for anybody
  // else to observe.
  DeviceHandle() : id_(fake_open()) {}

  // Release in the destructor. This runs on a normal return, an early return, a
  // break, and while an exception unwinds the stack. It also runs on the exit
  // path somebody adds next year without reading this file, which is the whole
  // point.
  ~DeviceHandle() { close(); }

  // Copying is refused. Two handles holding one id would each close it, and a
  // double release can shut a resource that another part of the program has
  // since opened for something else. Deleting the operation reports the mistake
  // at the line that attempts the copy, which is where it actually is.
  DeviceHandle(const DeviceHandle&) = delete;
  DeviceHandle& operator=(const DeviceHandle&) = delete;

  bool is_open() const { return id_ != kNoDevice; }
  int id() const { return id_; }

  // Closing marks the handle empty before doing anything else, so a second call
  // has nothing left to do. That makes close() and the destructor safe together,
  // in either order and any number of times.
  void close() {
    if (id_ == kNoDevice) return;
    const int held = id_;
    id_ = kNoDevice;
    fake_close(held);
  }

 private:
  int id_ = kNoDevice;
};

#endif  // LESSON_SOLUTION_HPP
