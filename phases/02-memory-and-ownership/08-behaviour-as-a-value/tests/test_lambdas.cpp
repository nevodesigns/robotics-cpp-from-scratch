#include <rc/test/rc_test.hpp>

#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <vector>

#include <rc/test/leak_check.hpp>

#include "solution.hpp"

namespace {

// Something of a known size to capture, so the std::function sweep has a knob
// to turn. Test code may use templates freely; the learner meets them in 03-07.
template <std::size_t Bytes>
struct Payload {
  char filler[Bytes] = {};
};

// Counts allocations caused by storing a lambda capturing Bytes of state.
template <std::size_t Bytes>
std::size_t allocations_to_store() {
  const Payload<Bytes> payload;
  rc::test::AllocationCount counted;
  std::function<std::size_t()> stored = [payload]() { return sizeof(payload); };
  const std::size_t used = counted.total();
  (void)stored();
  return used;
}

}  // namespace

RC_TEST("by value takes a copy now, by reference reads it later") {
  double limit = 1.0;

  const auto copy = watching_a_copy(limit);
  const auto original = watching_the_original(limit);

  // Both were created while the limit was 1.0. Now it is not.
  limit = 100.0;

  std::cout << "\n  both callables were made when the limit was 1, and it is now "
            << limit << "\n\n";
  std::cout << "    watching_a_copy says 50 exceeds it:      "
            << (copy(50.0) ? "true" : "false") << "\n";
  std::cout << "    watching_the_original says 50 exceeds it: "
            << (original(50.0) ? "true" : "false") << "\n\n";

  // The copy still answers against 1.0, which it owns.
  RC_CHECK(copy(50.0));
  RC_CHECK(copy(0.5) == false);

  // The reference answers against 100.0, which it only borrows.
  RC_CHECK(original(50.0) == false);
  RC_CHECK(original(150.0));

  // Turn the limit again and only one of them notices.
  limit = 10.0;
  RC_CHECK(copy(50.0));
  RC_CHECK(original(50.0));
  RC_CHECK(original(5.0) == false);

  std::cout << "  a by value capture is a decision taken once. A by reference\n";
  std::cout << "  capture is a question asked every call.\n\n";
}

RC_TEST("capturing a container by value copies its buffer") {
  const std::vector<double> samples(1024, 1.0);
  const Address original = address_of(samples.data());

  const auto copied = address_of_a_copied_buffer(samples);
  const auto borrowed = address_of_a_borrowed_buffer(samples);

  std::cout << "  a buffer of 1024 doubles, captured two ways\n\n";
  std::cout << "    by value     the callable reads "
            << (copied() == original ? "the same buffer" : "a copy of it") << "\n";
  std::cout << "    by reference the callable reads "
            << (borrowed() == original ? "the same buffer" : "a copy of it") << "\n\n";

  // The capture list is a list of initialisations, so the rules from lesson
  // 01-11 apply unchanged: by value copies, by reference does not.
  RC_CHECK(copied() != original);
  RC_CHECK(borrowed() == original);

  // And the caller's vector is untouched either way.
  RC_CHECK(samples.size() == 1024u);
  RC_CHECK(address_of(samples.data()) == original);
}

RC_TEST("a lambda is exactly as big as what it captured") {
  double limit = 1.0;
  const std::vector<double> samples(8, 1.0);

  const auto nothing = [](double value) { return value > 0.0; };
  const auto one_double = [limit](double value) { return value > limit; };
  const auto one_reference = [&limit](double value) { return value > limit; };
  const auto whole_vector = [samples]() { return samples.size(); };

  std::cout << "  " << std::left << std::setw(26) << "what it captures" << "sizeof\n"
            << std::right;
  const auto row = [](const char* name, std::size_t bytes) {
    std::cout << "  " << std::left << std::setw(26) << name << bytes << "\n" << std::right;
  };
  row("nothing", sizeof(nothing));
  row("one double", sizeof(one_double));
  row("one reference", sizeof(one_reference));
  row("a whole vector", sizeof(whole_vector));
  std::cout << "\n  sizeof(std::vector<double>) is " << sizeof(std::vector<double>)
            << ", and sizeof(double) is " << sizeof(double) << "\n\n";

  // A lambda is a class the compiler writes, with one member per capture. One
  // that captures nothing has no members, and an empty class still has to have
  // a distinct address, so it is one byte rather than none.
  RC_CHECK(sizeof(nothing) == 1u);
  RC_CHECK(sizeof(one_double) == sizeof(double));
  RC_CHECK(sizeof(whole_vector) == sizeof(std::vector<double>));

  // A reference capture stores somewhere to look, not the thing.
  RC_CHECK(sizeof(one_reference) < sizeof(whole_vector));
  RC_CHECK(sizeof(one_reference) <= sizeof(void*));

  (void)nothing(1.0);
  (void)one_double(1.0);
  (void)one_reference(1.0);
  (void)whole_vector();

  // And a std::function is one size whatever it was given, which is the point
  // of it: the type of the lambda is erased. Both of this lesson's callables
  // are the same size as each other and as the empty one, and only one of them
  // captured a whole vector.
  const auto a_copy = watching_a_copy(1.0);
  const auto a_buffer = address_of_a_copied_buffer(samples);
  RC_CHECK(sizeof(a_copy) == sizeof(std::function<bool(double)>));
  RC_CHECK(sizeof(a_buffer) == sizeof(std::function<Address()>));
  RC_CHECK(a_copy(2.0));
}

