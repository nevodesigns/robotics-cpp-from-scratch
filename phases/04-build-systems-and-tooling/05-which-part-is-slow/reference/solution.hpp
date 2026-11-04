#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstring>

#include <rc/core/clock.hpp>
#include <rc/core/index_iterator.hpp>

// A control loop misses its deadline. Phase 07 will measure that it missed and
// by how much. This measures which part of it was slow, which is a different
// question and the one you have to answer before changing anything.
//
// A profiler answers it on a desktop. On a robot you frequently cannot run
// one: the interesting failure happens once an hour, on a machine you reach
// over a network, in a loop that stops behaving the moment anything attaches
// to it. So the loop measures itself.
//
// Three things it is built out of, all of them already written. The RAII guard
// from lesson 02-03, so a section cannot be left open by an early return. The
// injectable clock from 03-05, so the tests here are exact rather than
// statistical. And the iterator from 03-08, so the report can be walked and
// sorted like anything else.

// One named part of the loop, and what it cost.
struct Section {
  const char* name = nullptr;
  int calls = 0;
  rc::core::Nanoseconds total = 0;
  rc::core::Nanoseconds worst = 0;
};

// Where the time in one iteration went.
//
// Fixed capacity, because this runs inside the loop it is measuring and a
// measurement that allocates changes the thing being measured. Sixteen named
// sections is more than any loop should have.
class SectionTimer {
 public:
  using value_type = Section;

  static constexpr std::size_t kMaxSections = 16;

  explicit SectionTimer(const rc::core::Clock& clock) : clock_(&clock) {}

  // Opens a section and closes it when it goes out of scope.
  //
  // The destructor is the whole reason this is a type rather than a pair of
  // calls. A begin and an end can be separated by an early return, a break, or
  // a throw, and then the section stays open and the next one inherits its
  // start time. Lesson 02-03 is where that was established.
  class Scope {
   public:
    Scope(SectionTimer& owner, const char* name)
        : owner_(&owner), index_(owner.index_of(name)), started_(owner.clock_->now()) {}

    ~Scope() { owner_->record(index_, owner_->clock_->now() - started_); }

    // Neither copied nor moved. A section is a place in the code, and two
    // guards for one section would record it twice.
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

   private:
    SectionTimer* owner_;
    std::size_t index_;
    rc::core::Nanoseconds started_;
  };

  // Returned by value, which works despite the deleted copy and move because
  // C++17 guarantees the elision for a prvalue. Lesson 01-11 measured that a
  // returned local travels out rather than being copied; this is the same rule
  // relied on deliberately.
  Scope measure(const char* name) { return Scope(*this, name); }

  // The report, in the order the sections were first seen.
  std::size_t size() const { return count_; }
  const Section& operator[](std::size_t index) const { return sections_[index]; }

  rc::core::IndexIterator<SectionTimer> begin() const {
    return rc::core::IndexIterator<SectionTimer>(this, 0);
  }
  rc::core::IndexIterator<SectionTimer> end() const {
    return rc::core::IndexIterator<SectionTimer>(this, count_);
  }

  // Everything the named sections accounted for.
  //
  // This is not the duration of the loop, and the difference between the two
  // is the point of the whole lesson. See unaccounted_for below.
  rc::core::Nanoseconds accounted_for() const {
    rc::core::Nanoseconds sum = 0;
    for (std::size_t i = 0; i < count_; ++i) sum += sections_[i].total;
    return sum;
  }

  // What the loop took, less what the sections explained.
  //
  // Measure the whole iteration in one outer section and pass its total here,
  // or time it yourself. A positive answer is the code between the sections:
  // the thing nobody thought to name, which is where the surprise usually is.
  //
  // A negative answer means sections were nested, so inner time was counted
  // twice. That is worth knowing rather than hiding, so it is not clamped.
  rc::core::Nanoseconds unaccounted_for(rc::core::Nanoseconds whole_loop) const {
    return whole_loop - accounted_for();
  }

  const Section* find(const char* name) const {
    for (std::size_t i = 0; i < count_; ++i)
      if (std::strcmp(sections_[i].name, name) == 0) return &sections_[i];
    return nullptr;
  }

  // Clears the slots as well as the count.
  //
  // Moving the counter alone looks like it works and does not: the next
  // section registered lands in slot zero, finds the previous run's calls and
  // total still sitting there, and adds to them. The suite for this lesson
  // caught exactly that, which is the same stale state failure as a section
  // left open inheriting a start time.
  void reset() {
    for (std::size_t i = 0; i < count_; ++i) sections_[i] = Section{};
    count_ = 0;
  }

 private:
  // Sections are found by name, and the comparison is a string compare rather
  // than a pointer compare, so the same literal written in two translation
  // units is still one section.
  std::size_t index_of(const char* name) {
    for (std::size_t i = 0; i < count_; ++i)
      if (std::strcmp(sections_[i].name, name) == 0) return i;

    // Out of room: everything after the sixteenth lands in the last slot
    // rather than being dropped, so the total still adds up and the report
    // shows a section with an implausible number of calls.
    if (count_ == kMaxSections) return kMaxSections - 1;

    sections_[count_].name = name;
    return count_++;
  }

  void record(std::size_t index, rc::core::Nanoseconds elapsed) {
    Section& section = sections_[index];
    ++section.calls;
    section.total += elapsed;
    if (elapsed > section.worst) section.worst = elapsed;
  }

  const rc::core::Clock* clock_;
  Section sections_[kMaxSections];
  std::size_t count_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
