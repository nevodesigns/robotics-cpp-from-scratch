#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <memory>

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

inline Smoother::Smoother(double alpha) : impl_(new Impl) {
  // TODO: store alpha in impl_->alpha.
  (void)alpha;
}

inline Smoother::Smoother(const Smoother& other) : impl_() {
  // TODO: a DEEP copy: impl_.reset(new Impl(*other.impl_)). Copying the pointer would not compile for
  // a unique_ptr, and sharing the state would make two Smoothers feed each other.
  (void)other;
}

inline Smoother::Smoother(Smoother&& other) noexcept = default;

inline Smoother& Smoother::operator=(const Smoother& other) {
  // TODO: if this is not the same object, give this one a copy of the other's Impl, as in the copy
  // constructor. Return *this.
  (void)other;
  return *this;
}

inline Smoother& Smoother::operator=(Smoother&& other) noexcept = default;

// The destructor of a unique_ptr to Impl has to be able to delete an Impl, so it must be written where Impl is
// complete: here, after the definition above.
inline Smoother::~Smoother() = default;

inline double Smoother::update(double reading) {
  // TODO: the first reading (count 0) becomes the value. After that, value += alpha * (reading - value).
  // Count every reading, and return the value.
  (void)reading;
  return 0.0;
}

inline int Smoother::count() const {
  // TODO: impl_->count.
  return -1;
}

#endif  // LESSON_SOLUTION_HPP
