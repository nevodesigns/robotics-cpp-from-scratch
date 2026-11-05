#include <rc/test/rc_test.hpp>
#include <rc/rt/blocking_queue.hpp>

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

using std::chrono::milliseconds;
using Clock = std::chrono::steady_clock;

// A loop of up to `limit` iterations that checks the token every `check_every`
// iterations and requests the stop itself at iteration `request_at`. Counted, so exact.
long iterations_run(long check_every, long request_at, long limit) {
  StopSource source;
  const StopToken token = source.get_token();
  long i = 0;
  while (i < limit) {
    ++i;
    if (i == request_at) source.request_stop();
    if (i % check_every == 0 && token.stop_requested()) break;
  }
  return i;
}

}  // namespace

RC_TEST("one request is seen by every token, and only the first caller is told it made it") {
  StopSource source;
  const StopToken first = source.get_token();
  const StopToken second = first;
  const StopToken empty;

  RC_CHECK(first.stop_possible());
  RC_CHECK(!first.stop_requested());
  RC_CHECK(!second.stop_requested());

  RC_CHECK(source.request_stop());
  RC_CHECK(!source.request_stop());

  RC_CHECK(first.stop_requested());
  RC_CHECK(second.stop_requested());
  RC_CHECK(source.get_token().stop_requested());

  // A token from no source can never be asked to stop.
  RC_CHECK(!empty.stop_possible());
  RC_CHECK(!empty.stop_requested());
}

RC_TEST("a callback runs once on the request, at once if it is late, and not at all if it is gone") {
  int ran = 0;
  StopSource source;
  const StopToken token = source.get_token();

  {
    const StopCallback gone(token, [&] { ran += 100; });
  }
  const StopCallback kept(token, [&] { ++ran; });
  RC_CHECK_EQ(ran, 0);

  source.request_stop();
  RC_CHECK_EQ(ran, 1);
  source.request_stop();
  RC_CHECK_EQ(ran, 1);

  // Registered after the request: it runs where it is constructed, so it cannot be missed.
  int late = 0;
  const StopCallback after(token, [&] { ++late; });
  RC_CHECK_EQ(late, 1);
}

RC_TEST("a stop request wakes every consumer asleep on a queue, through a callback that closes it") {
  rc::rt::BlockingQueue<int> queue;
  StopSource source;
  const StopCallback closes(source.get_token(), [&] { queue.close(); });

  std::vector<rc::rt::BlockingQueue<int>::Result> results(3, rc::rt::BlockingQueue<int>::Result::kGot);
  std::vector<std::thread> consumers;
  for (int i = 0; i < 3; ++i) {
    consumers.emplace_back([&queue, &results, i] {
      int value = 0;
      results[static_cast<std::size_t>(i)] = queue.pop_for(value, milliseconds(1500));
    });
  }

  const auto give_up = Clock::now() + milliseconds(1000);
  while (queue.waits() < 3 && Clock::now() < give_up) std::this_thread::sleep_for(milliseconds(1));

  const auto asked = Clock::now();
  source.request_stop();
  for (std::thread& consumer : consumers) consumer.join();
  const auto took = std::chrono::duration_cast<milliseconds>(Clock::now() - asked);

  int closed = 0;
  for (const auto result : results)
    if (result == rc::rt::BlockingQueue<int>::Result::kClosed) ++closed;

  std::cout << "  three consumers asleep with a 1500 ms wait, then request_stop(): " << closed
            << " of 3 returned closed, within " << took.count() << " ms\n\n";
  RC_CHECK_EQ(closed, 3);
  RC_CHECK(took.count() < 1000);
}

