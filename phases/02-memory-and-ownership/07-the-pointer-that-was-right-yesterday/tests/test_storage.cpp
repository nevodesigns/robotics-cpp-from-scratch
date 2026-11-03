#include <rc/test/rc_test.hpp>

#include <cstddef>
#include <deque>
#include <iomanip>
#include <iostream>
#include <list>
#include <vector>

#include "solution.hpp"

namespace {

// Nothing in this file reads through a pointer after the operation that
// invalidated it. Every claim is made by comparing storage identities and by
// reading the container, which is what lets the suite demonstrate the bug
// without committing it.

std::vector<double> three_readings() {
  std::vector<double> samples = {10.0, 20.0, 30.0};
  samples.shrink_to_fit();   // no slack, so the next push_back must move it
  return samples;
}

}  // namespace

RC_TEST("growing a vector moves its elements, and not on every push") {
  std::vector<double> samples;
  StorageId previous = storage_of(samples);

  int reallocations = 0;
  std::cout << "\n  " << std::left << std::setw(8) << "size" << std::setw(11) << "capacity"
            << std::setw(14) << "buffer moved" << "growth\n" << std::right;

  for (int i = 0; i < 20; ++i) {
    const bool predicted = would_reallocate(samples, 1);
    const std::size_t capacity_before = samples.capacity();

    samples.push_back(static_cast<double>(i));
    const StorageId now = storage_of(samples);
    const bool moved = !same_storage(previous, now);

    // The prediction is the artifact's whole job, and it is checked on every
    // single insertion rather than at the interesting ones.
    RC_CHECK(predicted == moved);

    if (moved) ++reallocations;
    if (i < 10 || moved) {
      std::cout << "  " << std::left << std::setw(8) << samples.size() << std::setw(11)
                << samples.capacity() << std::setw(14) << (moved ? "YES" : "no");
      if (moved && capacity_before > 0)
        std::cout << std::fixed << std::setprecision(2)
                  << (double(samples.capacity()) / double(capacity_before)) << "x";
      else
        std::cout << "-";
      std::cout << "\n" << std::right;
    }
    previous = now;
  }

  std::cout << "\n  " << reallocations << " reallocations for 20 insertions\n";
  std::cout << "  the growth factor above is your standard library's choice, not a\n";
  std::cout << "  rule, so it is printed rather than asserted\n\n";

  // It moves sometimes and not always, which is exactly why this reaches a
  // release: nineteen of twenty insertions are harmless.
  RC_CHECK(reallocations > 0);
  RC_CHECK(reallocations < 20);
  RC_CHECK(samples.size() == 20u);
}

RC_TEST("reserving first means nothing moves at all") {
  std::vector<double> samples;
  const std::size_t capacity = reserve_for(samples, 20);
  const StorageId start = storage_of(samples);

  int moves = 0;
  for (int i = 0; i < 20; ++i) {
    RC_CHECK(!would_reallocate(samples, 1));
    samples.push_back(static_cast<double>(i));
    if (!same_storage(start, storage_of(samples))) ++moves;
  }

  std::cout << "  after reserving for 20, the buffer moved " << moves << " times"
            << ", capacity " << samples.capacity() << "\n\n";

  RC_CHECK(capacity >= 20u);
  RC_CHECK(moves == 0);
  RC_CHECK(same_storage(start, storage_of(samples)));

  // And reserving again for room it already has changes nothing, so calling it
  // defensively is free rather than wasteful.
  const StorageId settled = storage_of(samples);
  RC_CHECK(reserve_for(samples, 0) == samples.capacity());
  RC_CHECK(same_storage(settled, storage_of(samples)));
}

RC_TEST("one insertion is enough, when there is no room left") {
  std::vector<double> samples = three_readings();
  const StorageId before = storage_of(samples);

  RC_CHECK(before.capacity == 3u);
  RC_CHECK(would_reallocate(samples, 1));

  samples.push_back(40.0);
  const StorageId after = storage_of(samples);

  std::cout << "  three readings with no slack, capacity " << before.capacity << "\n";
  std::cout << "  after one push_back, capacity " << after.capacity << ", buffer "
            << (same_storage(before, after) ? "unchanged" : "somewhere else") << "\n";
  std::cout << "  anything borrowed from the old buffer now refers to freed memory\n\n";

  RC_CHECK(!same_storage(before, after));
  RC_CHECK(after.capacity > before.capacity);

  // The values are all still there. The buffer holding them is not the one
  // anybody was given a pointer into.
  RC_CHECK(samples.size() == 4u);
  RC_CHECK_NEAR(samples[0], 10.0, 1e-12);
  RC_CHECK_NEAR(samples[3], 40.0, 1e-12);
}

