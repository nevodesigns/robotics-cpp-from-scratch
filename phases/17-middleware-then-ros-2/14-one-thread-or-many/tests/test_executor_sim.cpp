#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const int kControl = 0;
const int kPlanner = 1;

// A 100 Hz control callback that takes 1 tick of 1 ms, every 10 ticks for a second, and a planner callback that becomes ready at tick 5.
std::vector<Task> control_and_planner(int planner_ticks) {
  std::vector<Task> tasks;
  for (int i = 0; i < 100; ++i) {
    Task t;
    t.id = i;
    t.ready_at = i * 10;
    t.duration = 1;
    t.group = kControl;
    tasks.push_back(t);
  }
  Task p;
  p.id = 1000;
  p.ready_at = 5;
  p.duration = planner_ticks;
  p.group = kPlanner;
  tasks.push_back(p);
  return tasks;
}

}  // namespace

RC_TEST("on one thread callbacks run one after another, in the order they became ready") {
  std::vector<Task> tasks = {{1, 0, 5, 0}, {2, 1, 3, 1}, {3, 2, 2, 2}};
  const std::vector<Run> r = simulate(tasks, {true, true, true}, 1, 100);
  std::cout << "  three callbacks ready at 0, 1 and 2, taking 5, 3 and 2 ticks, one thread: started at " << r[0].started << ", " << r[1].started << ", " << r[2].started
            << "; waited " << r[0].waited << ", " << r[1].waited << ", " << r[2].waited << "\n\n";
  RC_CHECK(r.size() == 3);
  if (r.size() != 3) return;
  RC_CHECK(r[0].started == 0 && r[0].finished == 5);
  RC_CHECK(r[1].started == 5 && r[1].finished == 8);
  RC_CHECK(r[2].started == 8 && r[2].finished == 10);
  RC_CHECK(r[1].waited == 4);
  RC_CHECK(r[2].waited == 6);
}

RC_TEST("a long callback holds up a short one on a single thread, by almost the length of the long one") {
  std::cout << "  control every 10 ticks taking 1, planner ready at 5, one thread\n  planner takes   worst wait of a control callback\n";
  int at_50 = 0;
  int at_5 = 0;
  for (int ticks : {5, 20, 50, 200}) {
    const std::vector<Task> tasks = control_and_planner(ticks);
    const std::vector<Run> r = simulate(tasks, {true, true}, 1, 5000);
    const int w = worst_wait(tasks, r, kControl, 5000);
    std::cout << "  " << std::setw(13) << ticks << "   " << std::setw(30) << w << "\n";
    if (ticks == 5) at_5 = w;
    if (ticks == 50) at_50 = w;
  }
  std::cout << "\n";
  RC_CHECK(at_5 == 0);                                   // the planner finishes before the next control tick is due
  RC_CHECK(at_50 >= 40);
  RC_CHECK(at_50 <= 50);
}

RC_TEST("two threads and separate groups let the control callback run on time while the planner runs") {
  const std::vector<Task> tasks = control_and_planner(50);
  const std::vector<Run> one = simulate(tasks, {true, true}, 1, 5000);
  const std::vector<Run> two = simulate(tasks, {true, true}, 2, 5000);
  const int w1 = worst_wait(tasks, one, kControl, 5000);
  const int w2 = worst_wait(tasks, two, kControl, 5000);
  std::cout << "  planner of 50 ticks: one thread, control waited up to " << w1 << " ticks; two threads in separate groups, " << w2 << "\n\n";
  RC_CHECK(w1 >= 40);
  RC_CHECK(w2 == 0);
  RC_CHECK(worst_wait(tasks, two, kPlanner, 5000) == 0);
}

RC_TEST("two threads do nothing for callbacks that share one exclusive group, and the extra thread sits idle") {
  std::vector<Task> tasks = control_and_planner(50);
  for (Task& t : tasks) t.group = 0;                    // everything in one group
  const std::vector<Run> exclusive = simulate(tasks, {true}, 2, 5000);
  const std::vector<Run> reentrant = simulate(tasks, {false}, 2, 5000);
  const int w_ex = worst_wait(tasks, exclusive, 0, 5000);
  const int w_re = worst_wait(tasks, reentrant, 0, 5000);
  std::cout << "  everything in one group, two threads: exclusive group, worst wait " << w_ex << "; reentrant group, worst wait " << w_re << "\n\n";
  RC_CHECK(w_ex >= 40);
  RC_CHECK(w_re == 0);
}

RC_TEST("more callbacks than threads wait for a thread even when their group allows them to overlap") {
  std::vector<Task> tasks = {{1, 0, 10, 0}, {2, 0, 10, 0}, {3, 0, 10, 0}, {4, 0, 10, 0}};
  const std::vector<Run> r = simulate(tasks, {false}, 2, 100);
  std::cout << "  four callbacks of 10 ticks ready together, a reentrant group, two threads: started at " << r[0].started << " " << r[1].started << " " << r[2].started
            << " " << r[3].started << "\n\n";
  RC_CHECK(r.size() == 4);
  if (r.size() != 4) return;
  RC_CHECK(r[0].started == 0 && r[1].started == 0);
  RC_CHECK(r[2].started == 10 && r[3].started == 10);
  const std::vector<Run> three = simulate(tasks, {false}, 3, 100);
  RC_CHECK(three.size() == 4);
  if (three.size() != 4) return;
  RC_CHECK(three[2].started == 0);
  RC_CHECK(three[3].started == 10);
}

RC_TEST("a callback blocked by its group does not hold up the others behind it, and one that never gets to run is reported") {
  // Task 1 holds group 0 for 100 ticks. Task 2 is in group 0 too and must wait. Task 3, in group 1, is ready after task 2 and runs at once.
  std::vector<Task> tasks = {{1, 0, 100, 0}, {2, 1, 5, 0}, {3, 2, 5, 1}};
  const std::vector<Run> r = simulate(tasks, {true, true}, 2, 1000);
  RC_CHECK(r.size() == 3);
  if (r.size() != 3) return;
  std::cout << "  group 0 held by a 100 tick callback: the next in group 0 started at " << r[1].started << ", one in group 1 ready after it started at " << r[2].started << "\n";
  RC_CHECK(r[1].started == 100);
  RC_CHECK(r[2].started == 2);
  const std::vector<Run> cut = simulate(tasks, {true, true}, 2, 50);        // the limit comes first
  RC_CHECK(cut.size() == 3);
  if (cut.size() != 3) return;
  RC_CHECK(cut[1].started == -1);
  RC_CHECK(worst_wait(tasks, cut, 0, 50) == 49);
  std::cout << "  stopped at tick 50, the blocked one never ran and counts as having waited " << worst_wait(tasks, cut, 0, 50) << "\n\n";
}
