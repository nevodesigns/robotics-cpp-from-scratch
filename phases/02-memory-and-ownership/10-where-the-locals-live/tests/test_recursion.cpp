#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iostream>
#include <vector>

#include "solution.hpp"

// File scope, per audit rule L022.
int global_value = 1;
static int file_scope_value = 2;

namespace {

// Walks down a given number of levels, taking the address of a local at each
// one, so the suite can see which way the stack goes without going far.
void note_local_addresses(int levels, std::vector<const unsigned char*>& seen) {
  unsigned char local = 0;
  seen.push_back(&local);
  if (levels > 0) note_local_addresses(levels - 1, seen);
}

}  // namespace

RC_TEST("a local is nowhere near the heap, and deeper means lower") {
  unsigned char here = 0;
  int* heap = new int(3);

  std::cout << "  a local in this test   " << static_cast<const void*>(&here) << "\n";
  std::cout << "  a heap allocation      " << static_cast<const void*>(heap) << "\n";
  std::cout << "  a global               " << static_cast<const void*>(&global_value) << "\n";
  std::cout << "  a static at file scope " << static_cast<const void*>(&file_scope_value) << "\n\n";

  // They are different regions. Asserting an ordering between the stack and
  // the heap would be asserting a property of one operating system, so the
  // suite only asserts that they are not the same place.
  RC_CHECK(static_cast<const void*>(&here) != static_cast<const void*>(heap));
  delete heap;

  // The direction, which is a property of the processor and is the same on
  // every toolchain this curriculum claims: deeper calls get lower addresses.
  std::vector<const unsigned char*> seen;
  note_local_addresses(4, seen);
  RC_CHECK(seen.size() == 5u);

  int downwards = 0;
  for (std::size_t i = 1; i < seen.size(); ++i)
    if (seen[i] < seen[i - 1]) ++downwards;
  RC_CHECK(downwards == 4);

  const long frame = seen[0] - seen[1];
  std::cout << "  five locals, one per call, each deeper than the last:\n";
  std::cout << "    all " << downwards << " steps went downwards\n";
  std::cout << "    one frame of this function is " << frame << " bytes\n\n";
  // The frame size is not a curiosity: it is the divisor. A measured frame
  // and a known stack limit give the depth you can afford, which is what
  // ties this test to the work the lesson asks for.
  RC_CHECK(frame > 0);
  RC_CHECK(depth_that_fits(8L * 1024 * 1024, static_cast<int>(frame)) > 1000);

  std::cout << "  measured standalone with a smaller function: 48 bytes a\n";
  std::cout << "  frame, and a recursion with 64 byte frames reached 130860\n";
  std::cout << "  levels before signal 11, which the shell reports as 139.\n";
  std::cout << "  8 MiB over 130860 is 64 bytes, so the arithmetic holds.\n\n";
}

RC_TEST("the guard refuses the level that would go too deep") {
  DepthLimit limit(3);
  RC_CHECK(limit.max_depth() == 3);
  RC_CHECK(limit.depth() == 0);
  RC_CHECK(!limit.exceeded());

  int reached = 0;
  bool refused = false;

  // A recursion that asks permission. Written as a lambda taking itself so
  // the test needs no helper, which keeps the shape visible.
  struct Walker {
    DepthLimit& limit;
    int& reached;
    bool& refused;
    void go() {
      const DepthGuard guard(limit);
      if (!guard.ok()) { refused = true; return; }
      if (limit.depth() > reached) reached = limit.depth();
      go();
    }
  };
  Walker walker{limit, reached, refused};
  walker.go();

  RC_CHECK(reached == 3);
  RC_CHECK(refused);
  RC_CHECK(limit.exceeded());
  RC_CHECK(limit.high_water() == 3);

  // And the counter is back where it started, because every guard that
  // counted a level gave it back on the way out. A depth counter that leaked
  // would report a limit that was never reached.
  RC_CHECK(limit.depth() == 0);

  std::cout << "  a limit of 3: reached " << reached << ", refused the fourth,\n";
  std::cout << "  high water " << limit.high_water() << ", depth back to "
            << limit.depth() << " afterwards\n\n";
  std::cout << "  the decrement is a destructor and not a line at the bottom\n";
  std::cout << "  of the function, for lesson 02-03's reason: an early return\n";
  std::cout << "  skips a line and skips no destructor.\n\n";
}

