#include <rc/test/rc_test.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "solution.hpp"

namespace {

using std::chrono::milliseconds;

void nap() { std::this_thread::sleep_for(milliseconds(50)); }

// Starts a thread, then throws before anything could join it by hand. With a bare
// std::thread this function ends the process: the throw unwinds the stack, the
// thread object is destroyed still joinable, and std::terminate runs before any
// catch block can.
void start_then_throw(std::atomic<bool>& finished) {
  ScopedThread worker([&finished] {
    nap();
    finished = true;
  });
  throw std::runtime_error("something went wrong after the thread started");
}

}  // namespace

RC_TEST("a started thread is joinable, and joining, or being moved from, ends that") {
  // A bare std::thread, to show the states. It is joined, never destroyed joinable.
  std::thread bare;
  const bool default_state = bare.joinable();
  bare = std::thread([] {});
  const bool started = bare.joinable();
  std::thread taken = std::move(bare);
  const bool source_after_move = bare.joinable();
  const bool target_after_move = taken.joinable();
  taken.join();
  const bool after_join = taken.joinable();

  ScopedThread scoped([] {});
  const bool scoped_started = scoped.joinable();
  ScopedThread other = std::move(scoped);
  const bool scoped_source_after_move = scoped.joinable();
  other.join();
  const bool scoped_after_join = other.joinable();
  other.join();  // harmless the second time

  std::cout << "  joinable?             std::thread    ScopedThread\n";
  std::cout << "    just built, empty        " << default_state << "\n";
  std::cout << "    started                  " << started << "              " << scoped_started << "\n";
  std::cout << "    after being moved from   " << source_after_move << "              "
            << scoped_source_after_move << "\n";
  std::cout << "    the one it moved to      " << target_after_move << "\n";
  std::cout << "    after join               " << after_join << "              " << scoped_after_join
            << "\n\n";
  std::cout << "  A thread destroyed while joinable ends the process. Measured as separate\n";
  std::cout << "  programs on gcc 11, each with exit status 134, which is 128 plus SIGABRT:\n\n";
  std::cout << "    destroyed without join      terminate called without an active exception\n";
  std::cout << "    body throws                 terminate called after throwing an instance of\n";
  std::cout << "                                'std::runtime_error', what(): sensor lost\n";
  std::cout << "    throw before the join       terminate, and the catch block never ran\n";
  std::cout << "    detached, using a local     exit status 0, and 1 printed where 42 belonged\n\n";

  RC_CHECK(!default_state);
  RC_CHECK(started);
  RC_CHECK(!source_after_move);
  RC_CHECK(target_after_move);
  RC_CHECK(!after_join);

  RC_CHECK(scoped_started);
  RC_CHECK(!scoped_source_after_move);
  RC_CHECK(!scoped_after_join);
}

RC_TEST("the destructor joins, so leaving the scope waits for the thread") {
  std::atomic<bool> finished{false};
  {
    ScopedThread worker([&finished] {
      nap();
      finished = true;
    });
    RC_CHECK(worker.joinable());
  }
  RC_CHECK(finished.load());
}

RC_TEST("a throw after the thread started still joins it, and the catch still runs") {
  std::atomic<bool> finished{false};
  bool caught = false;
  try {
    start_then_throw(finished);
  } catch (const std::runtime_error&) {
    caught = true;
  }

  std::cout << "  start_then_throw: the exception reached the catch block " << (caught ? "yes" : "no")
            << ", and the thread had finished " << (finished.load() ? "yes" : "no") << "\n\n";
  RC_CHECK(caught);
  RC_CHECK(finished.load());
}

RC_TEST("a thread whose body throws is reported, not fatal") {
  ScopedThread failing([] {
    nap();
    throw std::runtime_error("sensor lost");
  });
  const bool failed = failing.failed();
  const std::string message = failing.error();

  ScopedThread fine([] { nap(); });
  const bool fine_failed = fine.failed();
  const std::string fine_message = fine.error();

  ScopedThread odd([] { throw 42; });
  const bool odd_failed = odd.failed();
  const std::string odd_message = odd.error();

  std::cout << "  how three threads ended:\n";
  std::cout << "    threw runtime_error(\"sensor lost\")   failed " << failed << ", \"" << message << "\"\n";
  std::cout << "    returned normally                    failed " << fine_failed << ", \"" << fine_message
            << "\"\n";
  std::cout << "    threw the int 42                     failed " << odd_failed << ", \"" << odd_message
            << "\"\n\n";

  RC_CHECK(failed);
  RC_CHECK_EQ(message, std::string("sensor lost"));
  RC_CHECK(!fine_failed);
  RC_CHECK_EQ(fine_message, std::string());
  RC_CHECK(odd_failed);
  RC_CHECK_EQ(odd_message, std::string("unknown exception"));
}

RC_TEST("assigning over a running thread joins the old one first") {
  std::atomic<bool> first_finished{false};
  std::atomic<bool> second_finished{false};

  ScopedThread worker([&first_finished] {
    nap();
    first_finished = true;
  });
  worker = ScopedThread([&second_finished] {
    nap();
    second_finished = true;
  });

  // The first thread was replaced, and it was joined on the way, not abandoned.
  RC_CHECK(first_finished.load());
  worker.join();
  RC_CHECK(second_finished.load());
}

RC_TEST("threads kept in a container are each joined when it is emptied") {
  std::atomic<int> done{0};
  std::vector<ScopedThread> pool;
  for (int i = 0; i < 8; ++i) {
    pool.emplace_back([&done] {
      std::this_thread::sleep_for(milliseconds(10));
      ++done;
    });
  }
  RC_CHECK_EQ(pool.size(), 8u);

  pool.clear();
  std::cout << "  eight threads started, container cleared: " << done.load() << " of 8 had finished\n\n";
  RC_CHECK_EQ(done.load(), 8);
}
