#include <rc/test/rc_test.hpp>

#include <stdexcept>
#include <type_traits>

#include "solution.hpp"

namespace {

// Every test starts from a clean device table, so one failing test cannot make
// the next one look broken.
struct CleanSlate {
  CleanSlate() { fake_open_ids().clear(); }
};

void throws_while_holding_a_device() {
  const DeviceHandle handle;
  throw std::runtime_error("something went wrong mid operation");
}

}  // namespace

RC_TEST("a handle opens a device when it is created") {
  const CleanSlate clean;
  const DeviceHandle handle;
  RC_CHECK(handle.is_open());
  RC_CHECK_EQ(fake_open_count(), 1);
}

RC_TEST("the device is closed when the handle goes out of scope") {
  const CleanSlate clean;
  {
    const DeviceHandle handle;
    RC_CHECK_EQ(fake_open_count(), 1);
  }
  RC_CHECK_EQ(fake_open_count(), 0);
}

RC_TEST("the device is closed even when an exception is thrown") {
  // This is the case a manually placed close at the bottom of a function cannot
  // handle at all, and it is why the cleanup belongs to the object.
  const CleanSlate clean;
  try {
    throws_while_holding_a_device();
  } catch (const std::runtime_error&) {
    // swallowed on purpose
  }
  RC_CHECK_EQ(fake_open_count(), 0);
}

RC_TEST("handles cannot be copied") {
  // If this ever compiles, two handles hold one id and the device is closed
  // twice. The check is done at compile time because that is where the mistake
  // is, rather than at run time where the damage is.
  RC_CHECK(!std::is_copy_constructible<DeviceHandle>::value);
  RC_CHECK(!std::is_copy_assignable<DeviceHandle>::value);
}

RC_TEST("closing early releases the device") {
  const CleanSlate clean;
  DeviceHandle handle;
  RC_CHECK_EQ(fake_open_count(), 1);
  handle.close();
  RC_CHECK_EQ(fake_open_count(), 0);
  RC_CHECK(!handle.is_open());
}

RC_TEST("closing twice is safe") {
  const CleanSlate clean;
  DeviceHandle handle;
  handle.close();
  handle.close();
  RC_CHECK_EQ(fake_open_count(), 0);
}

RC_TEST("closing early then destructing does not close twice") {
  const CleanSlate clean;
  const int other = fake_open();   // a device belonging to somebody else
  {
    DeviceHandle handle;
    handle.close();
  }
  // If the destructor had closed a stale id, it could have taken this one out.
  RC_CHECK(fake_is_open(other));
  RC_CHECK_EQ(fake_open_count(), 1);
  fake_close(other);
}

RC_TEST("a closed handle reports no device") {
  const CleanSlate clean;
  DeviceHandle handle;
  handle.close();
  RC_CHECK_EQ(handle.id(), kNoDevice);
}

RC_TEST("many handles hold distinct devices and release all of them") {
  const CleanSlate clean;
  {
    const DeviceHandle a;
    const DeviceHandle b;
    const DeviceHandle c;
    RC_CHECK_EQ(fake_open_count(), 3);
    RC_CHECK(a.id() != b.id());
    RC_CHECK(b.id() != c.id());
  }
  RC_CHECK_EQ(fake_open_count(), 0);
}

RC_TEST("handles are destroyed in reverse order of creation") {
  const CleanSlate clean;
  int first_id = kNoDevice;
  {
    const DeviceHandle first;
    first_id = first.id();
    {
      const DeviceHandle second;
      RC_CHECK(second.id() > first_id);
    }
    // The inner one is gone, the outer one is still held.
    RC_CHECK(fake_is_open(first_id));
  }
  RC_CHECK(!fake_is_open(first_id));
}
