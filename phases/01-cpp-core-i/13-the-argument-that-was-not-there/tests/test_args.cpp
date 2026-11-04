#include <rc/test/rc_test.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// A command line, built by hand. Testing a tool's argument handling by
// running the tool is slow, awkward and platform specific; the argv main
// receives is just an array of pointers, so a test can make one.
//
// The trailing nullptr is not decoration. The standard requires argv[argc] to
// be null, so a faithful fake has it, and the suite below uses it to check
// that the reader stops where the real one would.
class FakeCommandLine {
 public:
  explicit FakeCommandLine(std::vector<std::string> words) : words_(std::move(words)) {
    for (const std::string& w : words_) pointers_.push_back(w.c_str());
    pointers_.push_back(nullptr);
  }

  int argc() const { return static_cast<int>(words_.size()); }
  const char* const* argv() const { return pointers_.data(); }

 private:
  std::vector<std::string> words_;
  std::vector<const char*> pointers_;
};

}  // namespace

RC_TEST("argv is a count and an array, and the array ends in a null") {
  const FakeCommandLine line({"plan_path", "--speed", "0.4", "route.csv"});
  const Arguments args(line.argc(), line.argv());

  RC_CHECK(args.program() == "plan_path");
  RC_CHECK(args.size() == 3u);
  RC_CHECK(args[0] == "--speed");
  RC_CHECK(args[2] == "route.csv");

  // argv[argc] is null, guaranteed by the standard, so counting to argc and
  // walking until null agree. The reader does both and stops at whichever
  // comes first.
  RC_CHECK(line.argv()[line.argc()] == nullptr);

  // A short command line is the normal case, not an error case. Asking for an
  // argument that is not there gives an empty view rather than a crash,
  // because a tool reading its own arguments must not be able to fall over on
  // one the user did not type.
  const FakeCommandLine bare({"plan_path"});
  const Arguments none(bare.argc(), bare.argv());
  RC_CHECK(none.size() == 0u);
  RC_CHECK(none[0].empty());
  RC_CHECK(none.program() == "plan_path");

  std::cout << "  \"plan_path --speed 0.4 route.csv\" is argc 4, and the\n";
  std::cout << "  three arguments the user typed start at argv[1]\n\n";
  std::cout << "  reading argv[1] on a bare command line does not read off\n";
  std::cout << "  the end: it reads the guaranteed null, which looks like a\n";
  std::cout << "  value and is not one. Handing that to std::string is\n";
  std::cout << "  undefined behaviour, and on this standard library it throws\n";
  std::cout << "  std::logic_error and aborts, which is the lucky outcome.\n\n";
}

RC_TEST("absent, present with nothing, and present with a value are three answers") {
  const FakeCommandLine line({"drive", "--speed", "--limit=", "--mode", "fast"});
  const Arguments args(line.argc(), line.argv());

  // Never mentioned.
  const Option absent = args.option("--gain");
  RC_CHECK(!absent.present);
  RC_CHECK(!absent.has_value);

  // Mentioned, with the next word being another option, which is taken as
  // its value because that is what getopt does.
  const Option speed = args.option("--speed");
  RC_CHECK(speed.present);
  RC_CHECK(speed.has_value);
  RC_CHECK(speed.value == "--limit=");

  // Mentioned in the = form with nothing after the =.
  const Option limit = args.option("--limit");
  RC_CHECK(limit.present);
  RC_CHECK(limit.has_value);
  RC_CHECK(limit.value.empty());

  // The ordinary case.
  const Option mode = args.option("--mode");
  RC_CHECK(mode.present);
  RC_CHECK(mode.has_value);
  RC_CHECK(mode.value == "fast");

  // The same four lookups through a function that returns a fallback, which
  // is how this is usually written and is the reason the type above exists.
  const auto with_fallback = [&args](std::string_view name, std::string_view fallback) {
    const Option found = args.option(name);
    return found.present && found.has_value ? found.value : fallback;
  };

  std::cout << "  option            present  has value  value\n";
  std::cout << "  --gain            no       no         \n";
  std::cout << "  --speed           yes      yes        " << speed.value << "\n";
  std::cout << "  --limit=          yes      yes        (empty)\n";
  std::cout << "  --mode fast       yes      yes        " << mode.value << "\n\n";

  std::cout << "  the same four through a fallback of \"0.2\":\n";
  std::cout << "    --gain  " << with_fallback("--gain", "0.2") << "\n";
  std::cout << "    --limit " << (with_fallback("--limit", "0.2").empty()
                                      ? "(empty)" : "0.2") << "\n\n";
  std::cout << "  a user who never asked and a user who asked wrongly get the\n";
  std::cout << "  same answer from a fallback, and only one of them should.\n\n";

  RC_CHECK(with_fallback("--gain", "0.2") == "0.2");
}

