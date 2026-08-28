#include <rc/test/rc_test.hpp>

#include <memory>
#include <type_traits>
#include <utility>

#include "solution.hpp"

RC_TEST("a factory hands back a device the caller owns") {
  const int before = Device::live();
  {
    const std::unique_ptr<Device> device = make_device(7);
    RC_REQUIRE(device != nullptr);
    RC_CHECK_EQ(device->id(), 7);
    RC_CHECK_EQ(Device::live(), before + 1);
  }
  RC_CHECK_EQ(Device::live(), before);
}

RC_TEST("a registry keeps what it is given alive") {
  const int before = Device::live();
  {
    DeviceRegistry registry;
    registry.add(make_device(1));
    registry.add(make_device(2));
    RC_CHECK_EQ(registry.count(), 2);
    RC_CHECK_EQ(Device::live(), before + 2);
  }
  RC_CHECK_EQ(Device::live(), before);
}

RC_TEST("a registry ignores a null rather than storing it") {
  DeviceRegistry registry;
  registry.add(nullptr);
  RC_CHECK_EQ(registry.count(), 0);
}

RC_TEST("adding moves the caller's pointer, leaving it empty") {
  DeviceRegistry registry;
  std::unique_ptr<Device> device = make_device(3);
  registry.add(std::move(device));

  // After moving from it, the caller's pointer holds nothing. This is the one
  // observation you are allowed to make about a moved from object.
  RC_CHECK(device == nullptr);
  RC_CHECK_EQ(registry.count(), 1);
}

RC_TEST("finding looks without taking") {
  DeviceRegistry registry;
  registry.add(make_device(4));

  Device* seen = registry.find(4);
  RC_REQUIRE(seen != nullptr);
  RC_CHECK_EQ(seen->id(), 4);

  // Looking must not remove it.
  RC_CHECK_EQ(registry.count(), 1);
}

RC_TEST("finding something absent answers nothing") {
  DeviceRegistry registry;
  registry.add(make_device(4));
  RC_CHECK(registry.find(99) == nullptr);
}

RC_TEST("taking transfers ownership out and the device stays alive") {
  const int before = Device::live();
  DeviceRegistry registry;
  registry.add(make_device(5));
  registry.add(make_device(6));

  std::unique_ptr<Device> taken = registry.take(5);
  RC_REQUIRE(taken != nullptr);
  RC_CHECK_EQ(taken->id(), 5);

  // The registry gave it up, and the device is still alive because the caller
  // now owns it. Both halves matter.
  RC_CHECK_EQ(registry.count(), 1);
  RC_CHECK_EQ(Device::live(), before + 2);
}

RC_TEST("taking something absent answers nothing and changes nothing") {
  DeviceRegistry registry;
  registry.add(make_device(8));
  RC_CHECK(registry.take(99) == nullptr);
  RC_CHECK_EQ(registry.count(), 1);
}

RC_TEST("a taken device is destroyed when the caller lets go") {
  const int before = Device::live();
  {
    DeviceRegistry registry;
    registry.add(make_device(9));
    const std::unique_ptr<Device> taken = registry.take(9);
    RC_CHECK_EQ(Device::live(), before + 1);
  }
  RC_CHECK_EQ(Device::live(), before);
}

RC_TEST("everything is released even when devices are added and taken repeatedly") {
  const int before = Device::live();
  {
    DeviceRegistry registry;
    for (int id = 0; id < 20; ++id) registry.add(make_device(id));
    for (int id = 0; id < 20; id += 2) {
      const std::unique_ptr<Device> taken = registry.take(id);
      RC_CHECK(taken != nullptr);
    }
    RC_CHECK_EQ(registry.count(), 10);
  }
  RC_CHECK_EQ(Device::live(), before);
}

RC_TEST("a registry says out loud that it cannot be copied") {
  // A vector of unique_ptr does not make this true on its own: std::vector
  // declares a copy constructor whatever it holds, and only fails when it is
  // instantiated. The class has to state the decision for the trait to be
  // right, which is what makes the error land on the offending line rather than
  // deep inside the standard library.
  RC_CHECK(!std::is_copy_constructible<DeviceRegistry>::value);
  RC_CHECK(!std::is_copy_assignable<DeviceRegistry>::value);
}

RC_TEST("a registry can still be moved") {
  const int before = Device::live();
  {
    DeviceRegistry source;
    source.add(make_device(11));
    source.add(make_device(12));

    DeviceRegistry destination = std::move(source);
    RC_CHECK_EQ(destination.count(), 2);
    RC_CHECK_EQ(Device::live(), before + 2);
  }
  RC_CHECK_EQ(Device::live(), before);
}
