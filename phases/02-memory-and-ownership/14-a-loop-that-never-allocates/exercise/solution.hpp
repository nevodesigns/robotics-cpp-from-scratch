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
    // TODO: if free_count_ is 0 return nullptr. Otherwise take the index off the top of the stack,
    // index = free_[--free_count_], mark taken_[index] true, reset slots_[index] to Message{} so a
    // borrower never sees the last one's numbers, and return its address.
    return nullptr;
  }

  // Take one back. False, and nothing changed, if this is not a message that is out.
  bool release(Message* message) {
    // TODO: four refusals, then the work.
    //  - nullptr.
    //  - an address outside slots_ (use std::less<const Message*> to compare, as in the comment).
    //  - an address inside a slot and not at the start of it: the byte offset from slots_ is not a
    //    multiple of sizeof(Message).
    //  - a slot that is not out (taken_[index] is false): taking it twice would lend it out twice.
    // Then taken_[index] = false and push the index back: free_[free_count_++] = index. Return true.
    (void)message;
    return false;
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
