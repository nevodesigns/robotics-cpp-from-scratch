#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <string>

// Say how something went, either as a yes or no or as words. Each way writes a line in a shared
// log that the test reads back, so which overload ran is visible.
class Reporter {
 public:
  explicit Reporter(std::string* log) : log_(log) {}

  void report(bool ok) { *log_ += ok ? "flag: ok;" : "flag: failed;"; }

  void report(const std::string& message) { *log_ += "message: " + message + ";"; }

  // TODO: a third overload, report(const char* message), that calls report(std::string(message)).
  // A string literal becomes a const char* for free and a pointer becomes a bool with one more
  // built-in conversion, which beats the constructor that would make a std::string. Without this
  // function report("done") picks the bool one.

 private:
  std::string* log_;
};

// Something that sends text, trying again a number of times when it fails.
//
// The default number of retries is a virtual function and not a default argument. A default argument
// is filled in by the compiler from the type of the reference it is called through, which is not the
// type of the object, so a default argument on a virtual function takes the base class's value even
// when it is a derived object's function that runs.
class Sink {
 public:
  virtual ~Sink() = default;

  // Not virtual: fills in this object's own default, then calls the one that is.
  std::string send(const std::string& text) const {
    // TODO: return send_with(text, default_retries());
    (void)text;
    return "";
  }

  virtual std::string send_with(const std::string& text, int retries) const {
    return text + " retries=" + std::to_string(retries);
  }

  virtual int default_retries() const { return 3; }
};

// A sink for a quiet link: it does not try again unless asked to.
class Quiet : public Sink {
 public:
  // TODO: override default_retries() to return 0.
};

#endif  // LESSON_SOLUTION_HPP
