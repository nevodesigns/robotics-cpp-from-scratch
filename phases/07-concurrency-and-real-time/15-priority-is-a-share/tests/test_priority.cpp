#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// Low takes the lock and is slow with it, High wants it, Medium wants only the CPU.
std::vector<Task> classic() {
  std::vector<Task> tasks;
  tasks.push_back({'L', 1, 0, {{Op::kCompute, 1}, {Op::kLock, 0}, {Op::kCompute, 4}, {Op::kUnlock, 0}, {Op::kCompute, 1}}});
  tasks.push_back({'H', 3, 2, {{Op::kCompute, 1}, {Op::kLock, 0}, {Op::kCompute, 1}, {Op::kUnlock, 0}}});
  tasks.push_back({'M', 2, 3, {{Op::kCompute, 10}}});
  return tasks;
}

// High waits for Medium, Medium waits for Low, and a busy task N sits between Medium and High.
std::vector<Task> chain() {
  std::vector<Task> tasks;
  tasks.push_back({'L', 1, 0, {{Op::kLock, 0}, {Op::kCompute, 4}, {Op::kUnlock, 0}}});
  tasks.push_back({'M', 2, 1, {{Op::kLock, 1}, {Op::kLock, 0}, {Op::kCompute, 1}, {Op::kUnlock, 0}, {Op::kUnlock, 1}}});
  tasks.push_back({'H', 4, 2, {{Op::kLock, 1}, {Op::kCompute, 1}, {Op::kUnlock, 1}}});
  tasks.push_back({'N', 3, 2, {{Op::kCompute, 10}}});
  return tasks;
}

}  // namespace

RC_TEST("without inheritance the high priority task waits for the medium one, by way of the low one") {
  const Result result = simulate(classic(), 1, false);

  const int high_woke = 2;
  const int high_response = result.finished[1] - high_woke + 1;
  std::cout << "  Low (1) holds the lock, High (3) wants it, Medium (2) only wants the CPU:\n\n";
  std::cout << "    ticks     " << "0123456789012345678\n";
  std::cout << "    who ran   " << result.trace << "\n\n";
  std::cout << "    High finished at tick " << result.finished[1] << ", " << high_response
            << " ticks after it woke. Medium finished at " << result.finished[2] << ", before it.\n\n";

  RC_CHECK_EQ(result.trace, std::string("LLHMMMMMMMMMMLLLHL"));
  RC_CHECK_EQ(result.finished[1], 16);
  RC_CHECK_EQ(high_response, 15);

  // The inversion itself: a task that outranks nothing in the picture finishes before the highest.
  RC_CHECK(result.finished[2] < result.finished[1]);
}

RC_TEST("with inheritance the holder runs at the priority of whoever waits for it") {
  const Result result = simulate(classic(), 1, true);
  const Result before = simulate(classic(), 1, false);

  const int high_response = result.finished[1] - 2 + 1;
  std::cout << "  the same three tasks, the lock holder inheriting the priority of its waiter:\n\n";
  std::cout << "    who ran   " << result.trace << "\n\n";
  std::cout << "    High finished at tick " << result.finished[1] << ", " << high_response
            << " ticks after it woke, against " << before.finished[1] - 2 + 1 << ".\n";
  std::cout << "    Medium finished at " << result.finished[2] << ", against " << before.finished[2]
            << ": it pays " << result.finished[2] - before.finished[2] << " ticks for it.\n\n";

  RC_CHECK_EQ(result.trace, std::string("LLHLLLHMMMMMMMMMML"));
  RC_CHECK_EQ(result.finished[1], 6);
  RC_CHECK_EQ(high_response, 5);
  RC_CHECK_EQ(result.finished[2], 16);

  // High now finishes before Medium, as its priority says it should.
  RC_CHECK(result.finished[1] < result.finished[2]);
}

