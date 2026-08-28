#include "json.hpp"

#include <cctype>
#include <cstdlib>
#include <sstream>

namespace rcpp::json {
namespace {

class Parser {
 public:
  explicit Parser(const std::string& text) : text_(text) {}

  ParseResult run() {
    skip_space();
    Value root;
    if (!parse_value(root)) return fail_result();
    skip_space();
    if (pos_ != text_.size()) {
      error("trailing characters after the top level value");
      return fail_result();
    }
    ParseResult result;
    result.ok = true;
    result.value = std::move(root);
    return result;
  }

 private:
  const std::string& text_;
  std::size_t pos_ = 0;
  std::string error_;
  int error_line_ = 0;

  int line_at(std::size_t index) const {
    int line = 1;
    for (std::size_t i = 0; i < index && i < text_.size(); ++i)
      if (text_[i] == '\n') ++line;
    return line;
  }

  bool error(const std::string& message) {
    if (error_.empty()) {
      error_ = message;
      error_line_ = line_at(pos_);
    }
    return false;
  }

  ParseResult fail_result() {
    ParseResult result;
    result.ok = false;
    result.error = error_.empty() ? "unparseable input" : error_;
    result.line = error_line_;
    return result;
  }

  bool done() const { return pos_ >= text_.size(); }
  char peek() const { return done() ? '\0' : text_[pos_]; }

  void skip_space() {
    while (!done()) {
      const char c = text_[pos_];
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        ++pos_;
      } else {
        break;
      }
    }
  }

  bool literal(const char* word) {
    const std::size_t len = std::char_traits<char>::length(word);
    if (text_.compare(pos_, len, word) != 0) return error(std::string("expected ") + word);
    pos_ += len;
    return true;
  }

  bool parse_value(Value& out) {
    skip_space();
    if (done()) return error("unexpected end of input");
    switch (peek()) {
      case '{': return parse_object(out);
      case '[': return parse_array(out);
      case '"': {
        std::string s;
        if (!parse_string(s)) return false;
        out = Value(std::move(s));
        return true;
      }
      case 't': if (!literal("true")) return false;  out = Value(true);  return true;
      case 'f': if (!literal("false")) return false; out = Value(false); return true;
      case 'n': if (!literal("null")) return false;  out = Value();      return true;
      default: return parse_number(out);
    }
  }

  bool parse_object(Value& out) {
    ++pos_;  // consume '{'
    Object object;
    skip_space();
    if (peek() == '}') { ++pos_; out = Value(std::move(object)); return true; }
    while (true) {
      skip_space();
      if (peek() != '"') return error("expected a quoted key inside an object");
      std::string key;
      if (!parse_string(key)) return false;
      skip_space();
      if (peek() != ':') return error("expected a colon after an object key");
      ++pos_;
      Value child;
      if (!parse_value(child)) return false;
      object.emplace(std::move(key), std::move(child));
      skip_space();
      if (peek() == ',') { ++pos_; continue; }
      if (peek() == '}') { ++pos_; out = Value(std::move(object)); return true; }
      return error("expected a comma or a closing brace inside an object");
    }
  }

  bool parse_array(Value& out) {
    ++pos_;  // consume '['
    Array array;
    skip_space();
    if (peek() == ']') { ++pos_; out = Value(std::move(array)); return true; }
    while (true) {
      Value child;
      if (!parse_value(child)) return false;
      array.push_back(std::move(child));
      skip_space();
      if (peek() == ',') { ++pos_; continue; }
      if (peek() == ']') { ++pos_; out = Value(std::move(array)); return true; }
      return error("expected a comma or a closing bracket inside an array");
    }
  }

