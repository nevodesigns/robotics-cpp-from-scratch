#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>
#include <utility>

#include "solution.hpp"

namespace {

// The same filter written the usual way, with its data in the class, in two versions of its header.
class OpenV1 {
 public:
  double alpha = 0.5;
  double value = 0.0;
  int count = 0;
};

class OpenV2 {
 public:
  double alpha = 0.5;
  double value = 0.0;
  int count = 0;
  double history[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};     // the new field a later version adds
};

// The pointer version of the same two: an inside that grows and an outside that does not.
struct HiddenV1;
struct HiddenV2;
class BehindV1 {
 public:
  std::unique_ptr<HiddenV1> impl;
};
class BehindV2 {
 public:
  std::unique_ptr<HiddenV2> impl;
};

}  // namespace

RC_TEST("the first reading is the start, and then the value moves towards each new one") {
  Smoother s(0.5);
  RC_CHECK_EQ(s.count(), 0);
  RC_CHECK_NEAR(s.update(10.0), 10.0, 1e-12);
  RC_CHECK_NEAR(s.update(20.0), 15.0, 1e-12);
  RC_CHECK_NEAR(s.update(20.0), 17.5, 1e-12);
  RC_CHECK_EQ(s.count(), 3);

  Smoother slow(0.1);
  slow.update(0.0);
  RC_CHECK_NEAR(slow.update(10.0), 1.0, 1e-12);
  RC_CHECK_NEAR(slow.update(10.0), 1.9, 1e-12);
}

RC_TEST("a Smoother is as big as one pointer, however much is inside it") {
  std::cout << "  sizeof(Smoother) " << sizeof(Smoother) << ", sizeof(void*) " << sizeof(void*) << "\n";
  RC_CHECK_EQ(sizeof(Smoother), sizeof(std::unique_ptr<int>));
  RC_CHECK_EQ(sizeof(Smoother), sizeof(void*));

  // And it still does its job.
  Smoother s(0.5);
  RC_CHECK_NEAR(s.update(4.0), 4.0, 1e-12);
}

RC_TEST("adding a field changes the size of an open class and not of one behind a pointer") {
  std::cout << "  data in the class: version 1 is " << sizeof(OpenV1) << " bytes, version 2 is " << sizeof(OpenV2)
            << "\n  data behind a pointer: version 1 is " << sizeof(BehindV1) << " bytes, version 2 is "
            << sizeof(BehindV2) << "\n\n";
  RC_CHECK(sizeof(OpenV2) > sizeof(OpenV1));
  RC_CHECK_EQ(sizeof(BehindV1), sizeof(BehindV2));

  // The difference is what breaks code built against the old header: it reserves sizeof(OpenV1) bytes for
  // an object that now needs sizeof(OpenV2).
  RC_CHECK(sizeof(OpenV2) - sizeof(OpenV1) >= 80);

  Smoother s(0.5);
  s.update(1.0);
  RC_CHECK_EQ(s.count(), 1);
}

RC_TEST("a copy has its own state, assignment copies it, and a move hands it on") {
  Smoother a(0.5);
  a.update(10.0);
  a.update(20.0);              // 15

  Smoother b(a);               // a deep copy
  RC_CHECK_NEAR(b.update(20.0), 17.5, 1e-12);
  RC_CHECK_NEAR(a.update(0.0), 7.5, 1e-12);        // a was not moved by b's reading
  RC_CHECK_EQ(a.count(), 3);
  RC_CHECK_EQ(b.count(), 3);

  Smoother c(0.9);
  c = a;                       // copy assignment
  RC_CHECK_NEAR(c.update(7.5), 7.5, 1e-12);       // takes a's alpha of 0.5 and its value of 7.5: no movement
  RC_CHECK_EQ(c.count(), 4);

  Smoother& same = c;
  c = same;                    // assigning to itself changes nothing
  RC_CHECK_EQ(c.count(), 4);

  Smoother d(std::move(a));    // moved: d holds what a held
  RC_CHECK_EQ(d.count(), 3);
  RC_CHECK_NEAR(d.update(7.5), 7.5, 1e-12);
}

RC_TEST("moving a Smoother into another hands its state on") {
  Smoother original(0.5);
  original.update(4.0);
  Smoother moved(std::move(original));
  Smoother holder(0.1);
  holder = std::move(moved);
  RC_CHECK_EQ(holder.count(), 1);
  RC_CHECK_NEAR(holder.update(8.0), 6.0, 1e-12);
}
