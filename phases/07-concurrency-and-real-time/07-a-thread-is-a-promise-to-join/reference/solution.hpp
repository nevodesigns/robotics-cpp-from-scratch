#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>

// Every thread in phase 07 so far has been started and then joined, in a test, by
// someone who remembered. Nothing has said what happens to one that is not.
//
// A std::thread that is started is a promise: somebody will either join it or
// detach it before it goes away. Break the promise and the program does not
// complain politely.
//
// Each of these was measured as its own program, because each ends the process.
// gcc 11, run on this curriculum's machine, with the exit status the shell saw.
//
//   a thread destroyed without join        terminate called without an active
//                                          exception, status 134
//   a thread whose body throws             terminate called after throwing an
//                                          instance of std::runtime_error,
//                                          what(): sensor lost, status 134
//   an exception thrown after the thread   status 134, and the catch block that
//   started and before join                was waiting for it never ran
//   a detached thread using a local        the plain build printed 1 where the
//   of the function that started it        value should have been 42, and exited
//                                          with status 0
//
// Status 134 is 128 plus SIGABRT, which lesson 00-09 explained, and like every
// crash in this curriculum it arrives with no last line of output.
//
// The third is the one that catches careful programmers. The join is on the next
// line and the function is correct, but a throw between the two unwinds the
// stack, the thread object is destroyed while joinable, and std::terminate runs
// before any catch can. Correct code on the path that does not throw is not
// correct code. This is lesson 02-03's argument again: attach the cleanup to the
// object and not to the code path.
//
// The fourth is quieter. A detached thread outlives the function that started it
// and any local it refers to. The plain build ran to completion and exited 0 with
// the wrong number. The address sanitizer, with its default options, ran it
// without a report and printed a different wrong number, 200000001. With
// ASAN_OPTIONS=detect_stack_use_after_return=1, which lesson 02-10 explained, both
// gcc and clang reported stack-use-after-return at the line that read the local.
// The thread sanitizer reported a data race. Detach is almost never the answer.
//
// So the thread is held by an object whose destructor joins. That is all a scoped
// thread is, and it is worth more than it looks: every path out of the scope,
// returning, throwing, an early break, ends in a join.
//
// It also owns the exception question. An exception that escapes a thread body
// cannot be caught by anyone: there is no caller to catch it. So the body is
// wrapped, and what escaped is kept and reported on join, as a value the caller
// has to look at. A thread that fails has to be able to say so.
//
// The rule that falls out: never hold a bare std::thread; join in a destructor; do
// not detach; catch at the top of every thread body; ask a thread how it ended.

// A thread that is always joined. The state it reports into lives on the heap, so
// moving a ScopedThread does not move the memory the running thread is writing to.
class ScopedThread {
 public:
  // Starts a thread running body. An exception that escapes body is caught and
  // kept, and failed() and error() report it after the thread has finished.
  explicit ScopedThread(std::function<void()> body) : state_(new State) {
    State* state = state_.get();
    thread_ = std::thread([state, body] {
      try {
        body();
      } catch (const std::exception& e) {
        state->error = e.what();
        state->failed = true;
      } catch (...) {
        state->error = "unknown exception";
        state->failed = true;
      }
    });
  }

  // Every way out of a scope ends here, including a throw.
  ~ScopedThread() { join(); }

  // One owner. A copy would be two objects each promising to join the same thread.
  ScopedThread(const ScopedThread&) = delete;
  ScopedThread& operator=(const ScopedThread&) = delete;

  ScopedThread(ScopedThread&&) noexcept = default;

  // Assigning over a running thread must join it first. The defaulted assignment
  // would destroy a joinable std::thread, and that is std::terminate.
  ScopedThread& operator=(ScopedThread&& other) noexcept {
    if (this != &other) {
      join();
      thread_ = std::move(other.thread_);
      state_ = std::move(other.state_);
    }
    return *this;
  }

  // Waits for the thread to finish. Harmless to call twice, or on a thread that
  // was moved from.
  void join() {
    if (thread_.joinable()) thread_.join();
  }

  bool joinable() const { return thread_.joinable(); }

  // Whether the body threw. Asking waits for the thread, which is also what makes
  // reading the result safe: join is the point where the thread's writes become
  // visible.
  bool failed() {
    join();
    return state_ && state_->failed;
  }

  // The message of what escaped, or empty.
  std::string error() {
    join();
    return state_ ? state_->error : std::string();
  }

 private:
  struct State {
    bool failed = false;
    std::string error;
  };

  std::unique_ptr<State> state_;
  std::thread thread_;
};

#endif  // LESSON_SOLUTION_HPP
