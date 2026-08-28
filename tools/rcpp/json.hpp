// tools/rcpp/json.hpp
//
// A small JSON reader and writer. The repository refuses to depend on a
// package manager for its own tooling, so this is hand written, about three
// hundred lines, and readable in one sitting. It supports the whole of JSON
// except for surrogate pair escapes, which no file in this repository uses.

#ifndef RCPP_JSON_HPP
#define RCPP_JSON_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace rcpp::json {

class Value;

using Array = std::vector<Value>;
using Object = std::map<std::string, Value>;

enum class Kind { Null, Bool, Number, String, Array, Object };

class Value {
 public:
  Value() = default;
  Value(bool b) : kind_(Kind::Bool), bool_(b) {}
  Value(double n) : kind_(Kind::Number), number_(n) {}
  Value(std::string s) : kind_(Kind::String), string_(std::move(s)) {}
  Value(Array a) : kind_(Kind::Array), array_(std::move(a)) {}
  Value(Object o) : kind_(Kind::Object), object_(std::move(o)) {}

  Kind kind() const { return kind_; }
  bool is_null() const { return kind_ == Kind::Null; }
  bool is_bool() const { return kind_ == Kind::Bool; }
  bool is_number() const { return kind_ == Kind::Number; }
  bool is_string() const { return kind_ == Kind::String; }
  bool is_array() const { return kind_ == Kind::Array; }
  bool is_object() const { return kind_ == Kind::Object; }

  bool as_bool(bool fallback = false) const {
    return is_bool() ? bool_ : fallback;
  }
  double as_number(double fallback = 0.0) const {
    return is_number() ? number_ : fallback;
  }
  int as_int(int fallback = 0) const {
    return is_number() ? static_cast<int>(number_) : fallback;
  }
  const std::string& as_string() const { return string_; }
  std::string as_string_or(const std::string& fallback) const {
    return is_string() ? string_ : fallback;
  }
  const Array& as_array() const { return array_; }
  const Object& as_object() const { return object_; }

  // Returns a null Value when the key is absent, so callers can chain without
  // checking at every step.
  const Value& at(const std::string& key) const {
    static const Value none;
    if (!is_object()) return none;
    const auto it = object_.find(key);
    return it == object_.end() ? none : it->second;
  }
  bool has(const std::string& key) const {
    return is_object() && object_.count(key) > 0;
  }

  std::vector<std::string> string_list() const {
    std::vector<std::string> out;
    if (!is_array()) return out;
    for (const Value& v : array_)
      if (v.is_string()) out.push_back(v.as_string());
    return out;
  }

  std::string dump(int indent = 2, int depth = 0) const;

 private:
  Kind kind_ = Kind::Null;
  bool bool_ = false;
  double number_ = 0.0;
  std::string string_;
  Array array_;
  Object object_;
};

struct ParseResult {
  bool ok = false;
  Value value;
  std::string error;
  int line = 0;
};

ParseResult parse(const std::string& text);
std::string escape(const std::string& raw);

}  // namespace rcpp::json

#endif  // RCPP_JSON_HPP
