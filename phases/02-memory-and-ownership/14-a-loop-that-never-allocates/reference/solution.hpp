#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <functional>

// The thing being handed out: one reading on its way somewhere.
struct Message {
  double value = 0.0;
  int id = 0;
};

// A fixed set of Messages that are lent out and taken back, and never allocated.
//
// All sixty-four exist from the start, inside the pool. Lending one is taking an index off a stack of free
// indices, and taking one back is putting it on. Nothing here asks the heap for anything, which is what
// a loop that has to run on time needs: the heap's time depends on what else has happened to it.
class MessagePool {
 public:
  static const int kCapacity = 64;

  MessagePool() {
    // Lowest index on top, so the first lend is slot 0.
    for (int i = 0; i < kCapacity; ++i) {
      free_[i] = kCapacity - 1 - i;
      taken_[i] = false;
    }
    free_count_ = kCapacity;
  }

  // Lend a message, or nullptr if all of them are out.
  Message* acquire() {
    if (free_count_ == 0) return nullptr;
    const int index = free_[--free_count_];
    taken_[index] = true;
    slots_[index] = Message{};   // a lent message starts clean, not as the last borrower left it
    return &slots_[index];
  }

  // Take one back. False, and nothing changed, if this is not a message that is out.
  bool release(Message* message) {
    if (message == nullptr) return false;

    // Is it inside the pool at all? std::less, because comparing pointers that do not point into the
    // same array with < is not something the language promises an answer to.
    std::less<const Message*> before;
    if (before(message, slots_) || !before(message, slots_ + kCapacity)) return false;

    // Does it point at the start of a slot, and not into the middle of one?
    const std::size_t offset = static_cast<std::size_t>(reinterpret_cast<const char*>(message) -
                                                        reinterpret_cast<const char*>(slots_));
    if (offset % sizeof(Message) != 0) return false;

    const int index = static_cast<int>(offset / sizeof(Message));
    if (!taken_[index]) return false;   // it is already back: taking it twice would lend it out twice

    taken_[index] = false;
    free_[free_count_++] = index;
    return true;
  }

  int in_use() const { return kCapacity - free_count_; }
  int capacity() const { return kCapacity; }

 private:
  Message slots_[kCapacity];
  int free_[kCapacity];
  bool taken_[kCapacity];
  int free_count_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
