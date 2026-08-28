#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <utility>

// A buffer of samples that owns its memory directly.
//
// Implement all five special operations. The tests check independence after a
// copy, survival of self assignment, the state of a moved from object, and that
// every allocation is matched by a release.
class SampleBuffer {
 public:
  explicit SampleBuffer(std::size_t size = 0)
      : size_(size), data_(size > 0 ? new double[size]() : nullptr) {}

  // TODO: the destructor. Release what this object owns, and nothing else.
  ~SampleBuffer() {}

  // TODO: the copy constructor. The new buffer must own its own memory holding
  // the same values, so that changing one cannot be seen through the other.
  SampleBuffer(const SampleBuffer& other) : size_(0), data_(nullptr) { (void)other; }

  // TODO: copy assignment. This one already owns something, and the source may
  // be this very object. Copy and swap handles both problems at once:
  //
  //   SampleBuffer copy(other);
  //   swap(copy);
  //   return *this;
  SampleBuffer& operator=(const SampleBuffer& other) {
    (void)other;
    return *this;
  }

  // TODO: the move constructor. Take the buffer, then clear the source, because
  // the source's destructor will still run. Mark it noexcept.
  SampleBuffer(SampleBuffer&& other) : size_(0), data_(nullptr) { (void)other; }

  // TODO: move assignment. Release what this holds, take the source's, clear the
  // source. Mark it noexcept.
  SampleBuffer& operator=(SampleBuffer&& other) {
    (void)other;
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

#endif  // LESSON_SOLUTION_HPP
