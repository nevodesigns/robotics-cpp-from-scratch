#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>

#include "solution.hpp"

namespace {

// The two overloads most people write, with nothing for a string literal.
class Naive {
 public:
  explicit Naive(std::string* log) : log_(log) {}
  void report(bool ok) { *log_ += ok ? "flag: ok;" : "flag: failed;"; }
  void report(const std::string& message) { *log_ += "message: " + message + ";"; }

 private:
  std::string* log_;
};

// A default argument on a virtual function.
class BadSink {
 public:
  virtual ~BadSink() = default;
  virtual std::string send(const std::string& text, int retries = 3) const {
    return text + " retries=" + std::to_string(retries);
  }
};

class BadQuiet : public BadSink {
 public:
  std::string send(const std::string& text, int retries = 0) const override {
    return text + " retries=" + std::to_string(retries);
  }
};

// Which of these did a literal zero and a nullptr choose?
const char* which(int) { return "int"; }
const char* which(const char*) { return "pointer"; }

}  // namespace

RC_TEST("a string literal chooses the bool overload when nothing says otherwise") {
  std::string log;
  Naive naive(&log);
  naive.report("transmission failed");
  std::cout << "  without a const char* overload, report(\"transmission failed\") wrote: " << log << "\n";
  RC_CHECK_EQ(log, std::string("flag: ok;"));     // the pointer became a bool, and a pointer is not null

  // The reporter of this lesson does not make that mistake.
  std::string fixed_log;
  Reporter fixed(&fixed_log);
  fixed.report("transmission failed");
  RC_CHECK_EQ(fixed_log, std::string("message: transmission failed;"));
}

RC_TEST("with the const char* overload a string literal is a message, and flags and strings still work") {
  std::string log;
  Reporter r(&log);
  r.report("transmission failed");
  r.report(std::string("retrying"));
  r.report(true);
  r.report(false);
  std::cout << "  the same call with the third overload wrote: " << log << "\n\n";
  RC_CHECK_EQ(log, std::string("message: transmission failed;message: retrying;flag: ok;flag: failed;"));
}

RC_TEST("a default argument on a virtual function comes from the reference's type, not the object's") {
  BadQuiet quiet;
  const BadSink& as_base = quiet;
  std::cout << "  a quiet sink that defaults to 0 retries: called on itself " << quiet.send("x") << ", through a reference to the base "
            << as_base.send("x") << "\n";
  RC_CHECK_EQ(quiet.send("x"), std::string("x retries=0"));
  RC_CHECK_EQ(as_base.send("x"), std::string("x retries=3"));      // the derived function ran, with the base's default

  // The sink of this lesson takes the default from the object.
  Quiet fixed;
  const Sink& fixed_base = fixed;
  RC_CHECK_EQ(fixed_base.send("x"), std::string("x retries=0"));
}

RC_TEST("a default that comes from a virtual function follows the object") {
  Quiet quiet;
  Sink plain;
  const Sink& as_base = quiet;
  std::cout << "  the same with a virtual default: plain sink " << plain.send("x") << ", quiet sink through a base reference "
            << as_base.send("x") << "\n\n";
  RC_CHECK_EQ(plain.send("x"), std::string("x retries=3"));
  RC_CHECK_EQ(as_base.send("x"), std::string("x retries=0"));
  RC_CHECK_EQ(as_base.send_with("x", 7), std::string("x retries=7"));      // an explicit number still wins
}

RC_TEST("a literal zero is an int, and only nullptr is a pointer") {
  std::cout << "  which(0) chose " << which(0) << ", which(nullptr) chose " << which(nullptr) << "\n\n";
  RC_CHECK_EQ(std::string(which(0)), std::string("int"));
  RC_CHECK_EQ(std::string(which(nullptr)), std::string("pointer"));

  // And the reporter's overload for a pointer is not tricked by a null one either, as long as it is not called with one.
  std::string log;
  Reporter r(&log);
  r.report("ok");
  RC_CHECK_EQ(log, std::string("message: ok;"));
}
