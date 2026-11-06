#include <rc/test/rc_test.hpp>
#include <rc/test/leak_check.hpp>

#include <iostream>
#include <memory>

#include "solution.hpp"

RC_TEST("a fresh pool lends sixty-four messages, then says it is empty") {
  MessagePool pool;
  RC_CHECK_EQ(pool.capacity(), 64);
  RC_CHECK_EQ(pool.in_use(), 0);

  Message* lent[64];
  for (int i = 0; i < 64; ++i) {
    lent[i] = pool.acquire();
    RC_REQUIRE(lent[i] != nullptr);
  }
  RC_CHECK_EQ(pool.in_use(), 64);
  RC_CHECK(pool.acquire() == nullptr);
  RC_CHECK(pool.acquire() == nullptr);

  // No two of them are the same message.
  bool distinct = true;
  for (int i = 0; i < 64; ++i) {
    for (int j = i + 1; j < 64; ++j) {
      if (lent[i] == lent[j]) distinct = false;
    }
  }
  RC_CHECK(distinct);
}

RC_TEST("a steady loop of acquire and release allocates nothing, where new allocates every time") {
  MessagePool pool;
  const int cycles = 100000;

  const std::size_t before_pool = rc::test::total_blocks().load();
  double sum = 0.0;
  int done = 0;
  for (int i = 0; i < cycles; ++i) {
    Message* m = pool.acquire();
    if (m == nullptr) break;
    m->value = i;
    sum += m->value;
    pool.release(m);
    ++done;
  }
  const std::size_t pool_allocations = rc::test::total_blocks().load() - before_pool;

  const std::size_t before_new = rc::test::total_blocks().load();
  for (int i = 0; i < cycles; ++i) {
    std::unique_ptr<Message> m(new Message);
    m->value = i;
    sum += m->value;
  }
  const std::size_t new_allocations = rc::test::total_blocks().load() - before_new;

  std::cout << "  " << cycles << " lend and take back cycles: the pool made " << pool_allocations
            << " heap allocations, new made " << new_allocations << "\n\n";
  RC_CHECK_EQ(done, cycles);
  RC_CHECK(sum > 0.0);
  RC_CHECK(pool.in_use() == 0);
  RC_CHECK_EQ(pool_allocations, static_cast<std::size_t>(0));
  RC_CHECK_EQ(new_allocations, static_cast<std::size_t>(cycles));
}

RC_TEST("a lent message starts clean, and the one last released is the next one lent") {
  MessagePool pool;
  Message* first = pool.acquire();
  RC_REQUIRE(first != nullptr);
  first->value = 42.0;
  first->id = 7;
  RC_CHECK(pool.release(first));

  Message* second = pool.acquire();
  std::cout << "  released a message and asked for another: same slot " << (second == first ? "yes" : "no")
            << ", its value " << (second ? second->value : -1.0) << " and id " << (second ? second->id : -1) << "\n\n";
  RC_REQUIRE(second != nullptr);
  RC_CHECK(second == first);        // the most recently returned is the most recently used, which is still warm
  RC_CHECK_NEAR(second->value, 0.0, 1e-12);
  RC_CHECK_EQ(second->id, 0);
}

RC_TEST("an empty pool says so and does not hand out a slot twice") {
  MessagePool pool;
  Message* held[64];
  for (int i = 0; i < 64; ++i) held[i] = pool.acquire();

  // Give one back and ask twice: the first gets it, the second is told there is none.
  RC_REQUIRE(held[10] != nullptr);
  RC_CHECK(pool.release(held[10]));
  Message* again = pool.acquire();
  RC_CHECK(again == held[10]);
  RC_CHECK(pool.acquire() == nullptr);
  RC_CHECK_EQ(pool.in_use(), 64);
}

RC_TEST("a release that was never an acquire is refused, and a double release does not lend a slot twice") {
  MessagePool pool;
  Message* a = pool.acquire();
  RC_REQUIRE(a != nullptr);

  Message stranger;
  RC_CHECK(!pool.release(nullptr));
  RC_CHECK(!pool.release(&stranger));                                           // not in the pool
  RC_CHECK(!pool.release(reinterpret_cast<Message*>(reinterpret_cast<char*>(a) + 1)));   // inside a slot, not at its start
  RC_CHECK_EQ(pool.in_use(), 1);

  RC_CHECK(pool.release(a));
  RC_CHECK(!pool.release(a));        // already back
  RC_CHECK_EQ(pool.in_use(), 0);

  // After the double release, sixty-four lends still give sixty-four different messages and then none.
  Message* lent[64];
  for (int i = 0; i < 64; ++i) lent[i] = pool.acquire();
  int distinct_count = 0;
  for (int i = 0; i < 64; ++i) {
    bool seen = false;
    for (int j = 0; j < i; ++j) {
      if (lent[j] == lent[i]) seen = true;
    }
    if (!seen && lent[i] != nullptr) ++distinct_count;
  }
  std::cout << "  after a refused double release: 64 lends gave " << distinct_count << " different messages, the 65th "
            << (pool.acquire() == nullptr ? "was refused" : "was lent") << "\n\n";
  RC_CHECK_EQ(distinct_count, 64);
  RC_CHECK(pool.acquire() == nullptr);
}
