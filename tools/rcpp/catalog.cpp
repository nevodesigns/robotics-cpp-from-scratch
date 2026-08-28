#include "catalog.hpp"

#include <algorithm>
#include <cctype>
#include <map>

namespace rcpp {
namespace {

bool leading_number(const std::string& name, int& out) {
  if (name.size() < 2) return false;
  if (!std::isdigit(static_cast<unsigned char>(name[0]))) return false;
  if (!std::isdigit(static_cast<unsigned char>(name[1]))) return false;
  out = (name[0] - '0') * 10 + (name[1] - '0');
  return name.size() > 2 && name[2] == '-';
}

}  // namespace

std::string phase_title_from_slug(const std::string& slug) {
  const std::size_t dash = slug.find('-');
  const std::string body = dash == std::string::npos ? slug : slug.substr(dash + 1);

  // A phase title is read by a learner, so it is worth spelling properly.
  // Words that are acronyms or that should stay lowercase are listed rather
  // than guessed at, because there are few of them and guessing reads worse
  // than a short table.
  static const std::map<std::string, std::string> spellings = {
      {"cpp", "C++"},   {"qt", "Qt"},     {"raii", "RAII"}, {"ros", "ROS 2"},
      {"i", "I"},       {"ii", "II"},     {"iii", "III"},   {"iv", "IV"},
      {"and", "and"},   {"the", "the"},   {"of", "of"},     {"for", "for"},
      {"in", "in"},     {"to", "to"},     {"a", "a"},       {"with", "with"},
  };

  std::string title;
  bool first_word = true;
  for (const std::string& raw : split(body, '-')) {
    if (raw.empty()) continue;
    if (!title.empty()) title += ' ';

    const auto known = spellings.find(raw);
    if (known != spellings.end() && !(first_word && known->second == raw)) {
      title += known->second;
    } else {
      std::string word = raw;
      word[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(word[0])));
      title += word;
    }
    first_word = false;
  }
  return title;
}

std::string Lesson::target_name() const {
  std::string name = "lesson_" + id;
  for (char& c : name)
    if (!std::isalnum(static_cast<unsigned char>(c))) c = '_';
  return name;
}

int Catalog::lesson_count() const {
  int total = 0;
  for (const Phase& p : phases) total += static_cast<int>(p.lessons.size());
  return total;
}

const Lesson* Catalog::find(const std::string& id) const {
  for (const Phase& p : phases)
    for (const Lesson& l : p.lessons) {
      if (l.id == id) return &l;
      // Allow the short form: "14-01" matches "14-01-pid-from-scratch".
      if (starts_with(l.id, id + "-")) return &l;
    }
  return nullptr;
}

std::vector<const Lesson*> Catalog::all() const {
  std::vector<const Lesson*> out;
  for (const Phase& p : phases)
    for (const Lesson& l : p.lessons) out.push_back(&l);
  return out;
}

Catalog load_catalog(const fs::path& repo_root) {
  Catalog catalog;
  catalog.root = repo_root;
  const fs::path phases_dir = repo_root / "phases";
  if (!fs::is_directory(phases_dir)) {
    catalog.load_errors.push_back("no phases/ directory under " + repo_root.string());
    return catalog;
  }

  std::vector<fs::path> phase_dirs;
  for (const auto& entry : fs::directory_iterator(phases_dir))
    if (entry.is_directory()) phase_dirs.push_back(entry.path());
  std::sort(phase_dirs.begin(), phase_dirs.end());

  for (const fs::path& phase_path : phase_dirs) {
    const std::string phase_slug = phase_path.filename().string();
    int phase_num = 0;
    if (!leading_number(phase_slug, phase_num)) {
      catalog.load_errors.push_back("phase directory is not named NN-slug: " + phase_slug);
      continue;
    }

    Phase phase;
    phase.num = phase_num;
    phase.slug = phase_slug;
    phase.title = phase_title_from_slug(phase_slug);

    std::vector<fs::path> lesson_dirs;
    for (const auto& entry : fs::directory_iterator(phase_path))
      if (entry.is_directory()) lesson_dirs.push_back(entry.path());
    std::sort(lesson_dirs.begin(), lesson_dirs.end());

    for (const fs::path& lesson_path : lesson_dirs) {
      const std::string dir_slug = lesson_path.filename().string();
      int lesson_num = 0;
      if (!leading_number(dir_slug, lesson_num)) {
        catalog.load_errors.push_back("lesson directory is not named NN-slug: " +
                                      phase_slug + "/" + dir_slug);
        continue;
      }

      const fs::path manifest = lesson_path / "lesson.json";
      const auto text = read_file(manifest);
      if (!text) {
        catalog.load_errors.push_back("missing lesson.json: " + phase_slug + "/" + dir_slug);
        continue;
      }
      const json::ParseResult parsed = json::parse(*text);
      if (!parsed.ok) {
        catalog.load_errors.push_back("lesson.json line " + std::to_string(parsed.line) +
                                      ": " + parsed.error + " (" + phase_slug + "/" + dir_slug + ")");
        continue;
      }

      Lesson lesson;
      lesson.raw = parsed.value;
      lesson.path = lesson_path;
      lesson.dir_slug = dir_slug;
      lesson.rel_path = "phases/" + phase_slug + "/" + dir_slug;
      lesson.phase_num = phase_num;
      lesson.lesson_num = lesson_num;
      lesson.id = parsed.value.at("id").as_string_or("");
      lesson.title = parsed.value.at("title").as_string_or("");
      lesson.type = parsed.value.at("type").as_string_or("");
      lesson.minutes = parsed.value.at("minutes").as_int(0);
      lesson.hardware_tier = parsed.value.at("hardware_tier").as_int(0);
      lesson.cxx_standard = parsed.value.at("cxx_standard").as_int(20);
      lesson.platforms = parsed.value.at("platforms").string_list();
      lesson.requires_ids = parsed.value.at("requires").string_list();
      lesson.breaks_first = parsed.value.at("breaks_first").string_list();
      if (parsed.value.at("qt").is_object())
        lesson.qt_modules = parsed.value.at("qt").at("modules").string_list();
      lesson.needs_ros2 = !parsed.value.at("ros2").is_null();

      phase.lessons.push_back(std::move(lesson));
    }
    catalog.phases.push_back(std::move(phase));
  }
  return catalog;
}

}  // namespace rcpp
