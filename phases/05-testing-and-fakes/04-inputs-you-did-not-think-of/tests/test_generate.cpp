#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// The code under test: read an unsigned decimal number.
//
// `checked` is whether the overflow test is present. With it missing the parser
// reports success for a number too large to hold, having quietly wrapped it
// around, which is a real defect in real parsers and is exactly the sort that
// examples do not find.
struct Parsed {
  bool ok = false;
  unsigned long long value = 0;
};

Parsed parse(const std::string& text, bool checked) {
  if (text.empty()) return {};

  unsigned long long value = 0;
  for (const char c : text) {
    if (c < '0' || c > '9') return {};
    const unsigned digit = static_cast<unsigned>(c - '0');
    if (checked && value > (std::numeric_limits<unsigned long long>::max() - digit) / 10ULL)
      return {};
    value = value * 10ULL + digit;
  }
  return {true, value};
}

std::string without_leading_zeros(const std::string& text) {
  std::size_t i = 0;
  while (i + 1 < text.size() && text[i] == '0') ++i;
  return text.substr(i);
}

// The property: if the parser accepted the text, the value it produced prints
// back as the text it was given.
//
// "it read the number you wrote", which is the whole of what a parser is for. A
// weaker property, that the value survives being printed and read again, is
// true even when the parser has wrapped: the wrapped value is a perfectly good
// number and round trips as itself. Choosing the property is most of the work.
bool reads_what_was_written(const std::string& text, bool checked) {
  const Parsed parsed = parse(text, checked);
  if (!parsed.ok) return true;
  return std::to_string(parsed.value) == without_leading_zeros(text);
}

std::string any_printable(Source& source) {
  const int length = source.below(40);
  std::string text;
  for (int i = 0; i < length; ++i)
    text.push_back(static_cast<char>(32 + source.below(95)));
  return text;
}

std::string digits_to_forty(Source& source) {
  const int length = source.below(40);
  std::string text;
  for (int i = 0; i < length; ++i)
    text.push_back(static_cast<char>('0' + source.below(10)));
  return text;
}

std::string digits_to_ten(Source& source) {
  const int length = source.below(10);
  std::string text;
  for (int i = 0; i < length; ++i)
    text.push_back(static_cast<char>('0' + source.below(10)));
  return text;
}

// What nine careful examples look like.
const std::vector<std::string>& by_hand() {
  static const std::vector<std::string> cases = {"0",  "1",  "42",  "999", "1000000",
                                                 "",   "x",  "12x", "007"};
  return cases;
}

struct Sweep {
  int seeds_that_found = 0;
  double mean_trials = 0.0;
};

// How many seeds find a counterexample, and how many values it took. No
// shrinking here: this measures the generator, and shrinking is measured on its
// own below.
template <class Generate>
Sweep sweep(Generate generate, bool checked, long trials, int seeds = 20) {
  Sweep result;
  long total = 0;

  for (std::uint64_t seed = 1; seed <= static_cast<std::uint64_t>(seeds); ++seed) {
    Source source(seed);
    for (long i = 1; i <= trials; ++i) {
      if (reads_what_was_written(generate(source), checked)) continue;
      ++result.seeds_that_found;
      total += i;
      break;
    }
  }

  if (result.seeds_that_found > 0)
    result.mean_trials = static_cast<double>(total) / result.seeds_that_found;
  return result;
}

}  // namespace

RC_TEST("a source asked twice for the same seed gives the same values") {
  Source first(7), second(7), other(8);

  std::vector<int> a, b, c;
  for (int i = 0; i < 50; ++i) {
    a.push_back(first.below(1000));
    b.push_back(second.below(1000));
    c.push_back(other.below(1000));
  }

  RC_CHECK(a == b);
  RC_CHECK(a != c);
  RC_CHECK_EQ(first.seed(), static_cast<std::uint64_t>(7));

  // Everything asked for is inside the range, including the awkward limits.
  Source source(1);
  for (int i = 0; i < 1000; ++i) {
    const int value = source.below(3);
    RC_CHECK(value >= 0);
    RC_CHECK(value < 3);
  }
  RC_CHECK_EQ(source.below(1), 0);
  RC_CHECK_EQ(source.below(0), 0);
  RC_CHECK_EQ(source.below(-5), 0);
}

RC_TEST("nine careful examples, and what they miss") {
  int caught = 0;
  for (const std::string& text : by_hand())
    if (!reads_what_was_written(text, false)) ++caught;

  std::cout << "\n    nine hand written cases against a parser with no overflow\n";
  std::cout << "    check: " << caught << " of them fail\n";

  // None of them. They are the cases somebody thought of, and the person who
  // thinks of the cases is the person who wrote the code.
  RC_CHECK_EQ(caught, 0);

  // The value that does catch it is not exotic, it is just longer than anybody
  // types into a test by hand.
  RC_CHECK(!reads_what_was_written("18446744073709551616", false));
  RC_CHECK(reads_what_was_written("18446744073709551616", true));
}