  bool parse_string(std::string& out) {
    ++pos_;  // consume the opening quote
    out.clear();
    while (true) {
      if (done()) return error("unterminated string");
      const char c = text_[pos_++];
      if (c == '"') return true;
      if (c != '\\') { out.push_back(c); continue; }
      if (done()) return error("unterminated escape sequence");
      const char esc = text_[pos_++];
      switch (esc) {
        case '"':  out.push_back('"');  break;
        case '\\': out.push_back('\\'); break;
        case '/':  out.push_back('/');  break;
        case 'b':  out.push_back('\b'); break;
        case 'f':  out.push_back('\f'); break;
        case 'n':  out.push_back('\n'); break;
        case 'r':  out.push_back('\r'); break;
        case 't':  out.push_back('\t'); break;
        case 'u': {
          if (pos_ + 4 > text_.size()) return error("truncated \\u escape");
          const std::string hex = text_.substr(pos_, 4);
          pos_ += 4;
          const unsigned code = std::strtoul(hex.c_str(), nullptr, 16);
          if (code < 0x80) {
            out.push_back(static_cast<char>(code));
          } else if (code < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
          } else {
            out.push_back(static_cast<char>(0xE0 | (code >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
          }
          break;
        }
        default: return error("unknown escape sequence");
      }
    }
  }

  bool parse_number(Value& out) {
    const std::size_t start = pos_;
    if (peek() == '-' || peek() == '+') ++pos_;
    bool digits = false;
    while (!done() && std::isdigit(static_cast<unsigned char>(peek()))) { ++pos_; digits = true; }
    if (!done() && peek() == '.') {
      ++pos_;
      while (!done() && std::isdigit(static_cast<unsigned char>(peek()))) { ++pos_; digits = true; }
    }
    if (!digits) return error("expected a value");
    if (!done() && (peek() == 'e' || peek() == 'E')) {
      ++pos_;
      if (!done() && (peek() == '-' || peek() == '+')) ++pos_;
      while (!done() && std::isdigit(static_cast<unsigned char>(peek()))) ++pos_;
    }
    out = Value(std::strtod(text_.substr(start, pos_ - start).c_str(), nullptr));
    return true;
  }
};

}  // namespace

std::string escape(const std::string& raw) {
  std::string out;
  out.reserve(raw.size() + 8);
  for (const char c : raw) {
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buffer[8];
          std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
          out += buffer;
        } else {
          out.push_back(c);
        }
    }
  }
  return out;
}

std::string Value::dump(int indent, int depth) const {
  // An indent of zero means fully compact, with no newlines at all. A GitHub
  // Actions job output has to fit on one line, so this is not merely a
  // formatting preference.
  const bool compact = indent <= 0;
  const std::string newline = compact ? "" : "\n";
  const std::string pad = compact ? "" : std::string(static_cast<std::size_t>(indent * (depth + 1)), ' ');
  const std::string pad_close = compact ? "" : std::string(static_cast<std::size_t>(indent * depth), ' ');
  std::ostringstream out;
  switch (kind_) {
    case Kind::Null: return "null";
    case Kind::Bool: return bool_ ? "true" : "false";
    case Kind::Number: {
      if (number_ == static_cast<long long>(number_))
        out << static_cast<long long>(number_);
      else
        out << number_;
      return out.str();
    }
    case Kind::String: return "\"" + escape(string_) + "\"";
    case Kind::Array: {
      if (array_.empty()) return "[]";
      out << "[" << newline;
      for (std::size_t i = 0; i < array_.size(); ++i) {
        out << pad << array_[i].dump(indent, depth + 1);
        if (i + 1 < array_.size()) out << ",";
        out << newline;
      }
      out << pad_close << "]";
      return out.str();
    }
    case Kind::Object: {
      if (object_.empty()) return "{}";
      out << "{" << newline;
      std::size_t i = 0;
      for (const auto& [key, value] : object_) {
        out << pad << "\"" << escape(key) << "\":" << (compact ? "" : " ")
            << value.dump(indent, depth + 1);
        if (++i < object_.size()) out << ",";
        out << newline;
      }
      out << pad_close << "}";
      return out.str();
    }
  }
  return "null";
}

ParseResult parse(const std::string& text) { return Parser(text).run(); }

}  // namespace rcpp::json
