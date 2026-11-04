#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <type_traits>

// A robot keeps the last N of things. The last twenty encoder readings to
// smooth, the last hundred poses to draw a trail, the last fifty loop times to
// find the slow one. Always the last N, never all of them, because a machine
// that runs for a week cannot keep a week of samples.
//
// Written once, for every type, which is what a template is for. The phase is
// named for classes and templates and this is the template.

// A fixed capacity buffer that forgets the oldest thing when it is full.
//
// Two template parameters, and they are different kinds. T is a type
// parameter, which is the one everybody means by "template". Capacity is a non
// type parameter, a number known when the code is compiled, and that is what
// lets the storage be a plain array inside the object with no allocation
// anywhere.
template <class T, std::size_t Capacity>
class Ring {
  // A ring with no room is not a smaller ring, it is a thing with no coherent
  // behaviour: every operation would be an error. Refusing it at compile time
  // costs one line and removes the question.
  static_assert(Capacity > 0,
                "a Ring needs room for at least one element. Ring<double, 0> has "
                "no coherent behaviour, so pick a capacity.");

 public:
  // Known at compile time, so it can be used where a constant is required.
  static constexpr std::size_t capacity() { return Capacity; }

  std::size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }
  bool full() const { return count_ == Capacity; }

  void clear() {
    first_ = 0;
    count_ = 0;
  }

  // Where the element at this logical position actually sits.
  //
  // This one line is the whole idea of a ring buffer. Positions count from the
  // oldest, and the oldest moves, so the logical position and the slot in the
  // array are not the same number. The remainder brings it back round.
  // TODO 1: which slot in the array holds the element at this position.
  //
  // One line, and it is the whole idea of a ring buffer. Positions count from
  // the oldest element, and the oldest moves as the buffer is reused, so a
  // position and an array slot are not the same number.
  //
  // Start at first_, step forward by index, and bring it back round with the
  // remainder operator. The test checks that every slot is below the capacity
  // after a hundred pushes into a ring of three, so a version that walks off
  // the end will not hide.
  std::size_t slot(std::size_t index) const {
    (void)index;
    return 0;
  }

  // Add a reading, forgetting the oldest if there is no room.
  //
  // Overwriting rather than refusing is the decision that makes this useful
  // for a history. A sensor does not stop producing readings because your
  // buffer is full, and the newest reading is the one you want least to lose.
  // TODO 2: add a reading, forgetting the oldest if there is no room.
  //
  // Two cases.
  //
  // There is room: the new element goes at position count_, which is one past
  // the last one held, and the count goes up.
  //
  // There is not: the slot holding the oldest element is the one to reuse,
  // because it is the one you are willing to lose. Write there, and then the
  // position after it becomes the new oldest, so first_ steps forward and
  // wraps. The count does not change, because the buffer was already full.
  //
  // Overwriting rather than refusing is the decision that makes this useful
  // as a history: a sensor does not stop producing readings because your
  // buffer is full, and the newest reading is the one you least want to lose.
  void push(const T& value) {
    (void)value;
  }

  // Position 0 is the oldest reading still held, not the first one ever
  // pushed. Counting from the oldest is what makes a moving window natural:
  // every reading keeps its order relative to the others.
  // TODO 3: the element at this position, counting from the oldest.
  //
  // Position 0 is the oldest reading still held, not the first one ever
  // pushed. Counting from the oldest is what makes a moving window natural,
  // because every reading keeps its order relative to the others.
  //
  // One line, using the function from TODO 1.
  const T& operator[](std::size_t index) const {
    (void)index;
    return storage_[0];
  }

  const T& oldest() const { return storage_[first_]; }
  const T& newest() const { return storage_[slot(count_ - 1)]; }

 private:
  T storage_[Capacity] = {};
  std::size_t first_ = 0;
  std::size_t count_ = 0;
};

// The mean of what a ring is holding.
//
// A function template over both parameters, so it works for any ring without
// naming a capacity. The compiler works both out from the argument.
//
// The static_assert is the point of this function existing in the lesson. A
// template has no declared interface: it compiles against whatever type it is
// given and fails wherever the body happens to use something that type lacks.
// Without the assert, averaging a type that cannot be added produces an error
// inside this function, or inside whatever this function called, and names a
// file the caller has never opened.
template <class T, std::size_t Capacity>
double average(const Ring<T, Capacity>& ring) {
  // TODO 4: the mean of what the ring is holding.
  //
  // Start with the static_assert. std::is_arithmetic<T>::value is true for
  // int, double and the rest, and false for a struct. Write a message that
  // tells the caller what to pass instead, because that sentence is the first
  // line of the error they will read.
  //
  // Then an empty ring, which averages to zero rather than dividing by it.
  //
  // Then the sum. Accumulate into a double whatever T is, so that a
  // Ring<int, N> averages to 2.5 rather than to 2.
  //
  // Note that nothing here mentions a capacity. Capacity is a template
  // parameter the compiler works out from the argument, which is why one
  // function serves every ring.
  (void)ring;
  return 0.0;
}

// Counts how many distinct types a template was instantiated for.
//
// Each instantiation of a template is separate code with its own statics, and
// this makes that visible rather than asserted. Instantiated<double> and
// Instantiated<int> do not share a counter, because they are not the same
// class.
template <class T>
struct Instantiated {
  static int uses;
  static void note() { ++uses; }
};

template <class T>
int Instantiated<T>::uses = 0;

#endif  // LESSON_SOLUTION_HPP
