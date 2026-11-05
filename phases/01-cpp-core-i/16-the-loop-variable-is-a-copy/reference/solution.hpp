#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <vector>

// Lesson 01-02 taught the loop that counts. Lesson 01-04 then wrote
//
//   for (const double value : samples) sum += value;
//
// and nobody said what it was. This is the range based for, the loop that
// visits each thing in turn without an index, and it is in twenty
// lessons from phase 00 onwards.
//
// It has one declaration in the middle of it, the loop variable, and that is
// where everything goes wrong. The variable is declared by you, and what you
// write decides whether it IS the element or a copy of it.
//
// Measured on gcc 11 and clang 14.
//
// A loop variable declared by value is a copy, so changing it changes nothing.
// Five readings summing to 15, doubled:
//
//   for (double v : values)  v *= 2     sum is now 15
//   for (double& v : values) v *= 2     sum is now 30
//
// The loop runs, the variable is doubled, and the doubled copy is thrown away
// at the end of each pass. Whether anything says so depends on the compiler and
// on what else the loop does. As written above gcc 11 reported nothing and
// clang 14 reported -Wunused-but-set-variable, because the copy was set and
// never read. Add one line that reads it afterwards, x *= 2; sum += x, and
// both were silent. A warning that appears only when the bug is the whole body
// is not protection.
//
// It also copies every element. A thousand readings, each loop run once:
//
//   for (Reading r : readings)           1000 copies
//   for (const Reading& r : readings)       0
//   for (auto r : readings)              1000
//   for (const auto& r : readings)          0
//
// auto does not change this. It asks the compiler to write the type, and the
// type it writes has no reference in it unless you add one.
//
// The loop variable is not the element, so its address is no use. Asked from
// inside the loop, the address of a by value variable was different from the
// address of the element, and the address of a reference variable was the
// same. A function that returns a pointer to what it found has to be written
// with the reference, or it hands back the address of a copy that has already
// gone.
//
// A temporary in the range expression is gone before the body runs. With
// Holder(4).readings_by_value() the Holder had already been destroyed by the
// time the body first ran, and the vector it returned was kept alive. If the
// function had returned a reference into the Holder instead, the body would
// have read a dead object. Run once outside the suite, that summed to
// 2882482050 with no error, and the address sanitizer reported
// stack-use-after-scope. Nothing in the language says that the loop is the
// culprit, and C++23 changed this rule, so it is worth knowing which standard
// you are on.
//
// A vector of bool is not a vector of bools. It packs eight flags to a byte, so
// there is no bool to refer to, and indexing hands back a small proxy object.
// Measured here, sizeof(bool) is 1 and sizeof of what flags[0] returns is 16: a
// pointer to the byte and a mask for the bit. The size is the library's choice,
// so the suite asserts only that it is not a bool's. A loop variable
// that is a plain reference wants something that exists and is refused by both
// compilers, which checks/ proves. A loop variable that copies the proxy works,
// and writing to the flags is an index loop.
//
// The rule that falls out: read with const auto&, change with auto&, and copy
// only a thing that is as cheap as a number.

// A reading that counts how many times it is copied, so that a test can
// measure a loop without timing it. The count is the evidence. Time is a
// property of the machine that day.
inline int& copies_made() {
  static int count = 0;
  return count;
}

struct Reading {
  double value = 0.0;

  Reading() = default;
  explicit Reading(double v) : value(v) {}
  Reading(const Reading& other) : value(other.value) { ++copies_made(); }
  Reading& operator=(const Reading& other) {
    value = other.value;
    ++copies_made();
    return *this;
  }
};

// Multiplies every reading by factor, in place.
inline void scale_all(std::vector<Reading>& readings, double factor) {
  for (Reading& reading : readings) reading.value *= factor;
}

// The sum of every reading, copying none of them.
inline double total(const std::vector<Reading>& readings) {
  double sum = 0.0;
  for (const Reading& reading : readings) sum += reading.value;
  return sum;
}

// How many readings are strictly above limit.
inline int count_above(const std::vector<Reading>& readings, double limit) {
  int count = 0;
  for (const Reading& reading : readings)
    if (reading.value > limit) ++count;
  return count;
}

// The first reading strictly above limit, or nullptr. The pointer is to the
// element in the vector and not to a copy of it, which is why the loop
// variable is a reference.
inline const Reading* first_above(const std::vector<Reading>& readings, double limit) {
  for (const Reading& reading : readings)
    if (reading.value > limit) return &reading;
  return nullptr;
}

// How many flags are set.
//
// The loop variable is copied, which is right here: it is the stand-in for a
// bit, as cheap as a bool.
inline int count_true(const std::vector<bool>& flags) {
  int count = 0;
  for (const bool flag : flags)
    if (flag) ++count;
  return count;
}

// Sets every flag to value.
//
// An index loop. A reference to a flag cannot be written, because there is no
// bool for it to refer to.
inline void mark_all(std::vector<bool>& flags, bool value) {
  for (std::size_t i = 0; i < flags.size(); ++i) flags[i] = value;
}

#endif  // LESSON_SOLUTION_HPP
