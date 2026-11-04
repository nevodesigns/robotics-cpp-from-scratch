#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>

// Lesson 03-05 handed you an interface and asked you to implement it, because
// an injectable clock was the only way to give dt a provenance. This lesson is
// about what that machinery actually does, what it costs, and the two ways it
// goes silently wrong.
//
// Three interfaces exist in librc already: rc::core::Clock, rc::io::BytePort
// and rc::io::SensorSource. Each of them opens with a line whose necessity is
// not obvious until you have seen what happens without it.

// Counters, so the suite can watch which destructors ran rather than trust a
// claim that they did. At file scope because lesson 05-06's rule applies: a
// test that changes shared state puts it back, and these are reset rather than
// assumed.
struct DestructionLog {
  int base_destroyed = 0;
  int derived_destroyed = 0;
  void reset() {
    base_destroyed = 0;
    derived_destroyed = 0;
  }
};

inline DestructionLog& destruction_log() {
  static DestructionLog log;
  return log;
}

// An interface written the way the three in librc are written.
//
// Two lines carry their weight here and neither is decoration.
//
// The virtual destructor. Deleting a derived object through a base pointer
// calls the base destructor only, unless the base destructor is virtual. The
// derived one never runs, so whatever it was going to release is not released.
// That case is undefined behaviour rather than merely wasteful, and the
// measurements are in the lesson document, because a test cannot safely
// perform it.
//
// The deleted copy operations. Copying a derived object into a base shaped
// variable is perfectly legal and silently throws the derived part away, which
// is called slicing. Deleting the copy in the base turns that from a wrong
// answer at runtime into a refusal at compile time, and checks/ holds the
// refusal.
class Sensor {
 public:
  // The virtual is given to you rather than left as an exercise, and the
  // reason is the lesson itself: an exercise that asked you to leave it out
  // would be asking you to ship undefined behaviour, which the suite cannot
  // safely run and the address sanitizer stops outright. You cannot test for
  // this trap at runtime. You require it at compile time, which is what the
  // static_assert in the suite and the third file in checks/ do.
  //
  // TODO 1: count the destruction.
  //
  // Increment base_destroyed here and derived_destroyed in Thermometer, so
  // the suite can watch which destructors actually ran and in which order
  // rather than take it on trust.
  virtual ~Sensor() {}

  // TODO 2: delete the copy operations.
  //
  // Copying a Thermometer into a Sensor shaped variable is legal and throws
  // the Thermometer part away. That is slicing, and neither gcc nor clang
  // warns about it, even with clang's -Weverything.
  //
  // An interface is not a value, so there is no sensible meaning for copying
  // one. Deleting the copy constructor and the copy assignment turns a wrong
  // answer at runtime into a refusal at compile time, and checks/ holds the
  // refusal that proves it.

  virtual double read() const = 0;
  virtual const char* name() const = 0;

 protected:
  // Protected rather than public, because an interface is not something you
  // instantiate on its own.
  Sensor() = default;
};

class Thermometer : public Sensor {
 public:
  explicit Thermometer(double celsius) : celsius_(celsius) {}
  ~Thermometer() override {}  // TODO 1 continues here

  double read() const override { return celsius_; }
  const char* name() const override { return "thermometer"; }

 private:
  double celsius_ = 0.0;
};

// The same hierarchy with the copy left in, so the suite can measure what
// slicing actually does. This one is not a model to follow; it is the subject
// of the measurement.
class LooseSensor {
 public:
  virtual ~LooseSensor() = default;
  virtual double read() const { return 0.0; }
};

class LooseThermometer : public LooseSensor {
 public:
  explicit LooseThermometer(double celsius) : celsius_(celsius) {}
  double read() const override { return celsius_; }

 private:
  double celsius_ = 0.0;
};

// Two shapes whose only difference is one virtual function, so the suite can
// measure what that costs in space.
struct PlainReading {
  int id = 0;
  double value = 0.0;
};

struct PolymorphicReading {
  int id = 0;
  double value = 0.0;
  virtual ~PolymorphicReading() = default;
};

// Dispatch through a reference keeps the dynamic type. Taking the parameter by
// value cannot, because the parameter is a LooseSensor and that is all the
// space there is.
inline double read_through_reference(const LooseSensor& s) { return s.read(); }

// TODO 3: take the parameter by value.
//
// Change the parameter from a reference to a LooseSensor by value, and watch
// what the suite then reports. The function body does not change. Only the
// parameter does, and the answer changes with it, because the copy is a
// LooseSensor and that is all the space there is.
inline double read_by_value(const LooseSensor& s) { return s.read(); }

// A direct call on a known type, for the comparison against a virtual one.
inline double read_directly(const LooseThermometer& t) { return t.LooseThermometer::read(); }

#endif  // LESSON_SOLUTION_HPP
