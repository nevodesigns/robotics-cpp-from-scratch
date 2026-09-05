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

  // TODO 1: the constructor, and why it is explicit.
  //
  // Take a double and keep it. Mark it explicit, so that a bare number cannot
  // become a quantity by being passed to something that wanted one. That
  // refusal is half the value of the type: the conversion has to be written
  // down, at the edge, where somebody decided what the number meant.
  //
  // checks/a_bare_double_becoming_a_length.cpp is compiled by the build and
  // expected to fail. Leave off the explicit and that check starts compiling,
  // which the build reports as a failure.
  constexpr Quantity(double value) : value_(value) {}

  constexpr double value() const { return value_; }

  // TODO 2: the arithmetic that makes sense, and only that.
  //
  //   quantity + quantity   and   quantity - quantity   give a quantity
  //   -quantity                                         gives a quantity
  //   quantity * number     and   quantity / number     give a quantity
  //   quantity / quantity                               gives a plain number
  //
  // That last one is the interesting line. A length divided by a length is a
  // ratio, which has no unit, so it returns a double rather than a Quantity.
  //
  // Do not write a quantity times a quantity. A length times a length is an
  // area, which this system has no name for, so leaving it out is what makes
  // checks/multiplying_two_lengths.cpp fail to compile.
  constexpr Quantity operator+(Quantity other) const {
    return Quantity(value_ + other.value_);
  }
  constexpr Quantity operator-(Quantity other) const {
    return Quantity(value_ - other.value_);
  }
  constexpr Quantity operator-() const { return Quantity(-value_); }
  constexpr Quantity operator*(double scale) const { return Quantity(value_ * scale); }
  constexpr Quantity operator/(double scale) const { return Quantity(value_ / scale); }
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
// TODO 3: the conversions, written once.
//
// metres, millimetres, seconds, milliseconds, radians and degrees make a
// quantity from a number in a stated unit. as_millimetres, as_milliseconds and
// as_degrees take one back out.
//
// This is where a unit system earns its keep. Not in the arithmetic, which was
// probably right anyway, but in there being exactly one line in the program
// that knows a millimetre is a thousandth of a metre, instead of a factor of a
// thousand appearing wherever somebody remembered it.
//
// Pi is 3.14159265358979323846. The two degree functions cannot be constexpr
// alongside a division by it in the same style, so leave them plain inline.
constexpr Metres metres(double value) { return Metres(value); }
constexpr Metres millimetres(double value) { return Metres(value); }
constexpr Seconds seconds(double value) { return Seconds(value); }
constexpr Seconds milliseconds(double value) { return Seconds(value); }
constexpr Radians radians(double value) { return Radians(value); }

inline Radians degrees(double value) { return Radians(value); }

constexpr double as_millimetres(Metres length) { return length.value(); }
constexpr double as_milliseconds(Seconds time) { return time.value(); }

inline double as_degrees(Radians angle) { return angle.value(); }

#endif  // LESSON_SOLUTION_HPP
