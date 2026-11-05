// rc/rt/seqlock.hpp
//
// A value one thread writes and many read, from lesson 07-12, graduated.
//
// Lesson 07-01 shared a reading with a mutex. That is correct and it is the right answer
// for a value read occasionally. A robot has another kind: one pose, written by one
// thread at a kilohertz, read by a dozen. Measured on this curriculum's machine, gcc 11,
// 20 microseconds between writes, a 32 byte pose whose four fields are always equal so a
// torn read can be seen, 300 ms, by the number of reader threads:
//
//                    1 reader   4 readers   8 readers     reads per ms, all readers
//   mutex               40774       10093        8503
//   shared_mutex        37521        5786        6351
//   seqlock            204873      790348      914603      (a second run: 127640, 747010, 991519)
//
//   writes the writer completed, of about 14,000 possible, with 8 readers:
//   mutex 13019   shared_mutex 92 (a second run: 71)   seqlock 14585
//
// Three findings, not one. The mutex gets SLOWER as readers are added, because every
// read takes the same lock, and 8 readers got a fifth of what one did. shared_mutex,
// built for exactly this, was worse than the plain mutex and starved the writer: 92
// writes in 300 ms where 14000 were wanted, because readers kept arriving before the
// last one left and the writer never got in. And the seqlock scaled with readers and
// never made the writer wait. No run of any of the three produced a torn read.
//
// How a seqlock works. A counter, odd while a write is in progress and even when none
// is. A writer makes it odd, writes the data, makes it even. A reader reads the counter,
// reads the data, reads the counter again, and trusts the data only if the counter was
// even and has not changed. Otherwise it throws the copy away and tries again. Readers
// take no lock and write nothing, so they cannot slow each other down or the writer.
//
// The price is exactly that retry, and it is counted. 8 readers retried about 110000
// times in 300 ms, a few per write. The data is also read while it may be changing, and
// that is the part that has to be done carefully.
//
// A seqlock that copies the value with memcpy reads memory another thread may be writing,
// which is a data race in C++ whatever the hardware does with it. Measured: it produced no
// torn reads here, 0 in 200000 writes. And clang 14's thread sanitizer reported it, 20
// data races in 5 runs, where gcc 11's reported nothing. A real race that happens to
// work, which one of the two tools finds and the other does not. Keeping the value as an
// array of atomic words, read and written with relaxed loads and stores between the
// counter's fences, is legal C++, and both sanitizers reported nothing on it.
//
// So the type is limited to those whose bytes are their value, trivially copyable, and
// a type that is not is refused at compile time, which checks/ proves.
//
// What this does not do. It has ONE writer. Two writers would interleave their odd and
// even steps and corrupt the counter. A reader of a value that changes faster than it can
// be read starves, retrying for ever, so it suits a value that changes in bursts and not
// one rewritten in a tight loop. Measured: 50000 writes with no pause against 3 readers
// gave 21117 successful reads and 501504 retries, about 24 retries for every read. And it hands readers a COPY, so it is for values small
// enough to copy in a few instructions.
//
// The rule that falls out: one writer, small plain values, readers that retry, the data
// held as atomic words, and a retry count somebody looks at.

#ifndef RC_RT_SEQLOCK_HPP
#define RC_RT_SEQLOCK_HPP

#include <atomic>
#include <cstddef>
#include <cstring>
#include <type_traits>

#include <rc/test/race.hpp>

namespace rc {
namespace rt {

template <class T>
class Seqlock {
  static_assert(std::is_trivially_copyable<T>::value,
                "a Seqlock copies its value as raw bytes, so the type must be trivially copyable");
 public:
  // The single writer. Odd means a write is in progress.
  void write(const T& value) {
    const unsigned begin = sequence_.load(std::memory_order_relaxed);
    sequence_.store(begin + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);

    unsigned long long words[kWords] = {};
    std::memcpy(words, &value, sizeof(T));
    for (std::size_t i = 0; i < kWords; ++i) data_[i].store(words[i], std::memory_order_relaxed);

    test::race_point("seqlock.write.middle");
    sequence_.store(begin + 2, std::memory_order_release);
  }

  // Any number of readers, from any thread. Retries until it has a whole value.
  T read() const {
    for (;;) {
      const unsigned before = sequence_.load(std::memory_order_acquire);
      if ((before & 1u) != 0) {
        retries_.fetch_add(1, std::memory_order_relaxed);
        continue;
      }

      unsigned long long words[kWords];
      for (std::size_t i = 0; i < kWords; ++i) words[i] = data_[i].load(std::memory_order_relaxed);
      std::atomic_thread_fence(std::memory_order_acquire);

      test::race_point("seqlock.read.middle");

      if (sequence_.load(std::memory_order_relaxed) == before) {
        T value;
        std::memcpy(&value, words, sizeof(T));
        return value;
      }
      retries_.fetch_add(1, std::memory_order_relaxed);
    }
  }

  // How many times a reader threw a copy away and tried again.
  long retries() const { return retries_.load(std::memory_order_relaxed); }

 private:
  static constexpr std::size_t kWords = (sizeof(T) + 7) / 8;

  std::atomic<unsigned> sequence_{0};
  std::atomic<unsigned long long> data_[kWords]{};
  mutable std::atomic<long> retries_{0};
};

}  // namespace rt
}  // namespace rc

#endif  // RC_RT_SEQLOCK_HPP
