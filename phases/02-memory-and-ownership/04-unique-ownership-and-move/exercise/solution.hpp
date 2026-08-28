#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <memory>
#include <utility>
#include <vector>

// A device that counts how many currently exist, so the tests can prove nothing
// is leaked and nothing is destroyed twice. Provided for you.
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

// Returns a new device with the given id, owned by the caller.
inline std::unique_ptr<Device> make_device(int id) {
  // TODO: use std::make_unique rather than new.
  (void)id;
  return nullptr;
}

class DeviceRegistry {
 public:
  DeviceRegistry() = default;

  // TODO: refuse copying, and allow moving.
  //
  // You might expect the unique_ptr members to handle this by themselves. They
  // do not, quite: std::vector declares a copy constructor whatever it holds,
  // and only fails when somebody actually instantiates it, so the class still
  // claims to be copyable when asked. Say what you mean:
  //
  //   DeviceRegistry(const DeviceRegistry&) = delete;
  //   DeviceRegistry& operator=(const DeviceRegistry&) = delete;
  //   DeviceRegistry(DeviceRegistry&&) = default;
  //   DeviceRegistry& operator=(DeviceRegistry&&) = default;

  // Takes ownership of a device. A null pointer is ignored rather than stored.
  void add(std::unique_ptr<Device> device) {
    // TODO: the parameter owns the device now, and the registry needs to take
    // it from the parameter. A unique_ptr cannot be copied into the vector.
    (void)device;
  }

  // Removes the device with this id and hands ownership to the caller.
  // Returns nullptr when there is no such device.
  std::unique_ptr<Device> take(int id) {
    // TODO
    (void)id;
    return nullptr;
  }

  // A device to look at, without taking it. Returns nullptr when absent.
  // A raw pointer is the right return type here: it says look but do not keep.
  Device* find(int id) {
    // TODO
    (void)id;
    return nullptr;
  }

  int count() const { return static_cast<int>(devices_.size()); }

 private:
  std::vector<std::unique_ptr<Device>> devices_;
};

#endif  // LESSON_SOLUTION_HPP
