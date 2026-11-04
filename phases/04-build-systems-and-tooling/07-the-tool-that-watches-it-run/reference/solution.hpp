#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// A sanitizer is not a debugger and not a static analyser. It is extra code
// the compiler weaves into your program, which checks things as the program
// runs and reports the first one that is wrong.
//
// You have been relying on them since lesson 02-02 without being told what
// they are. Ten lessons in this curriculum mention one, continuous
// integration runs a job built with two of them on every push, and until this
// lesson the only section about them anywhere was called "the sanitizer lane
// does not cover this".
//
// This is that lesson, and the headline is the part nobody says out loud:
// the two sanitizers overlap, neither is a superset of the other, and there
// are whole classes of defect that neither one sees.

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


// Which tool finds which defect, which is the table this lesson exists to
// build. Every row below was measured rather than looked up.
enum class Defect {
  heap_overflow,       // reading or writing past the end of a new[] block
  use_after_free,      // touching memory after it was handed back
  signed_overflow,     // arithmetic that will not fit, as in lesson 00-04
  uninitialised_read,  // reading a value nobody wrote, as in lesson 02-09
  data_race,           // two threads, one value, no synchronisation
};

// The name of the sanitizer that reports it, or what to do instead.
//
// The measurements, all taken for this lesson on gcc 11 at -O1 unless the
// row says otherwise:
//
//   heap_overflow       plain build printed "read 0" and carried on.
//                       address reported heap-buffer-overflow, with the block
//                       size known at compile time and again with it known
//                       only at runtime. undefined reported it too in the
//                       first case, through its object-size check, and
//                       reported nothing at all in the second. So address is
//                       the answer: undefined finds some of these and cannot
//                       be relied on for them.
//
//   use_after_free      address. The same shadow memory and quarantine that
//                       makes the allocator measurement in this lesson's
//                       second test come back empty is what detects this.
//
//   signed_overflow     plain build printed -1293967296. undefined reported
//                       "signed integer overflow: 3001 * 1000000 cannot be
//                       represented". address printed -1293967296 and said
//                       nothing: it is blind to this entirely.
//
//   uninitialised_read  neither. Lesson 02-09 measured this at length: no
//                       warning on either compiler for the case that reaches
//                       a robot, address runs it clean and prints the
//                       garbage, and the memory sanitizer, which is the tool
//                       for it, is clang only, cannot be combined with
//                       address, needs an instrumented standard library, and
//                       missed that case here.
//
//   data_race           thread, which is a third sanitizer and cannot be
//                       combined with address either. Phase 07 is where that
//                       is met properly.
inline const char* finds(Defect defect) {
  switch (defect) {
    case Defect::heap_overflow: return "address";
    case Defect::use_after_free: return "address";
    case Defect::signed_overflow: return "undefined";
    case Defect::uninitialised_read: return "neither";
    case Defect::data_race: return "thread";
  }
  return "neither";
}

// Whether the pair this curriculum's sanitizer lane builds with, address and
// undefined together, would find it.
inline bool found_by_the_lane(Defect defect) {
  const char* tool = finds(defect);
  return tool[0] == 'a' || tool[0] == 'u';
}

#endif  // LESSON_SOLUTION_HPP
