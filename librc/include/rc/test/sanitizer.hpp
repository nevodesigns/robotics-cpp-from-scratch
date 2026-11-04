// rc/test/sanitizer.hpp
//
// Which sanitizers this build was compiled with, from lesson 04-07,
// graduated.
//
// A sanitizer is not a debugger and not a static analyser. It is extra code
// the compiler weaves into the program, which checks things as the program
// runs and reports the first one that is wrong.
//
// This header exists because the instrumentation is not a passive observer,
// so a test sometimes has to know whether it is running under one. The
// address sanitizer replaces the allocator: freed blocks go into a quarantine
// rather than being handed straight back, and memory is poisoned on the way
// out. Measured on lesson 02-09's heap test, which writes a pattern into 32
// bytes, frees them, and asks for 32 bytes again:
//
//   without the sanitizer   27 of 32 bytes zero on arrival, 16 of 32 came
//                           back still holding the pattern
//   with the sanitizer       0 of 32 and 0 of 32
//
// Both results are correct and neither is a fact about C++. A test that
// asserted the first pair would fail in the sanitizer lane, and one that said
// nothing about which build it was in would be publishing a number without
// saying what it measured.
//
// Because the detection is constexpr, a branch on it costs nothing at
// runtime: the compiler keeps one side and removes the other. So a suite can
// carry both paths and pay for neither.
//
// What the sanitizers find, measured for the lesson. Of five common defects
// the pair this curriculum's lane builds with finds three: a heap overflow
// and a use after free go to the address sanitizer, signed overflow to the
// undefined behaviour sanitizer, an uninitialised read is found by neither,
// and a data race needs the thread sanitizer, which cannot be combined with
// the address one. The two it misses are the two this curriculum has measured
// most carefully, which is not a coincidence: a defect no tool reports is one
// you design against instead.
//
// And a sanitizer only ever reports what the test actually ran. The lane is
// worth exactly as much as the suite's coverage.

#ifndef RC_TEST_SANITIZER_HPP
#define RC_TEST_SANITIZER_HPP

namespace rc {
namespace test {

// Detecting the address sanitizer.
//
// There is no portable way to ask, and the two compilers disagree about the
// spelling, which is measured rather than guessed:
//
//   gcc 11     defines __SANITIZE_ADDRESS__, and has no __has_feature at all
//   clang 14   has __has_feature(address_sanitizer), and does not define
//              __SANITIZE_ADDRESS__
//
// Verified on both compilers with no sanitizer, with address, with undefined,
// and with both together: eight configurations, correct in all eight.
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    define RC_LESSON_ADDRESS_SANITIZER 1
#  endif
#endif
#if !defined(RC_LESSON_ADDRESS_SANITIZER) && defined(__SANITIZE_ADDRESS__)
#  define RC_LESSON_ADDRESS_SANITIZER 1
#endif

// Was this translation unit compiled with the address sanitizer?
//
// constexpr, because the answer is fixed when the program is built, so it can
// be used in a static_assert or to pick a branch the compiler then removes.
constexpr bool address_sanitizer_on() {
#if defined(RC_LESSON_ADDRESS_SANITIZER)
  return true;
#else
  return false;
#endif
}

// And the one that cannot be answered.
//
// Neither gcc nor clang defines anything at all for the undefined behaviour
// sanitizer. Measured by dumping every predefined macro under
// -fsanitize=undefined on both compilers: nothing matching SANITIZ appears.
//
// So this returns false always, and the honesty is in the name: it is not
// "ubsan is off", it is "nobody will tell us". A test must not conclude that
// the undefined behaviour sanitizer is absent, only that it cannot find out.
constexpr bool undefined_sanitizer_detectable() { return false; }

// What a measurement of the allocator is worth in this build.
//
// This is the reason the header exists rather than the detection being a
// curiosity. The address sanitizer replaces the allocator: freed blocks go
// into a quarantine instead of being handed straight back, and memory is
// poisoned on the way out.
//
// Measured on lesson 02-09's own heap test, which writes a pattern into 32
// bytes, frees them, and asks for 32 bytes again:
//
//   without the sanitizer   27 of 32 bytes were zero on arrival, 16 of 32
//                           came back still holding the pattern
//   with the sanitizer       0 of 32 and 0 of 32
//
// Both results are correct. Neither is a fact about C++. A test that asserted
// the first pair would fail under the sanitizer lane, and a test that said
// nothing about which build it was in would be reporting a number without
// saying what it measured.
constexpr bool allocator_measurements_trustworthy() { return !address_sanitizer_on(); }

// A name for a report, so a suite can print what it was built with.
inline const char* sanitizer_name() {
  return address_sanitizer_on() ? "address (and possibly undefined, which cannot be detected)"
                                : "none that can be detected";
}

}  // namespace test
}  // namespace rc

#endif  // RC_TEST_SANITIZER_HPP
