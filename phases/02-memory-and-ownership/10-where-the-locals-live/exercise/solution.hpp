#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Lesson 02-01 taught that a pointer is where something is. Lesson 02-02
// taught the heap. Nobody has yet said where an ordinary local variable
// lives, how much room there is for them, or what happens when you run out.
//
// There is a lot of room and it is not unlimited, and running out does not
// throw, does not return an error, and does not print anything. The program
// stops.
//
// Measured on this curriculum's machine, and every number below is from a run
// rather than a manual.
//
// Where things are. One program, five addresses:
//
//   a local in main        0x7ffd1795103f
//   a heap allocation      0x5baa7c65deb0
//   a global               0x5baa48796010
//   a static at file scope 0x5baa48796014
//   the code of main       0x5baa48793259
//
// The local is somewhere else entirely. Globals, statics and code sit near
// each other low down, the heap above them, and the stack far above both. The
// exact numbers change every run, because the operating system moves things
// about on purpose, and the pattern does not.
//
// Which way it grows. A local one call deeper was at a LOWER address than its
// caller's, by 48 bytes. The stack grows downwards, so "deeper" means
// numerically smaller, which is the opposite of what the word suggests.
//
// How much room. The limit is a setting, not a property of C++: 8192 KiB here,
// which `ulimit -s` prints and `setrlimit` changes. Windows fixes it at link
// time instead, with a default of 1 MiB, so a recursion that is comfortable on
// Linux can end a program on Windows.
//
// What running out looks like. A function recursing with a small frame reached
// a depth of 130860 and then died with signal 11, which the shell reported as
// status 139. Lesson 00-09 explains that number: 128 plus the signal. No
// message, no exception, no last line of output, because the output that was
// buffered went with it.
//
// 8 MiB over 130860 frames is 64 bytes a frame. That is the arithmetic worth
// remembering: the depth you can afford is the limit divided by the frame, and
// a frame holding a 1 KiB buffer gives you eight thousand calls rather than a
// hundred and thirty thousand.

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
  // TODO 1: decide whether this level is allowed, and count it if it is.
  //
  // If the current depth has already reached the limit, refuse: record that
  // the limit was exceeded, set ok_ to false, and count nothing. Otherwise
  // take the level, update the high water mark, and remember that you took
  // it so the destructor knows to give it back.
  //
  // The stub below refuses everything, which is the safe direction to be
  // wrong in. A stub that allowed everything would recurse until the stack
  // ran out, and an exercise that ends in signal 11 teaches nothing about
  // which function to write.
  explicit DepthGuard(DepthLimit& limit) : limit_(&limit) {
    ok_ = false;
  }

  // TODO 2: give the level back, if this guard took one.
  //
  // One line, and the condition matters: a guard that refused took nothing
  // and must not decrement, or the counter drifts below zero and the limit
  // stops meaning anything.
  ~DepthGuard() {}

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
// TODO 3: how deep can this stack go.
//
// The limit divided by the frame, and then two thirds of that, because the
// frame you measured is one function and the real call chain includes
// everything it calls, library code included.
//
// Guard the nonsense: a zero or negative stack, and a zero or negative
// frame, must give 0 rather than dividing by zero. And cap the answer, so a
// tiny frame on a large stack does not return a depth no int can hold.
inline int depth_that_fits(long stack_bytes, int bytes_per_frame) {
  (void)stack_bytes;
  (void)bytes_per_frame;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
