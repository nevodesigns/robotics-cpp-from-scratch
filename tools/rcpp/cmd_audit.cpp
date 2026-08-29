// rcpp audit
//
// Enforces the lesson contract. The rule of this repository is that the
// contract and the checker are the same thing: if a rule is written in
// docs/CONTRACT.md it has a numbered check here, and if it has no check here it
// is not a rule. That is what stops a curriculum from drifting away from its
// own documentation as it grows.

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>

#include "atlas.hpp"
#include "catalog.hpp"
#include "commands.hpp"
#include "json.hpp"
#include "util.hpp"

namespace rcpp {
namespace {

struct Finding {
  std::string rule;
  std::string where;
  std::string message;
};

// The toolchains and what each provides, read from platforms.json so that the
// auditor and the continuous integration matrix cannot disagree about them.
struct Toolchain {
  std::string id;
  bool has_qt = false;
};

std::vector<Toolchain> load_toolchains(const fs::path& repo_root, std::vector<Finding>& out) {
  std::vector<Toolchain> toolchains;
  const auto text = read_file(repo_root / "platforms.json");
  if (!text) {
    out.push_back({"L019", "platforms.json", "missing, and it is the source of truth for toolchains"});
    return toolchains;
  }
  const json::ParseResult parsed = json::parse(*text);
  if (!parsed.ok) {
    out.push_back({"L019", "platforms.json",
                   "line " + std::to_string(parsed.line) + ": " + parsed.error});
    return toolchains;
  }
  for (const json::Value& entry : parsed.value.at("toolchains").as_array()) {
    Toolchain toolchain;
    toolchain.id = entry.at("id").as_string_or("");
    toolchain.has_qt = entry.at("qt").as_bool(false);
    if (toolchain.id.empty())
      out.push_back({"L019", "platforms.json", "a toolchain entry has no id"});
    else
      toolchains.push_back(toolchain);
  }
  return toolchains;
}

const Toolchain* find_toolchain(const std::vector<Toolchain>& toolchains, const std::string& id) {
  for (const Toolchain& toolchain : toolchains)
    if (toolchain.id == id) return &toolchain;
  return nullptr;
}

const std::vector<std::string>& required_sections() {
  static const std::vector<std::string> sections = {
      "## The Problem", "## The Concept",        "## Build It",
      "## Use It",      "## What Breaks First",  "## Ship It"};
  return sections;
}

bool is_allowed_source(const fs::path& file) {
  static const std::set<std::string> allowed = {
      ".cpp", ".hpp", ".h", ".cc", ".md", ".json", ".cmake", ".qml", ".ui", ".svg", ".csv"};
  if (file.filename() == "CMakeLists.txt") return true;
  if (file.filename() == ".gitkeep") return true;
  return allowed.count(file.extension().string()) > 0;
}

// The em dash is banned across the whole repository, so the checker owns the
// rule rather than a habit.
bool has_em_dash(const std::string& text, int& line_number) {
  int line = 1;
  for (std::size_t i = 0; i + 2 < text.size(); ++i) {
    if (text[i] == '\n') ++line;
    const unsigned char a = static_cast<unsigned char>(text[i]);
    const unsigned char b = static_cast<unsigned char>(text[i + 1]);
    const unsigned char c = static_cast<unsigned char>(text[i + 2]);
    if (a == 0xE2 && b == 0x80 && c == 0x94) { line_number = line; return true; }
  }
  return false;
}

void check_manifest(const Lesson& lesson, std::vector<Finding>& out) {
  const std::string where = lesson.rel_path;
  if (lesson.id.empty()) out.push_back({"L003", where, "lesson.json has no id"});
  if (lesson.title.empty()) out.push_back({"L003", where, "lesson.json has no title"});
  if (lesson.minutes <= 0) out.push_back({"L003", where, "lesson.json needs a positive minutes value"});

  static const std::set<std::string> types = {"learn", "build", "capstone"};
  if (!types.count(lesson.type))
    out.push_back({"L003", where, "type must be learn, build or capstone, found: " + lesson.type});

  char expected_prefix[8];
  std::snprintf(expected_prefix, sizeof(expected_prefix), "%02d-%02d-", lesson.phase_num, lesson.lesson_num);
  if (!starts_with(lesson.id, expected_prefix))
    out.push_back({"L004", where, "id should start with " + std::string(expected_prefix) +
                                      " to match its position on disk, found: " + lesson.id});
  if (lesson.id != std::string(expected_prefix) + lesson.dir_slug.substr(3))
    out.push_back({"L004", where, "id should be " + std::string(expected_prefix) + lesson.dir_slug.substr(3)});

  if (lesson.platforms.empty())
    out.push_back({"L011", where, "platforms must list at least one toolchain"});

  if (lesson.hardware_tier < 0 || lesson.hardware_tier > 3)
    out.push_back({"L003", where, "hardware_tier must be 0, 1, 2 or 3"});
}

void check_docs(const Lesson& lesson, std::vector<Finding>& out) {
  const fs::path doc = lesson.path / "docs" / "en.md";
  const auto text = read_file(doc);
  if (!text) { out.push_back({"L005", lesson.rel_path, "missing docs/en.md"}); return; }
  if (trim(*text).empty()) { out.push_back({"L005", lesson.rel_path, "docs/en.md is empty"}); return; }

  const std::vector<std::string> lines = split(*text, '\n');
  if (lines.empty() || !starts_with(lines[0], "# "))
    out.push_back({"L005", lesson.rel_path, "docs/en.md must open with a level one heading"});

  bool motto = false;
  for (std::size_t i = 1; i < lines.size() && i < 6; ++i)
    if (starts_with(trim(lines[i]), "> ")) motto = true;
  if (!motto)
    out.push_back({"L005", lesson.rel_path, "docs/en.md needs a one line motto as a block quote near the top"});

  for (const std::string& section : required_sections())
    if (!contains(*text, "\n" + section))
      out.push_back({"L006", lesson.rel_path, "docs/en.md is missing the section: " + section});
}

void check_layout(const Lesson& lesson, std::vector<Finding>& out) {
  // A lesson registers itself with exactly one line of CMake at its root. The
  // helper reads lesson.json and builds both variants, so nothing else should
  // appear here and no lesson should hand roll its own targets.
  const auto lesson_cmake = read_file(lesson.path / "CMakeLists.txt");
  if (!lesson_cmake) {
    out.push_back({"L007", lesson.rel_path, "missing CMakeLists.txt at the lesson root"});
  } else if (!contains(*lesson_cmake, "rc_add_lesson()")) {
    out.push_back({"L007", lesson.rel_path,
                   "CMakeLists.txt must call rc_add_lesson(), which builds both variants "
                   "from lesson.json"});
  }

  bool exercise_source = false;
  if (fs::is_directory(lesson.path / "exercise"))
    for (const auto& e : fs::directory_iterator(lesson.path / "exercise"))
      if (e.is_regular_file() && (e.path().extension() == ".cpp" || e.path().extension() == ".hpp"))
        exercise_source = true;
  if (!exercise_source)
    out.push_back({"L007", lesson.rel_path, "exercise/ has no C++ source for the learner to complete"});

  bool reference_source = false;
  if (fs::is_directory(lesson.path / "reference"))
    for (const auto& e : fs::directory_iterator(lesson.path / "reference"))
      if (e.is_regular_file() && (e.path().extension() == ".cpp" || e.path().extension() == ".hpp"))
        reference_source = true;
  if (!reference_source)
    out.push_back({"L008", lesson.rel_path, "reference/ has no worked implementation"});

  bool tests = false;
  if (fs::is_directory(lesson.path / "tests"))
    for (const auto& e : fs::directory_iterator(lesson.path / "tests"))
      if (e.is_regular_file() && e.path().extension() == ".cpp") tests = true;
  if (!tests)
    out.push_back({"L009", lesson.rel_path, "tests/ has no test file, every lesson ships a failing suite"});

  if (lesson.hardware_tier >= 2) {
    const bool has_fixture = fs::is_directory(lesson.path / "fixtures") ||
                             lesson.raw.at("fallback").is_string();
    if (!has_fixture)
      out.push_back({"L014", lesson.rel_path,
                     "a tier 2 or 3 lesson must declare a fallback, either a fixtures/ directory or a fallback field"});
  }
}

void check_quiz(const Lesson& lesson, std::vector<Finding>& out) {
  const fs::path quiz_path = lesson.path / "quiz.json";
  const auto text = read_file(quiz_path);
  if (!text) { out.push_back({"L010", lesson.rel_path, "missing quiz.json"}); return; }
  const json::ParseResult parsed = json::parse(*text);
  if (!parsed.ok) {
    out.push_back({"L010", lesson.rel_path,
                   "quiz.json line " + std::to_string(parsed.line) + ": " + parsed.error});
    return;
  }
  const json::Value& questions = parsed.value.at("questions");
  if (!questions.is_array()) {
    out.push_back({"L010", lesson.rel_path, "quiz.json needs a questions array"});
    return;
  }
  if (questions.as_array().size() != 6) {
    out.push_back({"L010", lesson.rel_path,
                   "quiz.json must hold exactly 6 questions, found " +
                       std::to_string(questions.as_array().size())});
  }

  std::map<std::string, int> stages;
  int index = 0;
  for (const json::Value& q : questions.as_array()) {
    const std::string at = "question[" + std::to_string(index++) + "]";
    const std::string stage = q.at("stage").as_string_or("");
    if (stage != "pre" && stage != "check" && stage != "post")
      out.push_back({"L010", lesson.rel_path, at + " stage must be pre, check or post"});
    else
      ++stages[stage];

    if (q.at("question").as_string_or("").empty())
      out.push_back({"L010", lesson.rel_path, at + " has no question text"});
    if (q.at("explanation").as_string_or("").empty())
      out.push_back({"L010", lesson.rel_path, at + " has no explanation"});

    const json::Value& options = q.at("options");
    if (!options.is_array() || options.as_array().size() != 4) {
      out.push_back({"L010", lesson.rel_path, at + " must offer exactly 4 options"});
      continue;
    }
    const json::Value& correct = q.at("correct");
    if (!correct.is_number() || correct.as_int(-1) < 0 || correct.as_int(-1) > 3)
      out.push_back({"L010", lesson.rel_path, at + " correct must be an index from 0 to 3"});
  }

  if (questions.as_array().size() == 6 &&
      (stages["pre"] != 1 || stages["check"] != 3 || stages["post"] != 2)) {
    out.push_back({"L010", lesson.rel_path,
                   "quiz.json stages must be 1 pre, 3 check and 2 post"});
  }
}

// Looking at code rather than at text.
//
// Rules L017 and L018 search for things that must not appear in a lesson. A
// plain substring search finds them in comments and in prose too, so a lesson
// that explains why the register keyword was removed gets reported for using
// it. A rule that cries wolf is a rule people learn to ignore, so the search
// removes comments and string literals first, then requires a whole token.
std::string code_only(const std::string& text) {
  std::string out;
  out.reserve(text.size());

  enum class In { Code, LineComment, BlockComment, String, Char };
  In state = In::Code;

  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    const char next = (i + 1 < text.size()) ? text[i + 1] : '\0';

    switch (state) {
      case In::Code:
        if (c == '/' && next == '/') { state = In::LineComment; ++i; out += "  "; }
        else if (c == '/' && next == '*') { state = In::BlockComment; ++i; out += "  "; }
        else if (c == '"') { state = In::String; out += ' '; }
        else if (c == '\'') { state = In::Char; out += ' '; }
        else out += c;
        break;
      case In::LineComment:
        if (c == '\n') { state = In::Code; out += '\n'; }
        else out += ' ';
        break;
      case In::BlockComment:
        if (c == '*' && next == '/') { state = In::Code; ++i; out += "  "; }
        else out += (c == '\n') ? '\n' : ' ';
        break;
      case In::String:
        if (c == '\\') { ++i; out += "  "; }
        else if (c == '"') { state = In::Code; out += ' '; }
        else out += ' ';
        break;
      case In::Char:
        if (c == '\\') { ++i; out += "  "; }
        else if (c == '\'') { state = In::Code; out += ' '; }
        else out += ' ';
        break;
    }
  }
  return out;
}

bool identifier_char(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

// A whole token match for identifier-like needles, so NULL does not match
// NULLABLE and register does not match registered. Anything with punctuation in
// it, such as malloc( or std::span, is matched as a plain substring.
bool contains_code_token(const std::string& code, const std::string& needle) {
  if (needle.empty()) return false;
  const bool identifier_like = identifier_char(needle.front()) && identifier_char(needle.back());

  std::size_t at = code.find(needle);
  while (at != std::string::npos) {
    if (!identifier_like) return true;
    const bool left_ok = (at == 0) || !identifier_char(code[at - 1]);
    const std::size_t after = at + needle.size();
    const bool right_ok = (after >= code.size()) || !identifier_char(code[after]);
    if (left_ok && right_ok) return true;
    at = code.find(needle, at + 1);
  }
  return false;
}

// L017: the curriculum is written in C++17, and that is a teaching decision
// rather than an accident. Facilities from C++20 and C++23 are reached through
// rc/core/compat.hpp, which the learner reads and understands, so a lesson that
// uses them directly would quietly raise the compiler requirement and skip the
// part that does the teaching.
void check_cxx17_baseline(const Lesson& lesson, std::vector<Finding>& out) {
  struct Later {
    const char* token;
    const char* standard;
    const char* instead;
  };
  static const std::vector<Later> later_facilities = {
      {"std::span", "C++20", "rc::span from rc/core/compat.hpp"},
      {"#include <span>", "C++20", "rc/core/compat.hpp"},
      {"std::format", "C++20", "rc::format from rc/core/compat.hpp"},
      {"#include <format>", "C++20", "rc/core/compat.hpp"},
      {"std::expected", "C++23", "rc::expected from rc/core/compat.hpp"},
      {"#include <expected>", "C++23", "rc/core/compat.hpp"},
      {"std::print", "C++23", "std::cout or rc::format"},
      {"#include <ranges>", "C++20", "the algorithms in <algorithm>"},
      {"std::ranges::", "C++20", "the algorithms in <algorithm>"},
      {"co_await", "C++20", "a plain function or a thread"},
      {"co_return", "C++20", "a plain return"},
      {"consteval", "C++20", "constexpr"},
      {"<=>", "C++20", "the individual comparison operators"},
  };

  for (const auto& entry : fs::recursive_directory_iterator(lesson.path)) {
    if (!entry.is_regular_file()) continue;
    const std::string ext = entry.path().extension().string();
    if (ext != ".cpp" && ext != ".hpp" && ext != ".h") continue;
    const auto text = read_file(entry.path());
    if (!text) continue;
    const std::string code = code_only(*text);
    const std::string where = fs::relative(entry.path(), lesson.path).string();
    for (const Later& later : later_facilities) {
      if (contains_code_token(code, later.token)) {
        out.push_back({"L017", lesson.rel_path,
                       where + " uses " + later.token + ", which is " + later.standard +
                           ". This curriculum is C++17: use " + later.instead});
      }
    }
  }

  if (lesson.cxx_standard != 17)
    out.push_back({"L017", lesson.rel_path,
                   "cxx_standard must be 17, found " + std::to_string(lesson.cxx_standard)});
}

// L018: teach the C++ people write now, not the C++ that survived from 1998.
// A learner meeting NULL or malloc in a lesson will reasonably assume it is
// current practice, and will carry it into their own code for years.
void check_no_discarded_style(const Lesson& lesson, std::vector<Finding>& out) {
  struct Discarded {
    const char* token;
    const char* instead;
  };
  static const std::vector<Discarded> discarded = {
      {"NULL", "nullptr"},
      {"malloc(", "a container, or std::make_unique"},
      {"calloc(", "a container, or std::make_unique"},
      {"realloc(", "a container that grows, such as std::vector"},
      {"strcpy(", "std::string"},
      {"strcat(", "std::string"},
      {"sprintf(", "rc::format"},
      {"printf(", "std::cout, or rc::format"},
      {"using namespace std;", "the std:: prefix, written out"},
      {"typedef", "using, which reads left to right"},
      {"register", "nothing, the keyword was removed"},
      {"throw()", "noexcept"},
  };

  for (const auto& entry : fs::recursive_directory_iterator(lesson.path)) {
    if (!entry.is_regular_file()) continue;
    const std::string ext = entry.path().extension().string();
    if (ext != ".cpp" && ext != ".hpp" && ext != ".h") continue;
    const auto text = read_file(entry.path());
    if (!text) continue;
    const std::string code = code_only(*text);
    const std::string where = fs::relative(entry.path(), lesson.path).string();
    for (const Discarded& old : discarded) {
      if (contains_code_token(code, old.token)) {
        out.push_back({"L018", lesson.rel_path,
                       where + " uses " + old.token + ", which modern C++ has left behind: use " +
                           old.instead});
      }
    }
  }
}

// L019: a platform claim must be backed by a lane that can actually prove it.
//
// The gap this closes is subtle and dangerous. A Qt lesson claiming a toolchain
// whose lane has no Qt is not built there at all: rc_add_lesson skips it,
// continuous integration reports success, and the claim looks satisfied while
// nothing was ever compiled. A silent skip is indistinguishable from a pass,
// which is the worst failure mode a checker can have.
void check_platform_claims(const Lesson& lesson, const std::vector<Toolchain>& toolchains,
                           std::vector<Finding>& out) {
  if (toolchains.empty()) return;   // already reported against platforms.json

  for (const std::string& claimed : lesson.platforms) {
    const Toolchain* toolchain = find_toolchain(toolchains, claimed);
    if (toolchain == nullptr) {
      out.push_back({"L019", lesson.rel_path,
                     "claims " + claimed + ", which platforms.json does not define"});
      continue;
    }
    if (!lesson.qt_modules.empty() && !toolchain->has_qt) {
      out.push_back({"L019", lesson.rel_path,
                     "needs Qt but claims " + claimed +
                         ", whose lane has no Qt. The lesson would be skipped there, "
                         "and a skip is indistinguishable from a pass"});
    }
  }
}

void check_one_language(const Lesson& lesson, std::vector<Finding>& out) {
  for (const auto& entry : fs::recursive_directory_iterator(lesson.path)) {
    if (!entry.is_regular_file()) continue;
    if (!is_allowed_source(entry.path())) {
      out.push_back({"L015", lesson.rel_path,
                     "only C++, CMake, markdown and data files belong in a lesson, found: " +
                         fs::relative(entry.path(), lesson.path).string()});
    }
  }
}

void check_em_dash(const fs::path& root, std::vector<Finding>& out) {
  static const std::set<std::string> scanned = {".md", ".json", ".cpp", ".hpp", ".h", ".cmake", ".txt"};
  for (const auto& entry : fs::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) continue;
    const std::string path = entry.path().string();
    if (contains(path, "/.git/") || contains(path, "/build/")) continue;
    if (!scanned.count(entry.path().extension().string())) continue;
    const auto text = read_file(entry.path());
    if (!text) continue;
    int line = 0;
    if (has_em_dash(*text, line))
      out.push_back({"L016", fs::relative(entry.path(), root).string(),
                     "em dash found on line " + std::to_string(line) + ", use a comma, a colon or a full stop"});
  }
}

void check_graph(const Catalog& catalog, std::vector<Finding>& out) {
  std::set<std::string> ids;
  for (const auto* lesson : catalog.all()) {
    if (!ids.insert(lesson->id).second)
      out.push_back({"L004", lesson->rel_path, "duplicate lesson id: " + lesson->id});
  }
  for (const auto* lesson : catalog.all())
    for (const std::string& need : lesson->requires_ids)
      if (!ids.count(need))
        out.push_back({"L012", lesson->rel_path, "requires an unknown lesson: " + need});

  // A depth first walk detects a cycle in the prerequisite graph.
  std::map<std::string, std::vector<std::string>> edges;
  for (const auto* lesson : catalog.all()) edges[lesson->id] = lesson->requires_ids;
  std::map<std::string, int> state;  // 0 unseen, 1 on the stack, 2 finished
  std::vector<std::string> stack;

  std::function<void(const std::string&)> walk = [&](const std::string& id) {
    if (state[id] == 2) return;
    if (state[id] == 1) {
      out.push_back({"L012", id, "prerequisite cycle involving " + id});
      return;
    }
    state[id] = 1;
    for (const std::string& next : edges[id])
      if (edges.count(next)) walk(next);
    state[id] = 2;
  };
  for (const auto& [id, _] : edges) walk(id);
}

void check_atlas_links(const Catalog& catalog, const Atlas& atlas, std::vector<Finding>& out) {
  for (const auto* lesson : catalog.all()) {
    if (lesson->breaks_first.empty())
      out.push_back({"L013", lesson->rel_path,
                     "breaks_first must name at least one atlas entry, that section is mandatory"});
    for (const std::string& id : lesson->breaks_first)
      if (!atlas.find(id))
        out.push_back({"L013", lesson->rel_path, "breaks_first names an atlas entry that does not exist: " + id});
  }
  for (const AtlasEntry& entry : atlas.entries) {
    for (const std::string& pattern : entry.patterns) {
      try {
        const std::regex probe(pattern, std::regex::ECMAScript | std::regex::icase);
        (void)probe;
      } catch (const std::regex_error& e) {
        out.push_back({"L013", "atlas/errors/" + entry.path.filename().string(),
                       std::string("match pattern does not compile: ") + e.what()});
      }
    }
    if (!entry.teaches.empty() && !catalog.find(entry.teaches))
      out.push_back({"L013", "atlas/errors/" + entry.path.filename().string(),
                     "teaches names a lesson that does not exist: " + entry.teaches});
    if (entry.symptom.empty() || entry.cause.empty() || entry.fix.empty())
      out.push_back({"L013", "atlas/errors/" + entry.path.filename().string(),
                     "every entry needs a Symptom, a Cause and a Fix section"});
  }
}

}  // namespace

