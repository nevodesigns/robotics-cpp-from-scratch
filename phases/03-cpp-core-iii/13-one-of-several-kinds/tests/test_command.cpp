#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <variant>

#include "solution.hpp"

RC_TEST("each kind describes itself") {
  RC_CHECK_EQ(describe(Command{Stop{}}), std::string("stop"));
  RC_CHECK_EQ(describe(Command{Drive{0.5, -0.25}}), std::string("drive 0.5 -0.25"));
  RC_CHECK_EQ(describe(Command{SetLimit{1.5}}), std::string("limit 1.5"));
}

RC_TEST("a command is one kind, and the wrong question is answered rather than guessed") {
  Command c;                                  // the first alternative, which is why Stop has to be constructible
  RC_CHECK_EQ(c.index(), static_cast<std::size_t>(0));
  RC_CHECK(std::holds_alternative<Stop>(c));
  RC_CHECK_EQ(describe(c), std::string("stop"));

  c = Drive{1.0, 2.0};
  RC_CHECK_EQ(c.index(), static_cast<std::size_t>(1));
  RC_CHECK(!std::holds_alternative<Stop>(c));
  RC_CHECK(std::get_if<SetLimit>(&c) == nullptr);        // asking politely gives nothing
  RC_REQUIRE(std::get_if<Drive>(&c) != nullptr);
  RC_CHECK_NEAR(std::get_if<Drive>(&c)->right, 2.0, 1e-12);

  bool threw = false;
  try {
    (void)std::get<SetLimit>(c);              // asking as if sure throws
  } catch (const std::bad_variant_access&) {
    threw = true;
  }
  RC_CHECK(threw);
}

RC_TEST("applying commands changes the state, and a limit applies to what the wheels are doing now") {
  RobotState s;
  apply(s, Drive{0.8, -0.4});
  RC_CHECK_NEAR(s.left, 0.8, 1e-12);
  RC_CHECK_NEAR(s.right, -0.4, 1e-12);

  apply(s, SetLimit{0.5});
  std::cout << "  driving at 0.8 and -0.4, then a limit of 0.5: wheels now " << s.left << " and " << s.right << "\n";
  RC_CHECK_NEAR(s.left, 0.5, 1e-12);
  RC_CHECK_NEAR(s.right, -0.4, 1e-12);

  apply(s, Drive{3.0, -3.0});
  std::cout << "  then drive 3 and -3: wheels " << s.left << " and " << s.right << "\n";
  RC_CHECK_NEAR(s.left, 0.5, 1e-12);
  RC_CHECK_NEAR(s.right, -0.5, 1e-12);

  apply(s, Stop{});
  std::cout << "  then stop: wheels " << s.left << " and " << s.right << ", limit still " << s.limit << "\n\n";
  RC_CHECK_NEAR(s.left, 0.0, 1e-12);
  RC_CHECK_NEAR(s.right, 0.0, 1e-12);
  RC_CHECK_NEAR(s.limit, 0.5, 1e-12);
}

RC_TEST("lines are read into the right kind, and anything else is refused") {
  Command c;
  RC_CHECK(parse_command("stop", &c));
  RC_CHECK(std::holds_alternative<Stop>(c));

  RC_CHECK(parse_command("  drive   0.25  -0.5 ", &c));
  RC_REQUIRE(std::holds_alternative<Drive>(c));
  RC_CHECK_NEAR(std::get<Drive>(c).left, 0.25, 1e-12);
  RC_CHECK_NEAR(std::get<Drive>(c).right, -0.5, 1e-12);

  RC_CHECK(parse_command("limit 2", &c));
  RC_REQUIRE(std::holds_alternative<SetLimit>(c));
  RC_CHECK_NEAR(std::get<SetLimit>(c).max_speed, 2.0, 1e-12);

  const char* refused[] = {"", "spin 3", "drive 1", "drive a b", "drive 1 2 3", "stop now", "limit 0", "limit -1",
                           "limit", "limit 1 2"};
  int count = 0;
  for (const char* text : refused) {
    Command before = Drive{9.0, 9.0};
    Command after = before;
    if (!parse_command(text, &after)) ++count;
    // A refused line leaves the output as it was.
    RC_CHECK(std::holds_alternative<Drive>(after));
    RC_CHECK_NEAR(std::get<Drive>(after).left, 9.0, 1e-12);
  }
  std::cout << "  " << count << " of " << sizeof(refused) / sizeof(refused[0]) << " bad lines refused\n\n";
  RC_CHECK_EQ(count, 10);
}

RC_TEST("a command is the size of its largest kind and a tag") {
  const std::size_t sum = sizeof(Stop) + sizeof(Drive) + sizeof(SetLimit);
  std::cout << "  sizeof(Drive) " << sizeof(Drive) << ", sizeof(SetLimit) " << sizeof(SetLimit) << ", sizeof(Command) "
            << sizeof(Command) << ", the three added up " << sum << "\n\n";
  RC_CHECK(sizeof(Command) > sizeof(Drive));
  RC_CHECK(sizeof(Command) <= 2 * sizeof(Drive));
  RC_CHECK_EQ(describe(Command{Drive{1, 1}}), std::string("drive 1 1"));
}
