#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <vector>

// Lesson 01-13 wrote a constructor, and so did 02-03, which attached cleanup to
// one. Neither said what a constructor is. Lesson 03-01 is about classes and
// never mentions one. This is the missing page, and it lives here because 02-03
// cannot be understood without it.
//
// A constructor is the function that runs when an object is made, before
// anything else can touch it. Its job is to make the object true: every member
// holding a value, every promise the class makes holding from the first moment.
//
// Everything below is measured, on gcc 11 and clang 14.
//
// Setting a member in the list is one step. Setting it in the body is two. Two
// members of a type that counts its own construction, set both ways:
//
//                        built blank   built as a copy   overwritten
//   in the list                  0              2              0
//   in the body                  2              0              2
//
// By the time the body runs every member already exists. A member left out of
// the list is built blank first, and the assignment in the body then overwrites
// it, which is the same result with twice the work. For an int that costs
// nothing. For a std::string or a std::vector it is an allocation made and
// thrown away. For a type with no blank form at all it does not compile.
//
// Members are built in the order they are DECLARED, not the order they are
// listed. A class written with its list in the opposite order built first_
// before second_ regardless. gcc 11 and clang 14 both warn about that list, as
// -Wreorder, and both are right to: the list is lying about what happens, and a
// member whose initialiser uses another member reads it before it exists.
//
// A constructor taking one value is an implicit conversion unless it says
// otherwise. A function taking a type built from a double was called with a bare
// 7 and compiled, and the 7 became a Loose without being asked. Marked
// explicit, the same call is refused by both compilers, and so is
//
//   Meters m = 5.0;
//   std::vector<int> v = 5;
//
// the second because the standard library's own size constructor is explicit.
// checks/ proves the first.
//
// Declaring any constructor takes the default one away. A class with only
// explicit Meters(double) has no Meters(), and "Meters m;" fails with
// "no matching function for call to Meters::Meters()" on gcc and "no matching
// constructor for initialization" on clang. = default asks for it back, and a
// default member initialiser, double step_ = 1.0, gives the members their
// values for every constructor that does not set them.
//
// The rule that falls out: initialise in the list, in declaration order; mark a
// one-value constructor explicit unless the conversion is the whole point; and
// give a class a default constructor only when "has not been given a value" is
// a state that makes sense for it.

// A member that counts how it was made, so that a test can see the difference
// between the two ways of setting it without timing anything.
struct TrackedCounts {
  int defaults = 0;  // built blank
  int copies = 0;    // built as a copy of another
  int assigns = 0;   // overwritten after being built

  // The value of each member as it was copied, in the order the copies
  // happened, which is the order the members were built.
  std::vector<double> copied_values;
};

inline TrackedCounts& tracked_counts() {
  static TrackedCounts counts;
  return counts;
}

struct Tracked {
  double value = 0.0;

  Tracked() { ++tracked_counts().defaults; }
  explicit Tracked(double v) : value(v) {}
  Tracked(const Tracked& other) : value(other.value) {
    ++tracked_counts().copies;
    tracked_counts().copied_values.push_back(other.value);
  }
  Tracked& operator=(const Tracked& other) {
    value = other.value;
    ++tracked_counts().assigns;
    return *this;
  }
};

// Subtracts an offset and applies a scale, which is what turns a raw reading
// into a calibrated one. Both are Tracked, so how the constructor sets them is
// visible.
class Calibration {
 public:
  Calibration(const Tracked& offset, const Tracked& scale) : offset_(offset), scale_(scale) {}

  double apply(double raw) const { return (raw - offset_.value) * scale_.value; }

 private:
  Tracked offset_;
  Tracked scale_;
};

// A distance. It has to be given a value, because a distance nobody measured is
// not zero, it is nothing, so there is no default constructor. And the one
// constructor is explicit, so a speed or an angle cannot become a distance by
// being passed to the wrong parameter.
class Meters {
 public:
  explicit Meters(double value) : value_(value) {}

  double value() const { return value_; }

 private:
  double value_;
};

inline double to_feet(Meters distance) { return distance.value() * 3.28084; }

// A setpoint that moves toward a target one step at a time and never past it.
//
// This one does have a default constructor, because "not going anywhere" is a
// meaningful state: a target of zero, standing at zero. The three members get
// their defaults where they are declared, so every constructor below starts
// from them and only says what it changes.
//
// The step must be positive. A step of zero never arrives, which lesson 01-02
// reported as a result and not a hang.
class Ramp {
 public:
  Ramp() = default;
  explicit Ramp(double target) : target_(target) {}
  Ramp(double target, double step) : target_(target), step_(step) {}

  double current() const { return current_; }
  double target() const { return target_; }
  double step() const { return step_; }

  double advance() {
    if (current_ < target_) {
      current_ += step_;
      if (current_ > target_) current_ = target_;
    } else if (current_ > target_) {
      current_ -= step_;
      if (current_ < target_) current_ = target_;
    }
    return current_;
  }

 private:
  double current_ = 0.0;
  double target_ = 0.0;
  double step_ = 1.0;
};

#endif  // LESSON_SOLUTION_HPP