RC_TEST("a refused level is reported, not thrown, and the walk still returns") {
  DepthLimit limit(8);

  // A tree walk that gives a partial answer and says so, which is what a
  // planner handed a deeper map than it expected should do.
  struct Counter {
    DepthLimit& limit;
    int visited = 0;
    void visit(int children, int depth_remaining) {
      const DepthGuard guard(limit);
      if (!guard.ok()) return;
      ++visited;
      if (depth_remaining == 0) return;
      for (int i = 0; i < children; ++i) visit(children, depth_remaining - 1);
    }
  };

  Counter shallow{limit};
  shallow.visit(2, 3);
  RC_CHECK(shallow.visited == 15);      // a full binary tree of depth 3
  RC_CHECK(!limit.exceeded());
  RC_CHECK(limit.depth() == 0);

  limit.reset();
  RC_CHECK(limit.high_water() == 0);

  Counter deep{limit};
  deep.visit(2, 20);                    // far deeper than the limit of 8
  RC_CHECK(limit.exceeded());
  RC_CHECK(limit.depth() == 0);
  RC_CHECK(limit.high_water() == 8);
  RC_CHECK(deep.visited > 0);
  RC_CHECK(deep.visited < 2000000);

  std::cout << "  a tree of depth 3 under a limit of 8: " << shallow.visited
            << " nodes, limit never reached\n";
  std::cout << "  a tree of depth 20 under the same limit: " << deep.visited
            << " nodes visited, cut off at depth " << limit.high_water() << "\n\n";
  std::cout << "  the second answer is partial and says so. That is the whole\n";
  std::cout << "  point: without the guard it is not a partial answer, it is\n";
  std::cout << "  signal 11 and no answer at all.\n\n";
}

RC_TEST("the depth that fits is arithmetic, with room left over") {
  // The measured case: 8 MiB of stack, 64 byte frames.
  const int fits = depth_that_fits(8L * 1024 * 1024, 64);
  RC_CHECK(fits > 0);
  RC_CHECK(fits < 131072);              // less than the measured crash depth
  RC_CHECK(fits > 80000);               // and not uselessly conservative

  // A fatter frame buys far fewer levels, which is the number people forget.
  const int fat = depth_that_fits(8L * 1024 * 1024, 1024);
  RC_CHECK(fat > 0);
  RC_CHECK(fat < fits / 8);

  // Windows defaults to 1 MiB, so the same code gets an eighth of the room.
  const int windows = depth_that_fits(1L * 1024 * 1024, 64);
  RC_CHECK(windows > 0);
  RC_CHECK(windows < fits / 4);

  // Nonsense in, zero out, rather than a division by zero.
  RC_CHECK(depth_that_fits(0, 64) == 0);
  RC_CHECK(depth_that_fits(8L * 1024 * 1024, 0) == 0);
  RC_CHECK(depth_that_fits(-1, 64) == 0);

  std::cout << "  stack      frame   depth that fits\n";
  std::cout << "  8 MiB      64 B    " << fits << "\n";
  std::cout << "  8 MiB    1024 B    " << fat << "\n";
  std::cout << "  1 MiB      64 B    " << windows << "   (the Windows default)\n\n";
  std::cout << "  two thirds of the arithmetic rather than all of it, because\n";
  std::cout << "  the frame you measured is one function and the real chain\n";
  std::cout << "  includes everything it calls, library code included.\n\n";
}
