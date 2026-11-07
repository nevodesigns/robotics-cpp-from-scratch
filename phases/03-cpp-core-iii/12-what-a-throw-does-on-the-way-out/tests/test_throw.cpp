#include <rc/test/rc_test.hpp>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include "solution.hpp"

namespace {

// Writes a line to a shared log when it is made and when it is destroyed.
std::vector<std::string>& log() {
  static std::vector<std::string> lines;
  return lines;
}

struct Guard {
  explicit Guard(const char* n) : name(n) { log().push_back(std::string("make ") + name); }
  ~Guard() { log().push_back(std::string("drop ") + name); }
  std::string name;
};

// One that throws a derived exception and is caught by value, and one by reference.
//
// Catching by value is the mistake on purpose, so the compiler's warning about it is switched off here.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcatch-value"
#endif
struct Caught {
  std::string message;
  bool still_derived = false;   // is what the handler holds still an invalid_argument?
};

Caught caught_by_value() {
  Caught result;
  try {
    throw std::invalid_argument("the specific message");
  } catch (std::exception e) {
    result.message = e.what();
    result.still_derived = typeid(e) == typeid(std::invalid_argument);
  }
  return result;
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

Caught caught_by_reference() {
  Caught result;
  try {
    throw std::invalid_argument("the specific message");
  } catch (const std::exception& e) {
    result.message = e.what();
    result.still_derived = typeid(e) == typeid(std::invalid_argument);
  }
  return result;
}

// Rethrow with `throw e;` and with `throw;`, then see which type arrives.
const char* rethrow_copy() {
  const char* result = "nothing";
  try {
    try {
      throw std::invalid_argument("x");
    } catch (const std::exception& e) {
      throw e;   // a copy, made from the type of e: std::exception
    }
  } catch (const std::invalid_argument&) {
    result = "still invalid_argument";
  } catch (const std::exception&) {
    result = "cut down to exception";
  }
  return result;
}

const char* rethrow_same() {
  const char* result = "nothing";
  try {
    try {
      throw std::invalid_argument("x");
    } catch (const std::exception&) {
      throw;
    }
  } catch (const std::invalid_argument&) {
    result = "still invalid_argument";
  } catch (const std::exception&) {
    result = "cut down to exception";
  }
  return result;
}

}  // namespace

RC_TEST("a number is read, and each kind of bad input is its own type with the text in its message") {
  RC_CHECK_EQ(parse_positive("42"), 42);
  RC_CHECK_EQ(parse_positive("2147483647"), 2147483647);

  bool invalid = false;
  std::string message;
  try {
    parse_positive("12abc");
  } catch (const std::invalid_argument& e) {
    invalid = true;
    message = e.what();
  }
  RC_CHECK(invalid);
  RC_CHECK(message.find("12abc") != std::string::npos);

  const char* out_of_range[] = {"0", "-3", "99999999999999999999", "2147483648"};
  int count = 0;
  for (const char* text : out_of_range) {
    try {
      parse_positive(text);
    } catch (const std::out_of_range&) {
      ++count;
    } catch (...) {
    }
  }
  RC_CHECK_EQ(count, 4);
}

RC_TEST("the function that does not throw keeps the two reasons apart") {
  int value = 0;
  std::string why;
  RC_CHECK(try_parse_positive("7", &value, &why));
  RC_CHECK_EQ(value, 7);

  why.clear();
  RC_CHECK(!try_parse_positive("seven", &value, &why));
  std::cout << "  seven:  " << why << "\n";
  RC_CHECK(why.find("bad input") == 0);

  why.clear();
  RC_CHECK(!try_parse_positive("-2", &value, &why));
  std::cout << "  -2:     " << why << "\n\n";
  RC_CHECK(why.find("bad range") == 0);
}

RC_TEST("on the way out of a throw, destructors run in reverse order, before the handler") {
  log().clear();
  try {
    Guard a("A");
    Guard b("B");
    Guard c("C");
    parse_positive("x");
    log().push_back("not reached");
  } catch (const std::invalid_argument&) {
    log().push_back("handler");
  }
  std::cout << "  the log:";
  for (const std::string& line : log()) std::cout << "  " << line << ",";
  std::cout << "\n\n";

  RC_REQUIRE(log().size() == 7);
  RC_CHECK_EQ(log()[3], std::string("drop C"));
  RC_CHECK_EQ(log()[4], std::string("drop B"));
  RC_CHECK_EQ(log()[5], std::string("drop A"));
  RC_CHECK_EQ(log()[6], std::string("handler"));
}

RC_TEST("catching by value cuts the exception down to its base, and by reference does not") {
  const Caught by_value = caught_by_value();
  const Caught by_reference = caught_by_reference();
  std::cout << "  thrown invalid_argument(\"the specific message\"):\n"
            << "    caught by value      message \"" << by_value.message << "\", still an invalid_argument: "
            << (by_value.still_derived ? "yes" : "no") << "\n"
            << "    caught by reference  message \"" << by_reference.message << "\", still an invalid_argument: "
            << (by_reference.still_derived ? "yes" : "no") << "\n"
            << "  (whether the message survives depends on the library; the type does not)\n\n";
  RC_CHECK(by_reference.still_derived);
  RC_CHECK_EQ(by_reference.message, std::string("the specific message"));
  RC_CHECK(!by_value.still_derived);

  // And the handler written in the exercise keeps what makes the error specific.
  int value = 0;
  std::string why;
  RC_CHECK(!try_parse_positive("12abc", &value, &why));
  RC_CHECK(why.find("12abc") != std::string::npos);
}

RC_TEST("throw; keeps the original type, and throw e; does not") {
  std::cout << "  rethrown with throw e;  : " << rethrow_copy() << "\n"
            << "  rethrown with throw;    : " << rethrow_same() << "\n\n";
  RC_CHECK_EQ(std::string(rethrow_copy()), std::string("cut down to exception"));
  RC_CHECK_EQ(std::string(rethrow_same()), std::string("still invalid_argument"));

  // The loop in the exercise does the second, so the caller can still tell the two errors apart.
  const char* texts[] = {"1", "x"};
  int out[2] = {0, 0};
  int failed = 0;
  std::string seen = "nothing";
  try {
    parse_all(texts, 2, out, &failed);
  } catch (const std::invalid_argument&) {
    seen = "invalid_argument";
  } catch (const std::exception&) {
    seen = "exception";
  }
  RC_CHECK_EQ(seen, std::string("invalid_argument"));
}

RC_TEST("parsing a list says which item failed and lets the same error carry on") {
  const char* good[] = {"1", "2", "3"};
  int out[3] = {0, 0, 0};
  int failed = 99;
  parse_all(good, 3, out, &failed);
  RC_CHECK_EQ(failed, -1);
  RC_CHECK_EQ(out[2], 3);

  const char* bad_text[] = {"4", "five", "6"};
  int out2[3] = {0, 0, 0};
  int failed2 = 99;
  bool as_invalid = false;
  try {
    parse_all(bad_text, 3, out2, &failed2);
  } catch (const std::invalid_argument&) {
    as_invalid = true;
  } catch (...) {
  }
  RC_CHECK(as_invalid);
  RC_CHECK_EQ(failed2, 1);
  RC_CHECK_EQ(out2[0], 4);
  RC_CHECK_EQ(out2[2], 0);       // never reached

  const char* range[] = {"7", "0"};
  int out3[2] = {0, 0};
  int failed3 = 99;
  bool as_range = false;
  try {
    parse_all(range, 2, out3, &failed3);
  } catch (const std::out_of_range&) {
    as_range = true;
  } catch (...) {
  }
  RC_CHECK(as_range);
  RC_CHECK_EQ(failed3, 1);
}
