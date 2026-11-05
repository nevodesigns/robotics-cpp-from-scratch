#include <rc/test/rc_test.hpp>

#include <atomic>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

struct Cell {
  const char* point;
  bool fired;
  int stranded;
};

// Runs one submit with shutdown() injected at the named point, and reports whether
// the hook ever fired and what was left stranded.
Cell run_with_shutdown_at(bool fixed, const char* point) {
  Intake intake(fixed);
  bool fired = false;
  {
    const RaceHook hook(point, [&] {
      if (!fired) {
        fired = true;
        intake.shutdown();
      }
    });
    intake.submit(1);
  }
  return Cell{point, fired, intake.stranded()};
}

}  // namespace

RC_TEST("with nothing installed a race point does nothing, and again once a hook is gone") {
  int calls = 0;
  race_point("never.installed");
  RC_CHECK_EQ(calls, 0);

  // A hook for another name does not run here either.
  {
    const RaceHook elsewhere("somewhere.else", [&] { ++calls; });
    race_point("never.installed");
  }
  RC_CHECK_EQ(calls, 0);

  // An all-zero stub passes everything above, so a hook has to work too: it runs while
  // installed, and the point goes back to doing nothing the moment it is removed.
  {
    const RaceHook here("never.installed", [&] { ++calls; });
    race_point("never.installed");
    RC_CHECK_EQ(calls, 1);
  }
  race_point("never.installed");
  RC_CHECK_EQ(calls, 1);
}

RC_TEST("a hook runs at its own point, every time it is reached, and stops when it goes out of scope") {
  int at_a = 0;
  int at_b = 0;

  {
    const RaceHook first("a", [&] { ++at_a; });
    const RaceHook second("b", [&] { ++at_b; });

    race_point("a");
    race_point("a");
    race_point("b");
    race_point("c");
    RC_CHECK_EQ(at_a, 2);
    RC_CHECK_EQ(at_b, 1);
  }

  race_point("a");
  race_point("b");
  RC_CHECK_EQ(at_a, 2);
  RC_CHECK_EQ(at_b, 1);

  // Two hooks at one name do not remove each other: each goes by its own id.
  int one = 0;
  int two = 0;
  {
    const RaceHook outer("same", [&] { ++one; });
    {
      const RaceHook inner("same", [&] { ++two; });
    }
    race_point("same");
    RC_CHECK_EQ(one, 1);
    RC_CHECK_EQ(two, 0);
  }
}

RC_TEST("a hook at the gap puts shutdown exactly where the bug needs it, on one thread") {
  Intake buggy(false);
  bool fired = false;
  bool accepted = false;
  {
    const RaceHook hook("submit.gap", [&] {
      fired = true;
      buggy.shutdown();
    });
    accepted = buggy.submit(7);
  }

  std::cout << "  buggy intake, shutdown() run at submit.gap on the same thread:\n";
  std::cout << "    the hook fired        " << (fired ? "yes" : "no") << "\n";
  std::cout << "    submit returned       " << (accepted ? "true, the task was accepted" : "false") << "\n";
  std::cout << "    tasks stranded        " << buggy.stranded() << "\n\n";

  RC_CHECK(fired);
  RC_CHECK(accepted);
  RC_CHECK_EQ(buggy.stranded(), 1);
}

