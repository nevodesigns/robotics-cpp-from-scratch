// A second translation unit, which is the whole point of this lesson.
//
// It includes the same header as the test file does, so everything the header
// defines exists here as well. What the linker then does with the two copies is
// what the tests measure.
#include "solution.hpp"

namespace {

// Registered from this file, by a constructor nobody calls.
const double kSecondValue = 2.0;
const Registered registration("from the second unit", &kSecondValue);

// A function with internal linkage, defined by the header, called from here.
// This is a different entity from the one the test file calls, and it has its
// own local static.
}  // namespace

int second_unit_registry_size() {
  return static_cast<int>(Registry::instance().size());
}

const Registry* second_unit_registry_address() { return &Registry::instance(); }

const double* second_unit_value() { return &kSecondValue; }

// The address of an inline function, taken here. The standard says this is one
// entity across the whole program, so it is the same address the test file
// sees.
const void* second_unit_instance_function() {
  return reinterpret_cast<const void*>(&Registry::instance);
}

// The static function from the header, called and inspected from this file.
int second_unit_bump_per_file() { return bump_per_file(); }
int second_unit_per_file_count() { return per_file_counter(); }
const void* second_unit_bump_address() { return address_of_bump_per_file(); }