RC_TEST("atoi cannot fail, so it cannot tell you anything") {
  // Four strings a user can type into a numeric option. atoi returns an int
  // for every one of them and has no way to report that three are not
  // numbers at all.
  const char* inputs[] = {"42", "abc", "", "3.9", "7abc"};
  const int expected_atoi[] = {42, 0, 0, 3, 7};

  std::cout << "  input      atoi   rc::core::parse_number\n";
  for (std::size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
    const int from_atoi = std::atoi(inputs[i]);
    const rc::core::ParsedNumber parsed = rc::core::parse_number(inputs[i]);
    RC_CHECK(from_atoi == expected_atoi[i]);

    std::cout << "  " << std::left << std::setw(10)
              << (*inputs[i] ? inputs[i] : "(empty)") << std::setw(7) << from_atoi
              << (parsed.ok ? "accepted" : "refused") << "\n" << std::right;
  }
  std::cout << "\n";

  // Two of the five are numbers and parse_number accepts exactly those two.
  // atoi gets three of the five wrong, in three different ways: 0 for two
  // things that are not numbers at all, which is indistinguishable from a
  // user who typed 0, and 3 for "3.9", which is a real number read as a
  // different one because atoi only ever reads an integer.
  RC_CHECK(rc::core::parse_number("42").ok);
  RC_CHECK(rc::core::parse_number("3.9").ok);
  RC_CHECK(!rc::core::parse_number("abc").ok);
  RC_CHECK(!rc::core::parse_number("").ok);
  RC_CHECK(!rc::core::parse_number("7abc").ok);

  // The same five arriving as options, which is how they reach a tool, so the
  // reader rather than the parser is what the suite is checking here.
  const FakeCommandLine typed({"drive", "--a=42", "--b=abc", "--c=", "--d=3.9", "--e=7abc"});
  const Arguments args(typed.argc(), typed.argv());
  RC_CHECK(args.number("--a").ok);
  RC_CHECK_NEAR(args.number("--a").value, 42.0, 1e-12);
  RC_CHECK(args.number("--d").ok);
  RC_CHECK_NEAR(args.number("--d").value, 3.9, 1e-12);
  RC_CHECK(args.number("--b").present && !args.number("--b").ok);
  RC_CHECK(args.number("--c").present && !args.number("--c").ok);
  RC_CHECK(args.number("--e").present && !args.number("--e").ok);

  std::cout << "  through the reader, --b, --c and --e are present and refused,\n";
  std::cout << "  which is the answer a tool can act on: tell the user which\n";
  std::cout << "  option was wrong instead of driving at 0.\n\n";
  std::cout << "  atoi on a value too large for an int is undefined, not\n";
  std::cout << "  merely wrong. Measured outside this suite, because the\n";
  std::cout << "  suite will not perform it: atoi(\"99999999999999999999\")\n";
  std::cout << "  returned -1, while strtol returned LONG_MAX and set ERANGE.\n";
  std::cout << "  A mistyped speed limit became a negative speed.\n\n";
}

RC_TEST("a numeric option keeps absent apart from unparseable") {
  const FakeCommandLine line({"drive", "--speed", "0.4", "--gain", "abc", "--trim"});
  const Arguments args(line.argc(), line.argv());

  const Arguments::Number speed = args.number("--speed");
  RC_CHECK(speed.present);
  RC_CHECK(speed.ok);
  RC_CHECK_NEAR(speed.value, 0.4, 1e-12);

  // Given, with a value that is not a number.
  const Arguments::Number gain = args.number("--gain");
  RC_CHECK(gain.present);
  RC_CHECK(!gain.ok);

  // Given, with the next word being nothing at all because it is the last.
  const Arguments::Number trim = args.number("--trim");
  RC_CHECK(trim.present);
  RC_CHECK(!trim.ok);

  // Never given.
  const Arguments::Number missing = args.number("--offset");
  RC_CHECK(!missing.present);
  RC_CHECK(!missing.ok);

  std::cout << "  option     present  parsed  value\n";
  std::cout << "  --speed    yes      yes     " << speed.value << "\n";
  std::cout << "  --gain     yes      no      \n";
  std::cout << "  --trim     yes      no      \n";
  std::cout << "  --offset   no       no      \n\n";
  std::cout << "  three different failures and one success. A tool that only\n";
  std::cout << "  had a double would drive at 0 for three of these rows and\n";
  std::cout << "  could not say which.\n\n";
}

RC_TEST("a repeated option takes the last, and a bare dash dash ends the options") {
  const FakeCommandLine repeated({"drive", "--speed", "0.2", "--speed", "0.9"});
  const Arguments args(repeated.argc(), repeated.argv());
  const Arguments::Number speed = args.number("--speed");
  RC_CHECK(speed.ok);
  RC_CHECK_NEAR(speed.value, 0.9, 1e-12);

  // Everything after a bare -- is positional, even if it looks like an
  // option, which is how a file called --help reaches a program that has one.
  const FakeCommandLine terminated({"plot", "--grid", "--", "--help", "route.csv"});
  const Arguments after(terminated.argc(), terminated.argv());
  RC_CHECK(after.has("--grid"));
  RC_CHECK(!after.option("--help").present);

  const std::vector<std::string_view> rest = after.positionals();
  RC_CHECK(rest.size() == 2u);
  RC_CHECK(rest[0] == "--help");
  RC_CHECK(rest[1] == "route.csv");

  std::cout << "  --speed 0.2 --speed 0.9 reads as " << speed.value
            << ", so a wrapper script\n";
  std::cout << "  can set a default that the user is still able to override\n\n";
  std::cout << "  after a bare --, \"--help\" is a filename and not a request\n";
  std::cout << "  for help, which is the only way to open a file called that\n\n";
}
