#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

enum class ParamType { boolean, integer, real, text };

// A value with a type. Nothing converts silently from one type to another: 1 is not 1.0.
struct Value {
  ParamType type = ParamType::integer;
  bool b = false;
  long i = 0;
  double d = 0.0;
  std::string s;

  static Value of(bool v) {
    Value x;
    x.type = ParamType::boolean;
    x.b = v;
    return x;
  }
  static Value of(long v) {
    Value x;
    x.type = ParamType::integer;
    x.i = v;
    return x;
  }
  static Value of(double v) {
    Value x;
    x.type = ParamType::real;
    x.d = v;
    return x;
  }
  static Value of(const std::string& v) {
    Value x;
    x.type = ParamType::text;
    x.s = v;
    return x;
  }
  // The number, for a range check: an integer or a real, and zero for anything else.
  double number() const { return type == ParamType::integer ? static_cast<double>(i) : (type == ParamType::real ? d : 0.0); }
};

inline bool same_value(const Value& a, const Value& b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case ParamType::boolean: return a.b == b.b;
    case ParamType::integer: return a.i == b.i;
    case ParamType::real: return a.d == b.d;
    case ParamType::text: return a.s == b.s;
  }
  return false;
}

// What is known about a parameter before anyone sets it.
struct Descriptor {
  bool read_only = false;
  bool has_range = false;      // for integers and reals
  double lowest = 0.0;
  double highest = 0.0;
};

struct SetResult {
  bool ok = true;
  std::string reason;     // why not, when not
};

struct Change {
  std::string name;
  Value before;
  Value after;
};

// The settings of a node. Each is declared with a default value, whose type is the type it will always have.
class ParameterServer {
 public:
  // A rule about the settings as a whole: given what they would be after a change, say why that is not allowed, or give an empty string.
  using Validator = std::function<std::string(const std::map<std::string, Value>&)>;

  void declare(const std::string& name, const Value& initial, const Descriptor& descriptor) {
    values_[name] = initial;
    descriptors_[name] = descriptor;
  }

  void set_validator(Validator v) { validator_ = std::move(v); }

  bool has(const std::string& name) const { return values_.count(name) != 0; }

  // The current value. A name never declared gives a default Value of type integer, so check has() first.
  Value get(const std::string& name) const {
    const auto found = values_.find(name);
    return found == values_.end() ? Value() : found->second;
  }

  // Change one setting. See set_many.
  SetResult set(const std::string& name, const Value& value) { return set_many({{name, value}}); }

  // Change several settings together: all of them or none. Each must be declared, not read only, of the declared type and inside its range;
  // then the validator is asked about the whole set of settings as they would be, and may refuse. If anything fails nothing changes and
  // the reason names the first problem. If all is well every change is applied, in the order given, and recorded.
  SetResult set_many(const std::vector<std::pair<std::string, Value>>& changes) {
    std::map<std::string, Value> proposed = values_;
    for (const auto& c : changes) {
      const auto found = values_.find(c.first);
      if (found == values_.end()) return fail(c.first + ": not declared");
      const Descriptor& d = descriptors_[c.first];
      if (d.read_only) return fail(c.first + ": read only");
      if (c.second.type != found->second.type) return fail(c.first + ": wrong type");
      if (d.has_range && (c.second.type == ParamType::integer || c.second.type == ParamType::real)) {
        const double n = c.second.number();
        if (n < d.lowest || n > d.highest) return fail(c.first + ": out of range");
      }
      proposed[c.first] = c.second;
    }
    if (validator_) {
      const std::string why = validator_(proposed);
      if (!why.empty()) return fail(why);
    }
    for (const auto& c : changes) {
      if (!same_value(values_[c.first], c.second)) log_.push_back(Change{c.first, values_[c.first], c.second});
      values_[c.first] = c.second;
    }
    return SetResult();
  }

  const std::vector<Change>& changes() const { return log_; }
  int refused() const { return refused_; }

 private:
  SetResult fail(const std::string& why) {
    ++refused_;
    SetResult r;
    r.ok = false;
    r.reason = why;
    return r;
  }
  std::map<std::string, Value> values_;
  std::map<std::string, Descriptor> descriptors_;
  Validator validator_;
  std::vector<Change> log_;
  int refused_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