RC_TEST("trying every point: the buggy version fails at the gap and nowhere else, the fixed one nowhere") {
  const char* points[] = {"submit.before", "submit.gap", "submit.after"};
  std::vector<Cell> buggy;
  std::vector<Cell> fixed;
  for (const char* point : points) {
    buggy.push_back(run_with_shutdown_at(false, point));
    fixed.push_back(run_with_shutdown_at(true, point));
  }

  std::cout << "  shutdown() injected at each point of one submit:\n\n";
  std::cout << "                   buggy intake                 fixed intake\n";
  std::cout << "    point          hook fired   stranded       hook fired   stranded\n";
  for (std::size_t i = 0; i < buggy.size(); ++i) {
    std::cout << "    " << std::left << std::setw(14) << points[i] << std::right << std::setw(8)
              << (buggy[i].fired ? "yes" : "no") << std::setw(12) << buggy[i].stranded << std::setw(18)
              << (fixed[i].fired ? "yes" : "no") << std::setw(11) << fixed[i].stranded << "\n";
  }
  std::cout << "\n  The fixed version has no gap to hook, so that row never fires. That is the repair.\n\n";

  // Buggy: shutdown before submit refuses it, in the gap strands it, after submit
  // finds it already queued.
  RC_CHECK(buggy[0].fired);
  RC_CHECK_EQ(buggy[0].stranded, 0);
  RC_CHECK(buggy[1].fired);
  RC_CHECK_EQ(buggy[1].stranded, 1);
  RC_CHECK(buggy[2].fired);
  RC_CHECK_EQ(buggy[2].stranded, 0);

  // Fixed: nothing is ever stranded, and the gap does not exist.
  for (const Cell& cell : fixed) RC_CHECK_EQ(cell.stranded, 0);
  RC_CHECK(fixed[0].fired);
  RC_CHECK(!fixed[1].fired);
  RC_CHECK(fixed[2].fired);
}

RC_TEST("hooks are safe to reach from several threads at once") {
  std::atomic<int> hits{0};
  const RaceHook hook("busy.point", [&] { ++hits; });

  std::vector<std::thread> threads;
  for (int t = 0; t < 4; ++t) {
    threads.emplace_back([] {
      for (int i = 0; i < 1000; ++i) race_point("busy.point");
    });
  }
  for (std::thread& thread : threads) thread.join();

  std::cout << "  4 threads reached one point 1000 times each: the hook ran " << hits.load() << " times\n\n";
  RC_CHECK_EQ(hits.load(), 4000);
}

RC_TEST("running it a lot finds the bug only sometimes, and forcing it finds it every time") {
  // Two real threads and no hooks, as an ordinary stress test would. The count of
  // trials that strand a task varies from run to run and is often zero, so it is
  // printed and not asserted. The fixed version must never strand one.
  const int kTrials = 300;
  int buggy_stranded = 0;
  int fixed_stranded = 0;

  for (int variant = 0; variant < 2; ++variant) {
    const bool fixed = variant == 1;
    for (int trial = 0; trial < kTrials; ++trial) {
      Intake intake(fixed);
      std::atomic<bool> go{false};
      std::thread submitter([&] {
        while (!go.load()) std::this_thread::yield();
        for (int i = 0; i < 4; ++i) intake.submit(i);
      });
      std::thread closer([&] {
        while (!go.load()) std::this_thread::yield();
        intake.shutdown();
      });
      go = true;
      submitter.join();
      closer.join();
      if (intake.stranded() != 0) (fixed ? fixed_stranded : buggy_stranded) += 1;
    }
  }

  // The same bug, forced: shutdown run at the gap, every time. This needs the
  // learner's hooks, which is why it is here beside the unforced count.
  int forced_found = 0;
  for (int trial = 0; trial < kTrials; ++trial)
    if (run_with_shutdown_at(false, "submit.gap").stranded == 1) ++forced_found;

  std::cout << "  " << kTrials << " trials each, two real threads, no hooks:\n";
  std::cout << "    buggy intake stranded a task in   " << buggy_stranded << " trials (varies, often 0)\n";
  std::cout << "    fixed intake stranded a task in   " << fixed_stranded << " trials\n";
  std::cout << "  the same " << kTrials << " trials with the interleaving forced by a hook:\n";
  std::cout << "    buggy intake stranded a task in   " << forced_found << " trials\n\n";
  std::cout << "  Measured outside the suite, 20000 trials at each of three delays, three runs: the\n";
  std::cout << "  buggy version stranded a task in 2 to 28 of 20000, about 0.07 percent, and the\n";
  std::cout << "  fixed version in 0 of 180000. A suite that runs it 100 times passes with the bug\n";
  std::cout << "  in about 93 percent of the time.\n\n";

  RC_CHECK_EQ(fixed_stranded, 0);

  // Forcing it finds the bug every time, where running it a lot finds it sometimes.
  RC_CHECK_EQ(forced_found, kTrials);
}
