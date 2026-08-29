#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <string>
#include <vector>

namespace mini {

struct Case {
  std::string name;
  void (*fn)();
};

struct Failure {
  std::string test;
  std::string file;
  int line = 0;
  std::string message;
};

struct Report {
  int passed = 0;
  int failed = 0;
};

// A static local rather than a global. Registrars in different translation
// units run in an order nobody defines, so a global vector might not exist yet
// when one of them appends to it. A static local is constructed the first time
// control reaches its declaration, which is necessarily before the first use.
inline std::vector<Case>& registry() {
  static std::vector<Case> cases;
  return cases;
}

inline std::vector<Failure>& failures() {
  static std::vector<Failure> found;
  return found;
}

inline std::string& current_test() {
  static std::string name;
  return name;
}

struct Registrar {
  Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

inline void fail(const char* file, int line, const std::string& message) {
  failures().push_back({current_test(), file, line, message});
}

inline Report run_all() {
  // Cleared rather than accumulated, so a second run reports the same thing as
  // the first. A framework whose answer depends on how many times it has been
  // asked is not much of a framework.
  failures().clear();

  Report report;
  for (const Case& test : registry()) {
    const std::size_t before = failures().size();
    current_test() = test.name;
    test.fn();
    current_test().clear();

    if (failures().size() == before) ++report.passed;
    else ++report.failed;
  }
  return report;
}

}  // namespace mini

#define MINI_CONCAT_INNER(a, b) a##b
#define MINI_CONCAT(a, b) MINI_CONCAT_INNER(a, b)

#define MINI_TEST(name)                                                   \
  static void MINI_CONCAT(mini_fn_, __LINE__)();                          \
  static const ::mini::Registrar MINI_CONCAT(mini_reg_, __LINE__)(        \
      name, &MINI_CONCAT(mini_fn_, __LINE__));                            \
  static void MINI_CONCAT(mini_fn_, __LINE__)()

// do while false rather than a bare block, so that the macro is one statement
// and behaves correctly as the body of an if without braces. The stringified
// expression is what lets a report name the code rather than only the values.
#define MINI_CHECK(expr)                                                     \
  do {                                                                       \
    if (!(expr)) {                                                           \
      ::mini::fail(__FILE__, __LINE__,                                       \
                   std::string("expected this to be true: " #expr));         \
    }                                                                        \
  } while (false)

#endif  // LESSON_SOLUTION_HPP