int cmd_audit(const Args& args) {
  const auto root = find_repo_root();
  if (!root) {
    std::cerr << "rcpp audit: run this from inside the repository\n";
    return 2;
  }
  const bool as_json = has_flag(args, "--json");
  const Catalog catalog = load_catalog(*root);
  const Atlas atlas = load_atlas(*root);

  std::vector<Finding> findings;
  const std::vector<Toolchain> toolchains = load_toolchains(*root, findings);
  for (const std::string& e : catalog.load_errors) findings.push_back({"L001", "phases", e});
  for (const std::string& e : atlas.load_errors) findings.push_back({"L013", "atlas", e});

  for (const auto* lesson : catalog.all()) {
    check_manifest(*lesson, findings);
    check_docs(*lesson, findings);
    check_layout(*lesson, findings);
    check_quiz(*lesson, findings);
    check_one_language(*lesson, findings);
    check_cxx17_baseline(*lesson, findings);
    check_platform_claims(*lesson, toolchains, findings);
    check_no_discarded_style(*lesson, findings);
  }
  check_graph(catalog, findings);
  check_atlas_links(catalog, atlas, findings);
  check_em_dash(*root, findings);

  if (as_json) {
    json::Object root_obj;
    root_obj.emplace("lessons", json::Value(static_cast<double>(catalog.lesson_count())));
    root_obj.emplace("atlas_entries", json::Value(static_cast<double>(atlas.entries.size())));
    json::Array items;
    for (const Finding& f : findings) {
      json::Object item;
      item.emplace("rule", json::Value(f.rule));
      item.emplace("where", json::Value(f.where));
      item.emplace("message", json::Value(f.message));
      items.push_back(json::Value(std::move(item)));
    }
    root_obj.emplace("findings", json::Value(std::move(items)));
    std::cout << json::Value(std::move(root_obj)).dump() << "\n";
    return findings.empty() ? 0 : 1;
  }

  for (const Finding& f : findings)
    std::cout << style::fail(f.rule) << "  " << f.where << "\n        " << f.message << "\n";

  std::cout << "\nrcpp audit: " << catalog.lesson_count() << " lesson(s), "
            << atlas.entries.size() << " atlas entrie(s), " << findings.size() << " issue(s)\n";
  return findings.empty() ? 0 : 1;
}

}  // namespace rcpp
