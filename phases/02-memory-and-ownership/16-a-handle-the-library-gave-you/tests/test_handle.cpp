#include <rc/test/rc_test.hpp>

#include <iostream>
#include <stdexcept>
#include <utility>

#include "solution.hpp"

namespace {

// The devices the raw version forgets. A real program has lost these for good; the test keeps the
// pointers only so that it can close them afterwards and leave nothing behind.
Device* g_orphans[256];
int g_orphan_count = 0;

// What a person writes first: open, open, read, close both. The early return leaks.
bool read_pair_raw(int first_id, int second_id, double* sum) {
  Device* first = device_open(first_id);
  if (first == nullptr) return false;
  Device* second = device_open(second_id);
  if (second == nullptr) {
    g_orphans[g_orphan_count++] = first;   // first is never closed by this function
    return false;
  }
  *sum = device_read(first) + device_read(second);
  device_close(first);
  device_close(second);
  return true;
}

}  // namespace

RC_TEST("two devices opened, read and closed: every open has its close") {
  reset_device_log();
  double sum = 0.0;
  RC_CHECK(read_pair(4, 6, &sum));
  RC_CHECK_NEAR(sum, 5.0, 1e-12);       // 4 * 0.5 + 6 * 0.5
  RC_CHECK_EQ(device_log().opened, 2);
  RC_CHECK_EQ(device_log().closed, 2);
  RC_CHECK_EQ(device_log().live, 0);
  RC_CHECK_EQ(device_log().bad_closes, 0);
}

RC_TEST("a failure half way leaks one device each time with raw handles and none with the owner") {
  const int attempts = 100;
  double sum = 0.0;

  reset_device_log();
  for (int i = 0; i < attempts; ++i) read_pair_raw(3, -1, &sum);
  const int raw_leaked = device_log().live;

  // Close what the raw version left, which a real program could not, and measure the owner.
  for (int i = 0; i < g_orphan_count; ++i) device_close(g_orphans[i]);
  g_orphan_count = 0;
  reset_device_log();
  for (int i = 0; i < attempts; ++i) RC_CHECK(!read_pair(3, -1, &sum));
  const int owned_leaked = device_log().live;

  std::cout << "  " << attempts << " attempts where the second device cannot be opened: raw handles left " << raw_leaked
            << " open, the owner left " << owned_leaked << "\n\n";
  RC_CHECK_EQ(raw_leaked, attempts);
  RC_CHECK_EQ(owned_leaked, 0);
  RC_CHECK_EQ(device_log().opened, attempts);
  RC_CHECK_EQ(device_log().closed, attempts);
}

RC_TEST("a handle is closed when an exception leaves the scope, and a failed open is never closed") {
  reset_device_log();
  try {
    DevicePtr d = open_checked(2);
    RC_REQUIRE(d != nullptr);
    throw std::runtime_error("something went wrong while holding the device");
  } catch (const std::runtime_error&) {
  }
  std::cout << "  an exception thrown while holding a device: opened " << device_log().opened << ", closed "
            << device_log().closed << "\n";
  RC_CHECK_EQ(device_log().opened, 1);
  RC_CHECK_EQ(device_log().closed, 1);

  reset_device_log();
  {
    DevicePtr nothing = open_checked(-5);
    RC_CHECK(nothing == nullptr);
  }
  std::cout << "  a device that failed to open went out of scope: bad closes " << device_log().bad_closes << "\n\n";
  RC_CHECK_EQ(device_log().bad_closes, 0);
  RC_CHECK_EQ(device_log().closed, 0);
}

RC_TEST("moving a handle moves the duty to close it, and it is closed exactly once") {
  reset_device_log();
  {
    DevicePtr a = open_checked(8);
    RC_REQUIRE(a != nullptr);
    DevicePtr b = std::move(a);
    RC_CHECK(a == nullptr);
    RC_CHECK(b != nullptr);
    RC_CHECK_EQ(device_log().closed, 0);

    DevicePtr c = open_checked(9);
    c = std::move(b);                  // c's own device is closed by the assignment, then it holds b's
    RC_CHECK_EQ(device_log().closed, 1);
  }
  std::cout << "  two devices opened, one handed over and one replaced: opened " << device_log().opened << ", closed "
            << device_log().closed << ", bad closes " << device_log().bad_closes << "\n\n";
  RC_CHECK_EQ(device_log().opened, 2);
  RC_CHECK_EQ(device_log().closed, 2);
  RC_CHECK_EQ(device_log().bad_closes, 0);
}

RC_TEST("the owner costs no more than the raw pointer it replaces") {
  std::cout << "  sizeof(Device*) " << sizeof(Device*) << ", sizeof(DevicePtr) " << sizeof(DevicePtr) << "\n\n";
  RC_CHECK_EQ(sizeof(DevicePtr), sizeof(Device*));

  // And it still does the job.
  reset_device_log();
  {
    DevicePtr d = open_checked(1);
    RC_CHECK(d != nullptr);
  }
  RC_CHECK_EQ(device_log().closed, 1);
}
