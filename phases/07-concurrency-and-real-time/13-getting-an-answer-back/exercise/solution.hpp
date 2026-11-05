#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <chrono>
#include <exception>
#include <future>
#include <memory>
#include <stdexcept>
#include <utility>

#include <rc/rt/worker_pool.hpp>

// The pool is the one the learner built in lesson 07-10, graduated into rc::rt. A
// using-declaration names exactly it, which is the safe tool lesson 01-17 measured.
using rc::rt::WorkerPool;

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

// TODO 1: run fn on the pool and return a future for its result.
//
//   using Result = decltype(fn());
//   std::shared_ptr<std::packaged_task<Result()>> task =
//       std::make_shared<std::packaged_task<Result()>>(std::move(fn));
//   std::future<Result> result = task->get_future();
//   pool.submit([task] { (*task)(); });
//   return result;
//
// The packaged_task is MOVE-ONLY and the pool takes a std::function, which must be
// copyable, so the task is held by a shared_ptr and the lambda copies the pointer. Try
// putting the packaged_task straight into the lambda and read the error.
//
// Do not catch anything. packaged_task catches an exception from fn and stores it in the
// future, and a refused submission destroys the task unrun, which the future reports as
// broken_promise.
//
// The stub returns a future that already holds an error, so a caller that gets it fails
// with a message and never waits for ever.
template <class F>
auto submit_future(WorkerPool& pool, F fn) -> std::future<decltype(fn())> {
  using Result = decltype(fn());
  (void)pool;
  (void)fn;
  std::promise<Result> unwritten;
  unwritten.set_exception(std::make_exception_ptr(std::runtime_error("submit_future is not written yet")));
  return unwritten.get_future();
}

// TODO 2: whether the future is ready within a limit.
//
// future.wait_for(limit) == std::future_status::ready. It never blocks longer than the
// limit, which is the point: get() would wait for ever on a task that never finishes.
//
// The stub says no.
template <class R>
bool ready_within(std::future<R>& future, std::chrono::milliseconds limit) {
  (void)future;
  (void)limit;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
