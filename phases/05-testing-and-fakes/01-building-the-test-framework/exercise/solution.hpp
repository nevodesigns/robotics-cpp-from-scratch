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

// The registry of every test that registered itself before main.
//
// TODO: hold the vector as a static local inside this function rather than as a
// plain global. A registrar in another translation unit may run before a global
// is constructed, and the order is undefined, but a static local is built the
// first time control reaches it, which is necessarily before any use.
inline std::vector<Case>& registry() {
  static std::vector<Case> cases;
  return cases;
}

// Failures recorded during the current run, same reasoning.
inline std::vector<Failure>& failures() {
  static std::vector<Failure> found;
  return found;
}

// The name of the test currently running, so a failure can say where it came
// from. Provided for you.
inline std::string& current_test() {
  static std::string name;
  return name;
}

// Registering happens in this constructor, which runs before main because the
// object it belongs to lives at file scope.
struct Registrar {
  Registrar(const char* name, void (*fn)()) {
    // TODO: append this case to the registry.
    (void)name;
    (void)fn;
  }
};

// Records one failure against the test that is running.
inline void fail(const char* file, int line, const std::string& message) {
  // TODO
  (void)file;
  (void)line;
  (void)message;
}

// Runs every registered case and reports how many passed and failed.
//
// Must give the same answer when called twice, which means clearing the
// recorded failures before starting rather than accumulating across runs.
inline Report run_all() {
  // TODO
  return Report{};
}

}  // namespace mini

// Two levels of macro, because pasting __LINE__ directly would paste the text
// rather than the number it expands to. This is a genuine preprocessor wart and
// the reason the real framework has the same pair.
#define MINI_CONCAT_INNER(a, b) a##b
#define MINI_CONCAT(a, b) MINI_CONCAT_INNER(a, b)

// Expands to a declaration, a registrar object whose constructor does the
// registering, and the beginning of a function definition. The macro ends
// without a brace on purpose, so the block you write becomes the body.
//
// Written for you. Read it until it is obvious.
#define MINI_TEST(name)                                                   \
  static void MINI_CONCAT(mini_fn_, __LINE__)();                          \
  static const ::mini::Registrar MINI_CONCAT(mini_reg_, __LINE__)(        \
      name, &MINI_CONCAT(mini_fn_, __LINE__));                            \
  static void MINI_CONCAT(mini_fn_, __LINE__)()

// TODO: record a failure when expr is false, naming the expression.
//
// #expr turns the expression into a string literal, which is how a report can
// print the code that failed rather than only the values. Wrap the body in
// do { ... } while (false) so the macro behaves like one statement.
#define MINI_CHECK(expr) \
  do {                   \
  } while (false)

#endif  // LESSON_SOLUTION_HPP
