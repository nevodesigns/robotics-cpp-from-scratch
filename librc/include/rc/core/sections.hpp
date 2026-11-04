// rc/core/sections.hpp
//
// Attributing time to named parts of one loop iteration, from lesson 04-05,
// graduated.
//
// Phase 07 measures that a control loop missed its deadline and by how much.
// This measures which part of it was slow, which is a different question and
// the one to answer before changing anything.
//
// A profiler answers it on a desktop. On a robot you frequently cannot run
// one: the interesting failure happens once an hour, on a machine you reach
// over a network, in a loop that stops behaving the moment anything attaches
// to it. So the loop measures itself.
//
// Built out of three things already written. The RAII guard from 02-03, so a
// section cannot be left open by an early return. The injectable clock from
// 03-05, so the tests are exact rather than statistical. The iterator from
// 03-08, so the report sorts like any other container.
//
// Two numbers from the lesson worth carrying with the code.
//
// On a loop broken into four named parts, the parts came to 880 of 1000
// microseconds. The missing 120, twelve percent, went somewhere nobody had
// thought to name, and that is the usual place for a surprise. unaccounted_for
// exists to print it.
//
// Nesting double counts. An outer section containing an inner one measured 700
// microseconds while the inner measured 500, so the sections accounted for
// 1200 microseconds of a 700 microsecond loop. unaccounted_for then reports
// minus 500 and is deliberately not clamped, because a negative answer is the
// only signal that the shares add up to more than a hundred percent.
//
// The instrument costs exactly two clock reads per section, measured with a
// counting clock. A 500 Hz loop with ten sections therefore asks the clock
// 10000 times a second, and whether that matters is a question about your
// clock rather than about this code.

#ifndef RC_CORE_SECTIONS_HPP
#define RC_CORE_SECTIONS_HPP

#include <cstddef>
#include <cstring>

#include <rc/core/clock.hpp>
#include <rc/core/index_iterator.hpp>

namespace rc {
namespace core {

// One named part of the loop, and what it cost.
struct Section {
  const char* name = nullptr;
  int calls = 0;
  Nanoseconds total = 0;
  Nanoseconds worst = 0;
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

  explicit SectionTimer(const Clock& clock) : clock_(&clock) {}

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
    Nanoseconds started_;
  };

  // Returned by value, which works despite the deleted copy and move because
  // C++17 guarantees the elision for a prvalue. Lesson 01-11 measured that a
  // returned local travels out rather than being copied; this is the same rule
  // relied on deliberately.
  Scope measure(const char* name) { return Scope(*this, name); }

  // The report, in the order the sections were first seen.
  std::size_t size() const { return count_; }
  const Section& operator[](std::size_t index) const { return sections_[index]; }

  IndexIterator<SectionTimer> begin() const {
    return IndexIterator<SectionTimer>(this, 0);
  }
  IndexIterator<SectionTimer> end() const {
    return IndexIterator<SectionTimer>(this, count_);
  }

  // Everything the named sections accounted for.
  //
  // This is not the duration of the loop, and the difference between the two
  // is the point of the whole lesson. See unaccounted_for below.
  Nanoseconds accounted_for() const {
    Nanoseconds sum = 0;
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
  Nanoseconds unaccounted_for(Nanoseconds whole_loop) const {
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

  void record(std::size_t index, Nanoseconds elapsed) {
    Section& section = sections_[index];
    ++section.calls;
    section.total += elapsed;
    if (elapsed > section.worst) section.worst = elapsed;
  }

  const Clock* clock_;
  Section sections_[kMaxSections];
  std::size_t count_ = 0;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_SECTIONS_HPP
