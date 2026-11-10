// rc/core/smoother.hpp
//
// A filter whose inside is hidden, from lesson 04-14, graduated.
//
// A class that keeps its data behind a pointer to a type that is only named can grow without changing what a caller was built against. A Smoother is 8 bytes, the size of
// one pointer. The same data in the class took 24 bytes in version 1 and 104 in version 2 once a history of ten readings was added, while the pointer versions were 8 and 8, so code built against the old
// header reserves the wrong amount for the new object. The destructor must be declared in the class and defined where the hidden type is complete (a compiler written one fails with an invalid application of
// sizeof to an incomplete type, checked by a snippet in the lesson), and the copy must be deep, since a unique_ptr cannot be copied and shared state would make two filters feed each other.

#ifndef RC_CORE_SMOOTHER_HPP
#define RC_CORE_SMOOTHER_HPP

#include <memory>

namespace rc {
namespace core {

// ---------------------------------------------------------------------------
// What a caller sees: the part that goes in the header.
//
// Nothing about the inside appears here except that there is one. Code that uses a Smoother is built
// against this and against sizeof(Smoother), which is one pointer, so the inside can grow without
// changing either.
// ---------------------------------------------------------------------------
class Smoother {
 public:
  explicit Smoother(double alpha);
  Smoother(const Smoother& other);                // a deep copy: the copy gets its own state
  Smoother(Smoother&& other) noexcept;
  Smoother& operator=(const Smoother& other);
  Smoother& operator=(Smoother&& other) noexcept;
  ~Smoother();                                    // declared here and defined below, where Impl is complete

  // Feed one reading in and get the smoothed value out.
  double update(double reading);

  // How many readings have been fed in.
  int count() const;

 private:
  struct Impl;                                    // named and not defined: an incomplete type
  std::unique_ptr<Impl> impl_;
};

// ---------------------------------------------------------------------------
// What only the implementation sees: the part that would go in the .cpp file.
//
// Add a field here, say a history of the last ten readings, and nothing above changes: the callers do
// not need to be built again, and a library built with the old inside still works with the new.
// ---------------------------------------------------------------------------
struct Smoother::Impl {
  double alpha = 0.5;
  double value = 0.0;
  int count = 0;
};

inline Smoother::Smoother(double alpha) : impl_(new Impl) { impl_->alpha = alpha; }

inline Smoother::Smoother(const Smoother& other) : impl_(new Impl(*other.impl_)) {}

inline Smoother::Smoother(Smoother&& other) noexcept = default;

inline Smoother& Smoother::operator=(const Smoother& other) {
  if (this != &other) impl_.reset(new Impl(*other.impl_));
  return *this;
}

inline Smoother& Smoother::operator=(Smoother&& other) noexcept = default;

// The destructor of a unique_ptr to Impl has to be able to delete an Impl, so it must be written where Impl is
// complete. Left to the compiler in the header it would be generated where Impl is only named.
inline Smoother::~Smoother() = default;

inline double Smoother::update(double reading) {
  if (impl_->count == 0) {
    impl_->value = reading;                        // the first reading is the start, not pulled towards zero
  } else {
    impl_->value += impl_->alpha * (reading - impl_->value);
  }
  ++impl_->count;
  return impl_->value;
}

inline int Smoother::count() const { return impl_->count; }

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_SMOOTHER_HPP
