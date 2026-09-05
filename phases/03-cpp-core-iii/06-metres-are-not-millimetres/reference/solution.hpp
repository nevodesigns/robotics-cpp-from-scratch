#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// A number that knows what it is.
//
// Every value in a robot is a quantity of something, and nothing in a double
// records which something. Metres and millimetres are both doubles. So are
// radians and degrees, seconds and milliseconds, and a velocity and a position.
// Adding two of them compiles perfectly whatever they mean.
//
// The Tag parameter has no members and is never instantiated. It exists so that
// Quantity<Metres> and Quantity<Seconds> are different types, which is the whole
// mechanism: the arithmetic below is defined between two of the same tag and
// nowhere else, so the compiler refuses the rest.
//
// It costs nothing at runtime. One double in, one double out, and every
// operation below is a function that disappears.
template <class Tag>
class Quantity {
 public:
  constexpr Quantity() = default;

  // Explicit, so a bare double cannot become a quantity by accident. That is
  // the whole point: the conversion has to be written down, at the edge, where
  // somebody decided what the number meant.
  explicit constexpr Quantity(double value) : value_(value) {}

  constexpr double value() const { return value_; }

  constexpr Quantity operator+(Quantity other) const {
    return Quantity(value_ + other.value_);
  }
  constexpr Quantity operator-(Quantity other) const {
    return Quantity(value_ - other.value_);
  }
  constexpr Quantity operator-() const { return Quantity(-value_); }

  // Scaling by a plain number is allowed, because a number has no unit and
  // twice a distance is a distance.
  constexpr Quantity operator*(double scale) const { return Quantity(value_ * scale); }
  constexpr Quantity operator/(double scale) const { return Quantity(value_ / scale); }

  // Dividing two of the same thing gives a plain number, which is what a ratio
  // is, and it is how a fraction of a distance is written.
  constexpr double operator/(Quantity other) const { return value_ / other.value_; }

  constexpr bool operator<(Quantity other) const { return value_ < other.value_; }
  constexpr bool operator>(Quantity other) const { return value_ > other.value_; }
  constexpr bool operator==(Quantity other) const { return value_ == other.value_; }
  constexpr bool operator!=(Quantity other) const { return value_ != other.value_; }

  Quantity& operator+=(Quantity other) {
    value_ += other.value_;
    return *this;
  }

 private:
  double value_ = 0.0;
};

template <class Tag>
constexpr Quantity<Tag> operator*(double scale, Quantity<Tag> quantity) {
  return quantity * scale;
}

template <class Tag>
Quantity<Tag> abs(Quantity<Tag> quantity) {
  return Quantity<Tag>(std::fabs(quantity.value()));
}

// The tags. Empty on purpose: they are names, not types with behaviour.
struct MetreTag {};
struct SecondTag {};
struct RadianTag {};

using Metres = Quantity<MetreTag>;
using Seconds = Quantity<SecondTag>;
using Radians = Quantity<RadianTag>;

// The conversions, written once, where somebody decided what the number meant.
//
// This is where a unit system earns its keep. Not in the arithmetic, which was
// probably right anyway, but in there being exactly one line in the program that
// knows a millimetre is a thousandth of a metre, instead of a factor of a
// thousand appearing wherever somebody remembered it.
constexpr Metres metres(double value) { return Metres(value); }
constexpr Metres millimetres(double value) { return Metres(value / 1000.0); }
constexpr Seconds seconds(double value) { return Seconds(value); }
constexpr Seconds milliseconds(double value) { return Seconds(value / 1000.0); }
constexpr Radians radians(double value) { return Radians(value); }

inline Radians degrees(double value) {
  return Radians(value * 3.14159265358979323846 / 180.0);
}

constexpr double as_millimetres(Metres length) { return length.value() * 1000.0; }
constexpr double as_milliseconds(Seconds time) { return time.value() * 1000.0; }

inline double as_degrees(Radians angle) {
  return angle.value() * 180.0 / 3.14159265358979323846;
}

#endif  // LESSON_SOLUTION_HPP
