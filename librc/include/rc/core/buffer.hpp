// rc/core/buffer.hpp
//
// The owning buffer from lesson 02-05, graduated.
//
// Written out by hand once, so that the five operations a resource owning type
// has to get right are something you have done rather than something you have
// read about. In real code the answer is std::vector, and knowing exactly what
// it is doing for you is the point of having written this.

#ifndef RC_CORE_BUFFER_HPP
#define RC_CORE_BUFFER_HPP

#include <algorithm>
#include <cstddef>
#include <utility>

namespace rc {
namespace core {

class SampleBuffer {
 public:
  explicit SampleBuffer(std::size_t size = 0)
      : size_(size), data_(size > 0 ? new double[size]() : nullptr) {}

  // delete[] on a null pointer is defined and does nothing, so an empty buffer
  // needs no special case here.
  ~SampleBuffer() { delete[] data_; }

  // A deep copy: new memory, same values. Allocating in the member initialiser
  // list means the object is either fully built or the constructor threw and it
  // never existed, so there is no half built state to clean up.
  SampleBuffer(const SampleBuffer& other)
      : size_(other.size_), data_(other.size_ > 0 ? new double[other.size_] : nullptr) {
    std::copy(other.data_, other.data_ + other.size_, data_);
  }

  // Copy and swap. Three lines that solve three problems at once:
  //
  //   self assignment  the copy is made before anything is released
  //   exception safety the allocation happens before this object is modified
  //   duplication      the copy constructor is the only place deep copying lives
  //
  // The temporary takes the old contents away with it when it is destroyed at
  // the closing brace.
  SampleBuffer& operator=(const SampleBuffer& other) {
    SampleBuffer copy(other);
    swap(copy);
    return *this;
  }

  // Take the buffer, then clear the source. Clearing is the important half: the
  // source's destructor still runs, and it must not free what was just taken.
  //
  // noexcept matters more than it looks. std::vector only moves its elements
  // while growing if their move constructor promises not to throw, and copies
  // them otherwise, so a missing noexcept is a silent slowdown.
  SampleBuffer(SampleBuffer&& other) noexcept
      : size_(other.size_), data_(other.data_) {
    other.size_ = 0;
    other.data_ = nullptr;
  }

  // Guarding against self assignment matters here too. Without the check,
  // moving an object onto itself would release its own buffer and then adopt
  // the pointer it just freed.
  SampleBuffer& operator=(SampleBuffer&& other) noexcept {
    if (this == &other) return *this;

    delete[] data_;
    size_ = other.size_;
    data_ = other.data_;
    other.size_ = 0;
    other.data_ = nullptr;
    return *this;
  }

  void swap(SampleBuffer& other) noexcept {
    std::swap(size_, other.size_);
    std::swap(data_, other.data_);
  }

  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  double at(std::size_t index) const { return data_[index]; }
  void set(std::size_t index, double value) { data_[index] = value; }

 private:
  std::size_t size_ = 0;
  double* data_ = nullptr;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_BUFFER_HPP
