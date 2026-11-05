// rc/core/recursion.hpp
//
// A depth limit a recursive function can report against, from lesson 02-10,
// graduated.
//
// Lesson 02-01 taught that a pointer is where something is and 02-02 taught
// the heap. This covers where an ordinary local lives, how much room there is
// for them, and what running out looks like, which is: nothing. No exception,
// no error code, no last line of output. The program stops.
//
// Every number here is from a run on this curriculum's machine.
//
// Where things are. One program, four addresses: a local at 0x7ffd1795103f, a
// heap allocation at 0x5baa7c65deb0, a global at 0x5baa48796010, the code of
// main at 0x5baa48793259. The local is somewhere else entirely. The exact
// values change every run because the operating system moves things on
// purpose; the pattern does not.
//
// Which way it grows. A local one call deeper sat at a LOWER address than its
// caller's, by 48 bytes. The stack grows downwards, so "deeper" means
// numerically smaller, which is the opposite of what the word suggests.
//
// How much room. The limit is a setting, not a property of C++. 8192 KiB
// here, which `ulimit -s` prints and `setrlimit` changes. Windows fixes it at
// link time with a default of 1 MiB, so a recursion comfortable on Linux can
// end a program on Windows: at 64 byte frames that is 10922 safe levels
// against 87381.
//
// What running out looks like. A recursion with 64 byte frames reached depth
// 130860 and died with signal 11, reported by the shell as status 139. Lesson
// 00-09 explains that number, 128 plus the signal, and also why there was no
// final line of output: what was still buffered went with the process.
//
// 8 MiB over 130860 frames is 64 bytes a frame, so the arithmetic holds, and
// it is the arithmetic worth remembering: the depth you can afford is the
// limit divided by the frame. A frame holding a 1 KiB buffer buys 5461 levels
// rather than 87381.
//
// Two traps measured alongside.
//
// A large local costs its space on paths that never run it, because the frame
// is allocated when the function is entered rather than when the declaration
// is reached. A 15625 KiB array declared in main crashed the program on a
// branch that only used a vector and never touched the array. Moving the
// array into its own function made the vector branch survive.
//
// A variable length array, which is the quickest way to put a runtime sized
// object on the stack, is accepted silently by both gcc and clang under
// -Wall -Wextra. It is not standard C++; it is a C99 feature both offer as an
// extension. -Wvla or -pedantic-errors rejects it on both, and one of those
// belongs in any project that cares, which is lesson 04-08's subject.
//
// The purpose of this header is not to make recursion safe. It is to make the
// failure a value you can return rather than a signal that ends the process,
// so a planner handed a map deeper than it expected says so.

#ifndef RC_CORE_RECURSION_HPP
#define RC_CORE_RECURSION_HPP

namespace rc {
namespace core {

// A depth limit a recursive function can report against.
//
// The point is not to make recursion safe. It is to make the failure a value
// you can return rather than a signal that ends the process, so a planner
// given a map deeper than it expected tells you so.
class DepthLimit {
 public:
  explicit DepthLimit(int max_depth) : max_depth_(max_depth < 0 ? 0 : max_depth) {}

  int max_depth() const { return max_depth_; }
  int depth() const { return depth_; }

  // Whether the limit was reached at any point, which stays true afterwards.
  // A caller unwinding back to the top needs to know that something was cut
  // short, and the depth has gone back to zero by then.
  bool exceeded() const { return exceeded_; }

  // The deepest it actually went, which is the number worth printing: it says
  // whether the limit was close or nowhere near.
  int high_water() const { return high_water_; }

  void reset() {
    depth_ = 0;
    exceeded_ = false;
    high_water_ = 0;
  }

 private:
  friend class DepthGuard;
  int max_depth_ = 0;
  int depth_ = 0;
  int high_water_ = 0;
  bool exceeded_ = false;
};

// One level of recursion, counted on the way in and uncounted on the way out.
//
// A destructor rather than a decrement at the end of the function, for lesson
// 02-03's reason: an early return, a break or a throw all skip a line at the
// bottom and none of them skips a destructor. A depth counter that leaks is
// worse than none, because it reports a limit that was never reached.
class DepthGuard {
 public:
  explicit DepthGuard(DepthLimit& limit) : limit_(&limit) {
    if (limit_->depth_ >= limit_->max_depth_) {
      limit_->exceeded_ = true;
      ok_ = false;
      return;
    }
    ++limit_->depth_;
    if (limit_->depth_ > limit_->high_water_) limit_->high_water_ = limit_->depth_;
    counted_ = true;
  }

  ~DepthGuard() {
    if (counted_) --limit_->depth_;
  }

  DepthGuard(const DepthGuard&) = delete;
  DepthGuard& operator=(const DepthGuard&) = delete;

  // False when this level was refused, which the caller must check before
  // recursing. Returning rather than throwing keeps the decision at the call
  // site, where the caller knows what a partial answer is worth.
  bool ok() const { return ok_; }

 private:
  DepthLimit* limit_;
  bool ok_ = true;
  bool counted_ = false;
};

// A depth that fits a given stack, with room to spare.
//
// Two thirds rather than all of it, because the frame size you measured is
// for one function and the real call chain includes everything it calls,
// including library code you did not write.
inline int depth_that_fits(long stack_bytes, int bytes_per_frame) {
  if (stack_bytes <= 0 || bytes_per_frame <= 0) return 0;
  const long frames = stack_bytes / bytes_per_frame;
  const long with_margin = frames * 2 / 3;
  if (with_margin > 1000000) return 1000000;
  return static_cast<int>(with_margin);
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_RECURSION_HPP
