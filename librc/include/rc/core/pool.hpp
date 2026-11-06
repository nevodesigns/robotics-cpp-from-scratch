// rc/core/pool.hpp
//
// A fixed pool of messages, lent out and taken back, from lesson 02-14, graduated.
//
// A control loop that has to run on time cannot ask the heap for memory, because how long the heap takes depends on everything
// that happened to it before. All sixty-four messages exist from the start inside the pool: lending one takes an index off a stack of
// free ones and taking it back puts it on. Measured with a count of every allocation, 100000 lend and take back cycles made 0 heap
// allocations through the pool and 100000 through new. Taking a message back is checked: a null pointer, an address outside the pool,
// an address in the middle of a slot and a second return of the same message are all refused, so the pool cannot lend one slot to two users.

#ifndef RC_CORE_POOL_HPP
#define RC_CORE_POOL_HPP

#include <cstddef>
#include <functional>

namespace rc {
namespace core {

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

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_POOL_HPP
