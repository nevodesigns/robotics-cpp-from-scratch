#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <memory>
#include <utility>
#include <vector>

class Device {
 public:
  explicit Device(int id) : id_(id) { ++live(); }
  ~Device() { --live(); }

  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

  int id() const { return id_; }

  static int& live() {
    static int count = 0;
    return count;
  }

 private:
  int id_ = 0;
};

inline std::unique_ptr<Device> make_device(int id) {
  // make_unique rather than new: one call, exception safe, and the word new
  // never appears. Returning by value moves the pointer out, so ownership goes
  // to the caller and the signature says so.
  return std::make_unique<Device>(id);
}

class DeviceRegistry {
 public:
  DeviceRegistry() = default;

  // Say it explicitly, even though a vector of unique_ptr cannot in fact be
  // copied. The trait std::is_copy_constructible still reports true for this
  // class without these lines, because std::vector declares a copy constructor
  // unconditionally and only fails when somebody instantiates it. Writing the
  // decision down makes the type honest about itself, and moves the error from
  // a page of template output to one clear line.
  DeviceRegistry(const DeviceRegistry&) = delete;
  DeviceRegistry& operator=(const DeviceRegistry&) = delete;

  DeviceRegistry(DeviceRegistry&&) = default;
  DeviceRegistry& operator=(DeviceRegistry&&) = default;

  void add(std::unique_ptr<Device> device) {
    // Refusing a null keeps every later loop free of null checks. Decide once,
    // at the boundary, rather than everywhere afterwards.
    if (device == nullptr) return;

    // The parameter owns the device. push_back cannot copy a unique_ptr, so the
    // ownership is moved into the vector and the parameter is left empty.
    devices_.push_back(std::move(device));
  }

  std::unique_ptr<Device> take(int id) {
    for (std::size_t i = 0; i < devices_.size(); ++i) {
      if (devices_[i]->id() != id) continue;

      // Move the ownership out before erasing, otherwise erase would destroy
      // the device we are trying to hand over.
      std::unique_ptr<Device> taken = std::move(devices_[i]);
      devices_.erase(devices_.begin() + static_cast<std::ptrdiff_t>(i));
      return taken;
    }
    return nullptr;
  }

  Device* find(int id) {
    for (const std::unique_ptr<Device>& device : devices_) {
      if (device->id() == id) return device.get();
    }
    return nullptr;
  }

  int count() const { return static_cast<int>(devices_.size()); }

 private:
  std::vector<std::unique_ptr<Device>> devices_;
};

#endif  // LESSON_SOLUTION_HPP