RC_TEST("inheritance has to be transitive, because the waiter may itself be waiting") {
  const Result none = simulate(chain(), 2, false);
  const Result chained = simulate(chain(), 2, true);

  std::cout << "  High (4) waits for Medium (2), who waits for Low (1); a busy task N (3) sits between:\n\n";
  std::cout << "                           trace                    High done   N done\n";
  std::cout << "    no inheritance         " << std::left << std::setw(25) << none.trace << std::right
            << std::setw(8) << none.finished[2] << std::setw(10) << none.finished[3] << "\n";
  std::cout << "    through the chain      " << std::left << std::setw(25) << chained.trace << std::right
            << std::setw(8) << chained.finished[2] << std::setw(10) << chained.finished[3] << "\n\n";
  std::cout << "  Inheriting only one step gives Low Medium's priority, 2, which N (3) still beats.\n";
  std::cout << "  Low has to inherit High's, 4, through Medium.\n\n";

  RC_CHECK_EQ(none.trace, std::string("LLNNNNNNNNNNLLMH"));
  RC_CHECK_EQ(none.finished[2], 15);
  RC_CHECK_EQ(none.finished[3], 11);

  RC_CHECK_EQ(chained.trace, std::string("LLLLMHNNNNNNNNNN"));
  RC_CHECK_EQ(chained.finished[2], 5);
  RC_CHECK_EQ(chained.finished[3], 15);
  RC_CHECK(chained.finished[2] < chained.finished[3]);
}

RC_TEST("the effective priority is the task's own unless someone is waiting on a lock it holds") {
  // Hand built state: L holds lock 0 and H is blocked on it.
  std::vector<Task> tasks;
  tasks.push_back({'L', 1, 0, {}});
  tasks.push_back({'H', 5, 0, {}});
  tasks.push_back({'M', 3, 0, {}});
  tasks[1].blocked_on = 0;
  const std::vector<int> holder = {0};

  RC_CHECK_EQ(effective_priority(tasks, holder, 0, true), 5);
  RC_CHECK_EQ(effective_priority(tasks, holder, 0, false), 1);
  RC_CHECK_EQ(effective_priority(tasks, holder, 1, true), 5);
  RC_CHECK_EQ(effective_priority(tasks, holder, 2, true), 3);

  // A waiter that is done no longer counts.
  tasks[1].done = true;
  RC_CHECK_EQ(effective_priority(tasks, holder, 0, true), 1);
}

RC_TEST("a priority is a share: the arithmetic predicted what the kernel gave") {
  const double even = cpu_share(0, 0);
  const double ten = cpu_share(0, 10);
  const double nineteen = cpu_share(0, 19);

  std::cout << "  nice weights: 0 is " << weight_of(0) << ", 10 is " << weight_of(10) << ", 19 is " << weight_of(19)
            << "\n\n";
  std::cout << "                         predicted      measured, two processes on one core\n";
  std::cout << "    nice 0 against 0     " << std::fixed << std::setprecision(1) << 100 * even
            << " percent      748 and 751 million iterations\n";
  std::cout << "    nice 0 against 10    " << 100 * ten << " percent      1330 and 145: 90.2 percent\n";
  std::cout << "    nice 0 against 19    " << 100 * nineteen << " percent      1448 and 24: 98.4 percent\n\n";
  std::cout << "  Unprivileged, asking for SCHED_FIFO or a nice below 0 was refused: Operation not permitted.\n\n";

  RC_CHECK_NEAR(even, 0.5, 1e-9);
  RC_CHECK_NEAR(ten, 1024.0 / 1134.0, 1e-9);
  RC_CHECK_NEAR(nineteen, 1024.0 / 1039.0, 1e-9);

  // Close to what was measured, within a percentage point.
  RC_CHECK_NEAR(ten, 0.902, 0.01);
  RC_CHECK_NEAR(nineteen, 0.984, 0.01);

  // The two shares add to one, and a lower priority gets less.
  RC_CHECK_NEAR(cpu_share(0, 10) + cpu_share(10, 0), 1.0, 1e-9);
  RC_CHECK(cpu_share(-5, 0) > cpu_share(0, 0));
  RC_CHECK(cpu_share(19, 0) < 0.02);

  // Values outside the range are held to it.
  RC_CHECK_NEAR(cpu_share(100, 0), cpu_share(19, 0), 1e-9);
  RC_CHECK_NEAR(cpu_share(-100, 0), cpu_share(-20, 0), 1e-9);
}