RC_TEST("every lambda has its own type, even two that are written the same") {
  const auto first = [](double value) { return value > 0.0; };
  const auto second = [](double value) { return value > 0.0; };

  // Computed into a variable first, because the comma inside the template
  // argument list would otherwise look like a second macro argument to the
  // preprocessor, which knows nothing about angle brackets.
  const bool different_lambdas_same_type =
      std::is_same<decltype(first), decltype(second)>::value;
  const bool one_lambda_same_as_itself =
      std::is_same<decltype(first), decltype(first)>::value;

  // Character for character identical, and two different types. The compiler
  // writes a new class for every lambda expression in the program.
  RC_CHECK(!different_lambdas_same_type);
  RC_CHECK(one_lambda_same_as_itself);

  std::cout << "  two character for character identical lambdas are different types\n";
  std::cout << "  which is why you store one with auto, and why putting one in a\n";
  std::cout << "  std::function is a conversion rather than a copy\n\n";

  // Which is also why this is the one case where auto is not a preference. You
  // cannot write the type down: it has no name. A function that needs to
  // return one therefore returns a std::function, as this lesson's do.
  RC_CHECK(first(1.0));
  RC_CHECK(second(1.0));
  RC_CHECK(watching_a_copy(1.0)(2.0));
}

RC_TEST("a std::function allocates when what it holds will not fit inside it") {
  std::cout << "  sizeof(std::function<std::size_t()>) is "
            << sizeof(std::function<std::size_t()>) << "\n\n";
  std::cout << "  " << std::left << std::setw(20) << "bytes captured"
            << "allocations to store it\n" << std::right;

  const std::size_t small = allocations_to_store<8>();
  const std::size_t edge = allocations_to_store<16>();
  const std::size_t over = allocations_to_store<17>();
  const std::size_t large = allocations_to_store<64>();

  const auto row = [](std::size_t bytes, std::size_t count) {
    std::cout << "  " << std::left << std::setw(20) << bytes << count << "\n" << std::right;
  };
  row(8, small);
  row(16, edge);
  row(17, over);
  row(64, large);

  // The same big lambda called directly, with no std::function anywhere.
  rc::test::AllocationCount counted;
  const Payload<64> payload;
  const auto direct = [payload]() { return sizeof(payload); };
  (void)direct();
  const std::size_t calling_it_directly = counted.total();

  std::cout << "\n  " << std::left << std::setw(20) << "64, called directly"
            << calling_it_directly << "\n" << std::right;

  // This lesson's own two callables land on opposite sides of that line, which
  // is the whole point made concrete. One captures a double, the other
  // captures a whole std::vector.
  const std::vector<double> samples(1024, 1.0);

  rc::test::AllocationCount for_a_double;
  const auto limit_test = watching_a_copy(1.0);
  const std::size_t storing_a_double = for_a_double.total();

  rc::test::AllocationCount for_a_vector;
  const auto buffer_reader = address_of_a_copied_buffer(samples);
  const std::size_t storing_a_vector = for_a_vector.total();

  std::cout << "\n  " << std::left << std::setw(34) << "watching_a_copy, one double"
            << storing_a_double << " allocations\n";
  std::cout << "  " << std::left << std::setw(34)
            << "a copied buffer, a whole vector" << storing_a_vector
            << " allocations\n" << std::right;

  RC_CHECK(limit_test(2.0));
  RC_CHECK(buffer_reader() != 0u);

  // Capturing one double is free to store. Capturing a container is not, and
  // it also copied the buffer, which is two costs for one capture list.
  RC_CHECK(storing_a_double == 0u);
  RC_CHECK(storing_a_vector > 0u);
  std::cout << "\n  the size it will hold for free is your standard library's choice,\n";
  std::cout << "  not a rule, so the sweep is printed rather than the number asserted\n\n";

  // Small captures are free, large ones are not, and the lambda itself never
  // allocates: it is std::function that needs somewhere to put it.
  RC_CHECK(small == 0u);
  RC_CHECK(large > 0u);
  RC_CHECK(calling_it_directly == 0u);

  // Which means a callback in a control loop is a question about what it
  // captured, and the answer is measurable rather than a matter of opinion.
  RC_CHECK(over >= small);
}

RC_TEST("a captureless lambda is also a plain function pointer") {
  // No state, so a function pointer is enough to describe it. This is the one
  // that a C API will accept.
  bool (*plain)(double) = [](double value) { return value > 1.0; };
  RC_CHECK(plain(2.0));
  RC_CHECK(!plain(0.5));

  // One that captures cannot be, because the pointer has nowhere to put the
  // captured values. That is in checks/ as code that must not compile.
  const double limit = 1.0;
  const auto capturing = [limit](double value) { return value > limit; };
  RC_CHECK(capturing(2.0));

  // It carries the limit with it, so it is the size of a double rather than
  // the one byte an empty one takes.
  RC_CHECK(sizeof(capturing) == sizeof(double));

  // Which is why this lesson's functions return a std::function and not a
  // function pointer: every one of them captures something.
  RC_CHECK(watching_a_copy(1.0)(2.0));
  RC_CHECK(!watching_a_copy(10.0)(5.0));

  std::cout << "  a captureless lambda assigned to bool(*)(double) and called: works\n";
  std::cout << "  the capturing one is refused, and checks/ holds the error\n\n";
}