RC_TEST("erase leaves the buffer alone and changes what each address holds") {
  std::vector<double> samples = {10.0, 20.0, 30.0, 40.0, 50.0};
  reserve_for(samples, 16);
  const StorageId before = storage_of(samples);

  RC_CHECK_NEAR(samples[2], 30.0, 1e-12);

  samples.erase(samples.begin() + 1);     // remove the 20

  const StorageId after = storage_of(samples);

  std::cout << std::defaultfloat;
  std::cout << "  before erasing index 1, index 2 held 30\n";
  std::cout << "  after erasing index 1,  index 2 holds  " << samples[2] << "\n";
  std::cout << "  and the buffer "
            << (same_storage(before, after) ? "did not move" : "moved") << "\n\n";

  // Nothing was reallocated. The storage is the same storage.
  RC_CHECK(same_storage(before, after));

  // And the same position now holds a different reading, because everything
  // after the erased element shifted down one.
  RC_CHECK_NEAR(samples[2], 40.0, 1e-12);
  RC_CHECK(samples.size() == 4u);

  // So a pointer saved to &samples[2] is still pointing at live memory inside
  // the same buffer, and it is pointing at the wrong reading. Nothing was
  // freed, so no sanitizer has anything to report, and the program carries on
  // with a plausible number. This is the worst of the three failures in this
  // lesson and the only one with no diagnostic at all.
  std::cout << "  nothing was freed, so nothing reports it. The pointer is live and\n";
  std::cout << "  wrong, which is worse than dangling and crashing.\n\n";
}

RC_TEST("which containers move their elements, and which promise not to") {
  std::cout << "  " << std::left << std::setw(14) << "container"
            << "first element after 100 insertions\n" << std::right;

  bool vector_moved = false;
  int vector_predicted = 0;
  {
    std::vector<double> c;
    c.push_back(1.0);
    const StorageId first = storage_of(c);
    for (int i = 0; i < 100; ++i) {
      if (would_reallocate(c, 1)) ++vector_predicted;
      c.push_back(static_cast<double>(i));
    }
    vector_moved = !same_storage(first, storage_of(c));
    std::cout << "  " << std::left << std::setw(14) << "vector"
              << (vector_moved ? "moved" : "unchanged") << ", predicted "
              << vector_predicted << " times\n" << std::right;
  }

  bool deque_moved = false;
  {
    std::deque<double> c;
    c.push_back(1.0);
    const double* first = &c[0];
    for (int i = 0; i < 100; ++i) c.push_back(static_cast<double>(i));
    deque_moved = (&c[0] != first);
    std::cout << "  " << std::left << std::setw(14) << "deque"
              << (deque_moved ? "moved" : "unchanged") << "\n" << std::right;
  }

  bool list_moved = false;
  {
    std::list<double> c;
    c.push_back(1.0);
    const double* first = &c.front();
    for (int i = 0; i < 100; ++i) c.push_back(static_cast<double>(i));
    list_moved = (&c.front() != first);
    std::cout << "  " << std::left << std::setw(14) << "list"
              << (list_moved ? "moved" : "unchanged") << "\n" << std::right;
  }
  std::cout << "\n";

  // The vector is the one that moves, and it is also the one you should
  // reach for first. The other two buy element stability with a pointer
  // chase per element, which phase 07 measures the cost of.
  RC_CHECK(vector_moved);
  RC_CHECK(!deque_moved);
  RC_CHECK(!list_moved);

  // And the artifact saw every one of the vector's moves coming.
  RC_CHECK(vector_predicted > 0);
  RC_CHECK(vector_predicted < 100);
}

RC_TEST("the prediction is exact across a long run of insertions") {
  // Every insertion, for a thousand of them, with the prediction checked
  // against what actually happened. Not a sample.
  std::vector<double> samples;
  StorageId previous = storage_of(samples);

  int predictions = 0;
  int wrong = 0;
  int moves = 0;
  for (int i = 0; i < 1000; ++i) {
    const bool predicted = would_reallocate(samples, 1);
    samples.push_back(static_cast<double>(i));
    const StorageId now = storage_of(samples);
    const bool moved = !same_storage(previous, now);

    ++predictions;
    if (predicted != moved) ++wrong;
    if (moved) ++moves;
    previous = now;
  }

  std::cout << "  " << predictions << " insertions, " << moves << " reallocations, "
            << wrong << " predictions wrong\n\n";

  RC_CHECK(wrong == 0);
  RC_CHECK(moves > 0);

  // And it predicts a bulk insertion as well as a single one, which is the
  // case a loop over a batch of readings runs into.
  std::vector<double> batch;
  RC_CHECK(would_reallocate(batch, 1));
  reserve_for(batch, 500);
  RC_CHECK(!would_reallocate(batch, 500));
  RC_CHECK(would_reallocate(batch, 5000));
}