RC_TEST("the generator decides whether the fault is reachable") {
  std::cout << "\n    a hundred thousand values per seed, twenty seeds\n\n";
  std::cout << "    " << std::left << std::setw(22) << "generator" << std::right
            << std::setw(16) << "found it in" << std::setw(16) << "mean trials" << "\n";

  const Sweep wide = sweep(any_printable, false, 100000);
  const Sweep aimed = sweep(digits_to_forty, false, 100000);
  const Sweep blind = sweep(digits_to_ten, false, 100000);

  const auto row = [](const char* name, const Sweep& s) {
    std::cout << "    " << std::left << std::setw(22) << name << std::right
              << std::setw(13) << s.seeds_that_found << " of 20" << std::setw(16)
              << std::fixed << std::setprecision(1)
              << (s.seeds_that_found > 0 ? s.mean_trials : 0.0) << "\n";
  };
  row("any printable byte", wide);
  row("digits, up to 40", aimed);
  row("digits, up to 10", blind);

  std::cout << "\n    the first almost never emits twenty digits in a row. The\n";
  std::cout << "    third cannot emit twenty digits at all, so it would report\n";
  std::cout << "    this parser correct for ever\n";

  // Aimed at the shape of the input, it finds the fault almost at once.
  RC_CHECK_EQ(aimed.seeds_that_found, 20);
  RC_CHECK(aimed.mean_trials < 10.0);

  // Too wide, and it never gets there.
  RC_CHECK_EQ(wide.seeds_that_found, 0);

  // Too narrow, and it cannot get there. Two million values, no failure, and a
  // green tick that means nothing at all.
  RC_CHECK_EQ(blind.seeds_that_found, 0);

  // And that one is not luck, it is arithmetic: the largest number nine digits
  // can spell is nowhere near the largest this parser can hold, so no value
  // that generator can produce will ever overflow. A generator has a reach, and
  // a passing property test is a statement about the reach as much as the code.
  RC_CHECK(!reads_what_was_written("99999999999999999999", false));
  for (int length = 1; length <= 9; ++length)
    RC_CHECK(reads_what_was_written(std::string(static_cast<std::size_t>(length), '9'),
                                    false));
}

RC_TEST("shrinking is what makes the failure readable") {
  std::cout << "\n    what the aimed generator found, before and after shrinking\n\n";
  std::cout << "    " << std::right << std::setw(6) << "seed" << std::setw(6)
            << "len" << "  " << std::left << std::setw(40) << "as found"
            << "shrunk" << "\n";

  const auto holds = [](const std::string& text) {
    return reads_what_was_written(text, false);
  };

  double raw = 0.0, small = 0.0;
  int counted = 0;
  for (std::uint64_t seed = 1; seed <= 6; ++seed) {
    Source probe(seed);
    std::string failure;
    for (long i = 1; i <= 100000; ++i) {
      const std::string candidate = digits_to_forty(probe);
      if (!holds(candidate)) { failure = candidate; break; }
    }
    RC_REQUIRE(!failure.empty());

    int steps = 0;
    const std::string shrunk = shrink(failure, simpler_strings, holds, &steps);
    raw += static_cast<double>(failure.size());
    small += static_cast<double>(shrunk.size());
    ++counted;

    std::cout << "    " << std::right << std::setw(6) << seed << std::setw(6)
              << failure.size() << "  " << std::left << std::setw(40) << failure
              << shrunk << "\n";

    // Whatever it shrank to still fails, which is the only thing a shrinker
    // must never get wrong.
    RC_CHECK(!holds(shrunk));
    RC_CHECK(shrunk.size() <= failure.size());
  }

  std::cout << "\n    mean length " << std::fixed << std::setprecision(2)
            << raw / counted << " before, " << small / counted << " after\n";
  std::cout << "\n    twenty digits is the shortest number this parser gets\n";
  std::cout << "    wrong, and the shrinker finds that without being told\n";

  // Every one lands on twenty characters, which is the shortest input that can
  // overflow, and the shape of the bug is legible in the value.
  RC_CHECK_NEAR(small / counted, 20.0, 0.01);
  RC_CHECK(raw / counted > 25.0);
}

RC_TEST("a property that holds, and the trials it takes to believe it") {
  const auto holds = [](const std::string& text) {
    return reads_what_was_written(text, true);
  };

  Source source(1);
  const auto result =
      hunt<std::string>(source, 50000, digits_to_forty, holds, simpler_strings);

  std::cout << "\n    with the overflow check in place: " << result.trials
            << " values, no counterexample\n";

  RC_CHECK(!result.found);
  RC_CHECK_EQ(result.trials, 50000L);

  // And the report says which seed, so the same two hundred thousand values can
  // be asked for again. A run that cannot be repeated is not evidence.
  RC_CHECK_EQ(source.seed(), static_cast<std::uint64_t>(1));
}

RC_TEST("a property that cannot fail proves nothing") {
  // The trap: a property written so that it is true whatever the code does.
  // This one only ever looks at values the parser rejected.
  const auto vacuous = [](const std::string& text) {
    const Parsed parsed = parse(text, false);
    if (parsed.ok) return true;   // says nothing about anything accepted
    return parsed.value == 0;
  };

  Source source(1);
  const auto result =
      hunt<std::string>(source, 50000, digits_to_forty, vacuous, simpler_strings);
  RC_CHECK(!result.found);

  // It passes on the broken parser and on the fixed one, which is the tell.
  const auto real = [](const std::string& text) {
    return reads_what_was_written(text, false);
  };
  Source again(1);
  const auto genuine =
      hunt<std::string>(again, 50000, digits_to_forty, real, simpler_strings);
  RC_CHECK(genuine.found);

  std::cout << "\n    a property that passes on the broken parser and on the\n";
  std::cout << "    fixed one is measuring neither. The way to find out is the\n";
  std::cout << "    same as for any test: break the code on purpose and watch\n";
  std::cout << "    it fail\n";
}
