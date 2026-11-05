// rc/rt/submit_future.hpp
//
// An answer back from a task on the pool, from lesson 07-13, graduated.
//
// Lesson 07-10's pool runs work and says nothing about it. A task returns void, so
// the answer to "what did it compute?" has nowhere to go, and a task that threw is
// counted and gone. The standard library has a machine for carrying an answer back
// from another thread, std::future, and three things about it were measured here.
//
// Measured on gcc 11. Four tasks that each wait up to 100 ms for the others, so
// that running together takes about 100 ms and one after another about 400 ms:
//
//                                         peak running at once    time
//   std::async, the futures discarded              1               400 ms
//   std::async, the futures kept                   4               100 ms
//
// std::async looks like a way to start work in parallel and a discarded future makes
// it serial: the future's destructor waits for the task, so the call that was meant
// to start it in the background does not return until it is done. Both gcc 11 and
// clang 14 warn about discarding it, as -Wunused-result from the nodiscard attribute,
// and the warning is the right one. Also measured, 2000 trivial std::async calls took
// 52 ms, about 26 microseconds each, a thread started per call, which is lesson 07-10's
// reason for a pool.
//
// And the default launch policy is a choice the library makes. On gcc 11 it ran on
// another thread. The standard allows deferred, which runs the task on the thread that
// asks for the answer, at the moment it asks. That is measured too: std::launch::deferred
// ran on the calling thread.
//
// A pool wants std::packaged_task, which wraps a callable and hands out a future. It is
// MOVE-ONLY, and the pool's tasks are std::function, which must be copyable. Measured:
//
//   error: static assertion failed: std::function target must be copy-constructible
//
// So the packaged_task lives in a std::shared_ptr and the lambda handed to the pool copies
// the pointer. One task, shared by the pool's copy and nobody else.
//
// What travels back through the future. A value, of any movable type: a std::string and
// a std::unique_ptr were returned. An exception: a task that threw
// runtime_error("sensor lost") made get() throw the same exception in the caller, which
// is what makes a failure in another thread a failure the caller has to deal with, where
// 07-10's pool only counted it. And the pool itself saw nothing wrong: completed() went up,
// failed() stayed at 0, because packaged_task caught the exception and kept it for the
// future. The caller is told, not the pool.
//
// One more, and the one that keeps a caller from hanging. If the pool refuses the task
// because it has shut down, the packaged_task is destroyed without running, and get()
// throws std::future_error with the code broken_promise. Measured: a task destroyed unrun
// gave "Broken promise", an exception, and not a wait for ever.
//
// The rule that falls out: keep the future, use a pool and not a thread per call, put the
// move-only task behind a shared_ptr, expect get() to throw, and never get() without a
// deadline you chose.

#ifndef RC_RT_SUBMIT_FUTURE_HPP
#define RC_RT_SUBMIT_FUTURE_HPP

#include <chrono>
#include <future>
#include <memory>
#include <utility>

#include <rc/rt/worker_pool.hpp>

namespace rc {
namespace rt {

// Runs fn on the pool and returns a future for what it returns.
//
// An exception from fn comes out of get(). A refusal, because the pool is shutting down,
// comes out of get() as broken_promise.
template <class F>
auto submit_future(WorkerPool& pool, F fn) -> std::future<decltype(fn())> {
  using Result = decltype(fn());

  // packaged_task is move-only and WorkerPool::Task is a std::function, which has to be
  // copyable, so the task is shared: the pool's copy of the lambda holds the pointer.
  std::shared_ptr<std::packaged_task<Result()>> task =
      std::make_shared<std::packaged_task<Result()>>(std::move(fn));
  std::future<Result> result = task->get_future();

  // If the pool refuses, the lambda is destroyed here, the last owner of the task goes
  // with it, and the future reports broken_promise instead of waiting for ever.
  pool.submit([task] { (*task)(); });
  return result;
}

// Whether the future is ready within a limit. Never blocks longer than that.
template <class R>
bool ready_within(std::future<R>& future, std::chrono::milliseconds limit) {
  return future.wait_for(limit) == std::future_status::ready;
}

}  // namespace rt
}  // namespace rc

#endif  // RC_RT_SUBMIT_FUTURE_HPP
