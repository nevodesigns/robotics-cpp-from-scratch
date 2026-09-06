#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

// Provided by the second translation unit, which included the same header.
int second_unit_registry_size();
const Registry* second_unit_registry_address();
const double* second_unit_value();
const void* second_unit_instance_function();
int second_unit_bump_per_file();
int second_unit_per_file_count();
const void* second_unit_bump_address();

namespace {

// Registered from this file, the same way.
const double kFirstValue = 1.0;
const Registered registration("from the test unit", &kFirstValue);

}  // namespace

RC_TEST("two translation units, one registry") {
  const Registry& here = Registry::instance();

  std::cout << "\n    " << std::left << std::setw(38) << "registry address, this unit"
            << std::right << static_cast<const void*>(&here) << "\n";
  std::cout << "    " << std::left << std::setw(38) << "registry address, other unit"
            << std::right << static_cast<const void*>(second_unit_registry_address())
            << "\n";
  std::cout << "    " << std::left << std::setw(38) << "entries seen here"
            << std::right << here.size() << "\n";
  std::cout << "    " << std::left << std::setw(38) << "entries seen there"
            << std::right << second_unit_registry_size() << "\n";

  // One object. Not two that agree, one.
  RC_CHECK(&here == second_unit_registry_address());
  RC_CHECK_EQ(static_cast<int>(here.size()), second_unit_registry_size());

  // Both files registered something before main ran, and both are in it.
  RC_CHECK(here.size() >= 2);
  RC_CHECK(here.find("from the test unit") == &kFirstValue);
  RC_CHECK(here.find("from the second unit") == second_unit_value());
  RC_CHECK(here.find("nobody registered this") == nullptr);
  RC_CHECK(here.find(nullptr) == nullptr);

  std::cout << "\n    two files added to it before main started, and neither\n";
  std::cout << "    knows the other exists. A function-local static inside an\n";
  std::cout << "    inline function is one object for the whole binary, which\n";
  std::cout << "    is the only header-only way to say that\n";
}

RC_TEST("an inline function is one entity, whoever asks") {
  const void* here = reinterpret_cast<const void*>(&Registry::instance);
  const void* there = second_unit_instance_function();

  std::cout << "\n    " << std::left << std::setw(38) << "instance(), this unit"
            << std::right << here << "\n";
  std::cout << "    " << std::left << std::setw(38) << "instance(), other unit"
            << std::right << there << "\n";

  // The standard requires this: an inline function is one entity in the
  // program, so its address is the same everywhere.
  RC_CHECK(here == there);

  std::cout << "\n    which is what makes the local static inside it one object\n";
  std::cout << "    rather than one per file\n";
}

RC_TEST("a static function in a header is one per file, silently") {
  // Bump twice from here and once from the other file.
  bump_per_file();
  bump_per_file();
  second_unit_bump_per_file();

  std::cout << "\n    " << std::left << std::setw(38) << "bumped from this unit"
            << std::right << 2 << "\n";
  std::cout << "    " << std::left << std::setw(38) << "bumped from the other unit"
            << std::right << 1 << "\n";
  std::cout << "    " << std::left << std::setw(38) << "count this unit can see"
            << std::right << per_file_counter() << "\n";
  std::cout << "    " << std::left << std::setw(38) << "count the other unit can see"
            << std::right << second_unit_per_file_count() << "\n";
  std::cout << "\n    " << std::left << std::setw(38) << "function address, this unit"
            << std::right << address_of_bump_per_file() << "\n";
  std::cout << "    " << std::left << std::setw(38) << "function address, other unit"
            << std::right << second_unit_bump_address() << "\n";

  // Three bumps and no counter says three. Each file has its own function and
  // its own static inside it, and neither is wrong about anything it can see.
  RC_CHECK_EQ(per_file_counter(), 2);
  RC_CHECK_EQ(second_unit_per_file_count(), 1);
  RC_CHECK(per_file_counter() != 3);

  std::cout << "\n    three bumps and no counter says three. Nothing failed to\n";
  std::cout << "    link, nothing warned, and both files are consistent with\n";
  std::cout << "    themselves. The addresses may or may not differ: a linker is\n";
  std::cout << "    allowed to fold two functions whose code is identical, which\n";
  std::cout << "    is why the counts are the thing to look at\n";
}

RC_TEST("the registry refuses what it cannot store") {
  Registry local;
  RC_CHECK_EQ(static_cast<int>(local.size()), 0);

  const double value = 7.0;
  local.add("something", &value);
  RC_CHECK_EQ(static_cast<int>(local.size()), 1);
  RC_CHECK(local.find("something") == &value);

  // Nothing to store is not an entry.
  local.add(nullptr, &value);
  local.add("nothing", nullptr);
  RC_CHECK_EQ(static_cast<int>(local.size()), 1);

  // Names are compared by content, because two translation units that both
  // write the same string are not required to produce the same address for it.
  // A local array is a distinct object from the literal the entry was
  // registered with, so a lookup comparing addresses could not find it.
  //
  // Clang will warn if that difference is asserted by comparing the two
  // pointers, and it is right to: comparing against a string literal is exactly
  // the mistake this test exists to rule out.
  char spelled_out[] = {'s', 'o', 'm', 'e', 't', 'h', 'i', 'n', 'g', '\0'};
  RC_CHECK(local.find(spelled_out) == &value);
}

RC_TEST("what the three answers to one definition actually do") {
  std::cout << "\n    a header defines something, and two files include it\n\n";
  std::cout << "    " << std::left << std::setw(16) << "written as" << std::right
            << std::setw(34) << "what the program gets" << "\n";
  std::cout << "    " << std::left << std::setw(16) << "nothing" << std::right
            << std::setw(34) << "two definitions, link fails" << "\n";
  std::cout << "    " << std::left << std::setw(16) << "static" << std::right
            << std::setw(34) << "one per file, silently" << "\n";
  std::cout << "    " << std::left << std::setw(16) << "inline" << std::right
            << std::setw(34) << "one, shared" << "\n";

  std::cout << "\n    the middle row is the dangerous one, because it links. Two\n";
  std::cout << "    checks in checks/ prove the first row: a function and a\n";
  std::cout << "    variable, each defined twice, neither of which compiles\n";

  // The registry proves the third row at runtime, above. The first row is
  // proved by the build refusing to compile checks/, which is a test in its own
  // right and reported like any other.
  RC_CHECK(Registry::instance().size() >= 2);
}
