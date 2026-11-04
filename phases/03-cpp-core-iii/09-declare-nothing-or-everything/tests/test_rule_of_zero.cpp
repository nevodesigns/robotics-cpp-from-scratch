#include <rc/test/rc_test.hpp>

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <type_traits>

#include "solution.hpp"

namespace {

// Written at file scope because rule L022 forbids a local constexpr: MSVC
// rejects one used inside a lambda as C3493.
constexpr int kRows = 64;

}  // namespace

RC_TEST("the trait everybody checks says yes to all of them") {
  // std::is_move_constructible asks whether a T can be built from an rvalue
  // T. A copy constructor does that, so a class with no move constructor at
  // all still answers yes.
  const bool quiet = std::is_move_constructible<Quiet>::value;
  const bool with_destructor = std::is_move_constructible<WithDestructor>::value;
  const bool with_copy = std::is_move_constructible<WithCopy>::value;

  std::cout << "\n  std::is_move_constructible\n\n";
  std::cout << "    nothing declared          " << (quiet ? "yes" : "no") << "\n";
  std::cout << "    ~X() = default declared   " << (with_destructor ? "yes" : "no") << "\n";
  std::cout << "    copy constructor declared " << (with_copy ? "yes" : "no") << "\n\n";

  RC_CHECK(quiet);
  RC_CHECK(with_destructor);
  RC_CHECK(with_copy);

  // And the honest trait, side by side, disagreeing about two of them.
  RC_CHECK(moves_cheaply<Quiet>());
  RC_CHECK(!moves_cheaply<WithDestructor>());
  RC_CHECK(!moves_cheaply<WithCopy>());

  std::cout << "  all three, and two of them have no move constructor whatever.\n";
  std::cout << "  The trait is satisfied by copying, so it cannot tell you this.\n\n";
}

RC_TEST("the trait that tells the truth, and what it predicts") {
  std::cout << "  " << std::left << std::setw(28) << "class" << std::setw(22)
            << "moves cheaply" << "buffer on move\n" << std::right;

  struct Row {
    const char* name;
    bool cheap;
    bool travels;
  };
  const Row rows[] = {
      {"nothing declared", moves_cheaply<Quiet>(), buffer_travels_on_move<Quiet>()},
      {"~X() = default declared", moves_cheaply<WithDestructor>(),
       buffer_travels_on_move<WithDestructor>()},
      {"copy constructor declared", moves_cheaply<WithCopy>(),
       buffer_travels_on_move<WithCopy>()},
      {"all five declared", moves_cheaply<AllFive>(), buffer_travels_on_move<AllFive>()},
  };

  for (const Row& row : rows) {
    std::cout << "  " << std::left << std::setw(28) << row.name << std::setw(22)
              << (row.cheap ? "yes" : "NO")
              << (row.travels ? "travelled" : "COPIED") << "\n" << std::right;
  }
  std::cout << "\n";

  // The trait and the behaviour agree on every row, which is the whole reason
  // to check the nothrow one.
  for (const Row& row : rows) RC_CHECK(row.cheap == row.travels);

  // Declaring nothing works. Declaring one thing breaks it.
  RC_CHECK(moves_cheaply<Quiet>());
  RC_CHECK(!moves_cheaply<WithDestructor>());
  RC_CHECK(!moves_cheaply<WithCopy>());

  // And declaring all five, every one of them = default, puts it back.
  RC_CHECK(moves_cheaply<AllFive>());

  // The destructor is the one worth dwelling on. It is written = default, it
  // does nothing, it is as close to not being there as the language allows,
  // and it still suppresses the move constructor.
  RC_CHECK(buffer_travels_on_move<Quiet>());
  RC_CHECK(!buffer_travels_on_move<WithDestructor>());
}

RC_TEST("what the destructor costs when the container grows") {
  const std::size_t quiet = allocations_to_grow<Quiet>(kRows);
  const std::size_t burdened = allocations_to_grow<WithDestructor>(kRows);
  const std::size_t restored = allocations_to_grow<AllFive>(kRows);

  std::cout << "  pushing " << kRows << " rows, each holding 64 doubles\n\n";
  std::cout << "  " << std::left << std::setw(28) << "element type" << std::right
            << std::setw(14) << "allocations" << "\n";
  const auto row = [](const char* name, std::size_t count) {
    std::cout << "  " << std::left << std::setw(28) << name << std::right
              << std::setw(14) << count << "\n";
  };
  row("nothing declared", quiet);
  row("~X() = default declared", burdened);
  row("all five declared", restored);
  std::cout << "\n";

  // Every reallocation of the outer vector has to relocate the rows. Moving
  // one steals its buffer and allocates nothing; copying one allocates a new
  // buffer for every row it relocates.
  RC_CHECK(burdened > quiet);
  RC_CHECK(restored == quiet);

  // Not a rounding difference. The ratio is the thing.
  RC_CHECK(burdened > quiet * 2);
  std::cout << "  one defaulted destructor, " << (burdened - quiet)
            << " extra allocations, from a line that does nothing\n\n";
}

RC_TEST("a vector only moves its elements when moving cannot throw") {
  // The condition the standard library uses, and the reason the nothrow trait
  // is the one that matters. A vector reallocating has already allocated the
  // new buffer and is part way through relocating; if a move threw halfway it
  // could not put the elements back, so it copies instead and keeps the strong
  // guarantee.
  RC_CHECK(std::is_nothrow_move_constructible<Quiet>::value);
  RC_CHECK(!std::is_nothrow_move_constructible<WithDestructor>::value);

  // Which is why a class can be move constructible and still be copied by
  // every container it is put into.
  RC_CHECK(std::is_move_constructible<WithDestructor>::value);
  RC_CHECK(!std::is_nothrow_move_constructible<WithDestructor>::value);

  // Which is exactly what moves_cheaply reports, and the reason it asks the
  // nothrow question rather than the obvious one.
  RC_CHECK(moves_cheaply<Quiet>());
  RC_CHECK(!moves_cheaply<WithDestructor>());

  std::cout << "  WithDestructor is move constructible and not nothrow move\n";
  std::cout << "  constructible, so every container copies it and the trait\n";
  std::cout << "  most people check reports no problem\n\n";
}

RC_TEST("declaring all five is correct and is not the advice") {
  // AllFive behaves exactly as Quiet does, and it took six lines to say so.
  RC_CHECK(moves_cheaply<AllFive>());
  RC_CHECK(buffer_travels_on_move<AllFive>());
  RC_CHECK(allocations_to_grow<Quiet>(kRows) > 0u);
  RC_CHECK(moves_cheaply<AllFive>() == moves_cheaply<Quiet>());
  RC_CHECK(buffer_travels_on_move<AllFive>() == buffer_travels_on_move<Quiet>());
  RC_CHECK(allocations_to_grow<AllFive>(kRows) == allocations_to_grow<Quiet>(kRows));

  // Those six lines now have to stay correct. Add a member that needs special
  // handling, change one of them, and the others no longer match. Quiet has
  // nothing to keep in step with anything.
  std::cout << "  AllFive matches Quiet on every measurement, and cost six lines\n";
  std::cout << "  that have to be maintained to go on matching it\n\n";

  // The copy operations still work on both, which is the part people fear
  // losing when they are told not to declare anything.
  Quiet original;
  original.samples.assign(8, 2.5);
  Quiet copied = original;
  RC_CHECK(copied.samples.size() == 8u);
  RC_CHECK_NEAR(copied.samples[0], 2.5, 1e-12);
  RC_CHECK(original.samples.size() == 8u);
  RC_CHECK(address_of(copied.samples.data()) != address_of(original.samples.data()));
}