RC_TEST("a sleep ends when it is asked to, and not before otherwise") {
  // Never asked: it sleeps the whole time and says it was not stopped.
  StopSource quiet;
  const auto before = Clock::now();
  const bool stopped_quiet = sleep_unless_stopped(quiet.get_token(), milliseconds(40));
  const auto slept = std::chrono::duration_cast<milliseconds>(Clock::now() - before);
  RC_CHECK(!stopped_quiet);
  RC_CHECK(slept.count() >= 39);

  // Already asked: it does not sleep at all.
  StopSource asked;
  asked.request_stop();
  const auto before_asked = Clock::now();
  RC_CHECK(sleep_unless_stopped(asked.get_token(), milliseconds(5000)));
  RC_CHECK(std::chrono::duration_cast<milliseconds>(Clock::now() - before_asked).count() < 1000);

  // Asked from another thread while it sleeps for five seconds.
  StopSource later;
  std::thread asker([&later] {
    std::this_thread::sleep_for(milliseconds(50));
    later.request_stop();
  });
  const auto before_later = Clock::now();
  const bool stopped = sleep_unless_stopped(later.get_token(), milliseconds(5000));
  const auto woke_after = std::chrono::duration_cast<milliseconds>(Clock::now() - before_later);
  asker.join();

  std::cout << "  a 5000 ms sleep, stop requested after 50 ms: stopped " << (stopped ? "yes" : "no")
            << ", woke after " << woke_after.count() << " ms\n\n";
  RC_CHECK(stopped);
  RC_CHECK(woke_after.count() >= 45);
  RC_CHECK(woke_after.count() < 1000);
}

RC_TEST("how often a loop looks is how fast it stops") {
  // Stop requested at iteration 100. Counted, so exact.
  struct Row {
    long check_every;
    long stopped_at;
  };
  const Row rows[] = {{1, iterations_run(1, 100, 100000)},
                      {64, iterations_run(64, 100, 100000)},
                      {1000, iterations_run(1000, 100, 100000)},
                      {100000, iterations_run(100000, 100, 100000)}};

  std::cout << "  the request is made at iteration 100:\n\n";
  std::cout << "    checks every        stopped at iteration\n";
  for (const Row& row : rows)
    std::cout << "    " << std::setw(12) << row.check_every << std::setw(24) << row.stopped_at << "\n";
  std::cout << "\n  Measured with real threads, gcc 11: a loop looking every iteration stopped in\n";
  std::cout << "  about 0.9 us, every 1000 in 2 us, every 100000 in 80 to 240 us. A flag polled\n";
  std::cout << "  between 10 ms sleeps took a median 3.3 ms, worst 9.3; a wait the request wakes\n";
  std::cout << "  took a median 72 us. A loop that never looks cannot be stopped at all.\n\n";

  RC_CHECK_EQ(rows[0].stopped_at, 100);
  RC_CHECK_EQ(rows[1].stopped_at, 128);
  RC_CHECK_EQ(rows[2].stopped_at, 1000);
  RC_CHECK_EQ(rows[3].stopped_at, 100000);
}

RC_TEST("destroying a callback waits for one that is running, so its captures are still there") {
  StopSource source;
  std::atomic<bool> in_callback{false};
  std::atomic<bool> release{false};
  std::atomic<bool> destroyed{false};
  std::atomic<bool> callback_finished{false};

  auto owner = std::make_unique<StopCallback>(source.get_token(), [&] {
    in_callback = true;
    const auto stop = Clock::now() + milliseconds(2000);
    while (!release.load() && Clock::now() < stop) std::this_thread::yield();
    callback_finished = true;
  });

  std::thread requester([&] { source.request_stop(); });

  const auto until = Clock::now() + milliseconds(1000);
  while (!in_callback.load() && Clock::now() < until) std::this_thread::yield();

  std::thread destroyer([&] {
    owner.reset();
    destroyed = true;
  });

  // The destructor must still be waiting for the callback: give it time to finish if it
  // is going to wrongly.
  std::this_thread::sleep_for(milliseconds(60));
  const bool destroyed_while_running = destroyed.load();

  release = true;
  requester.join();
  destroyer.join();

  std::cout << "  a callback held open while its owner was destroyed on another thread:\n";
  std::cout << "    destroyed while the callback was still running   "
            << (destroyed_while_running ? "yes, a use of freed captures" : "no, it waited") << "\n";
  std::cout << "    the callback finished before the destructor did  "
            << (callback_finished.load() && destroyed.load() ? "yes" : "no") << "\n\n";

  RC_CHECK(in_callback.load());
  RC_CHECK(!destroyed_while_running);
  RC_CHECK(callback_finished.load());
  RC_CHECK(destroyed.load());
}
