// rcpp catalog, rcpp list, rcpp start
//
// Everything these print is derived from the file system, so a count in this
// repository can never drift away from what is actually on disk.

#include <iostream>

#include "atlas.hpp"
#include "catalog.hpp"
#include "commands.hpp"
#include "json.hpp"
#include "util.hpp"

namespace rcpp {
namespace {

json::Value lesson_to_json(const Lesson& lesson) {
  json::Object o;
  o.emplace("id", json::Value(lesson.id));
  o.emplace("title", json::Value(lesson.title));
  o.emplace("type", json::Value(lesson.type));
  o.emplace("path", json::Value(lesson.rel_path));
  o.emplace("minutes", json::Value(static_cast<double>(lesson.minutes)));
  o.emplace("hardware_tier", json::Value(static_cast<double>(lesson.hardware_tier)));
  o.emplace("needs_ros2", json::Value(lesson.needs_ros2));

  json::Array platforms;
  for (const std::string& p : lesson.platforms) platforms.push_back(json::Value(p));
  o.emplace("platforms", json::Value(std::move(platforms)));

  json::Array requires_ids;
  for (const std::string& r : lesson.requires_ids) requires_ids.push_back(json::Value(r));
  o.emplace("requires", json::Value(std::move(requires_ids)));

  json::Array qt;
  for (const std::string& m : lesson.qt_modules) qt.push_back(json::Value(m));
  o.emplace("qt_modules", json::Value(std::move(qt)));
  return json::Value(std::move(o));
}

std::string tier_label(int tier) {
  switch (tier) {
    case 0: return "software only";
    case 1: return "simulated";
    case 2: return "cheap hardware";
    case 3: return "real robot";
  }
  return "unknown";
}

}  // namespace

int cmd_catalog(const Args& args) {
  const auto root = find_repo_root();
  if (!root) { std::cerr << "rcpp catalog: run this from inside the repository\n"; return 2; }
  const Catalog catalog = load_catalog(*root);
  const Atlas atlas = load_atlas(*root);

  json::Object totals;
  totals.emplace("phases", json::Value(static_cast<double>(catalog.phases.size())));
  totals.emplace("lessons", json::Value(static_cast<double>(catalog.lesson_count())));
  totals.emplace("atlas_entries", json::Value(static_cast<double>(atlas.entries.size())));

  json::Array phases;
  for (const Phase& phase : catalog.phases) {
    json::Object p;
    p.emplace("num", json::Value(static_cast<double>(phase.num)));
    p.emplace("slug", json::Value(phase.slug));
    p.emplace("title", json::Value(phase.title));
    json::Array lessons;
    for (const Lesson& lesson : phase.lessons) lessons.push_back(lesson_to_json(lesson));
    p.emplace("lessons", json::Value(std::move(lessons)));
    phases.push_back(json::Value(std::move(p)));
  }

  json::Object doc;
  doc.emplace("schema", json::Value(1.0));
  doc.emplace("totals", json::Value(std::move(totals)));
  doc.emplace("phases", json::Value(std::move(phases)));
  const std::string text = json::Value(std::move(doc)).dump();

  const std::string out = flag_value(args, "--out", "");
  if (out.empty() || has_flag(args, "--stdout")) {
    std::cout << text << "\n";
    return 0;
  }
  if (!write_file(*root / out, text + "\n")) {
    std::cerr << "rcpp catalog: could not write " << out << "\n";
    return 1;
  }
  std::cout << "wrote " << out << "\n";
  return 0;
}

int cmd_list(const Args& args) {
  const auto root = find_repo_root();
  if (!root) { std::cerr << "rcpp list: run this from inside the repository\n"; return 2; }
  const Catalog catalog = load_catalog(*root);
  const std::string phase_filter = flag_value(args, "--phase", "");

  for (const Phase& phase : catalog.phases) {
    if (!phase_filter.empty() && std::to_string(phase.num) != phase_filter &&
        phase.slug.rfind(phase_filter, 0) != 0)
      continue;
    std::cout << "\n" << style::bold(phase.slug) << style::dim("  " + phase.title) << "\n";
    for (const Lesson& lesson : phase.lessons) {
      std::cout << "  " << style::pass(lesson.id) << "  " << lesson.title << "\n";
      std::cout << "      " << style::dim(std::to_string(lesson.minutes) + " min, tier " +
                                          std::to_string(lesson.hardware_tier) + " (" +
                                          tier_label(lesson.hardware_tier) + "), " +
                                          std::to_string(lesson.platforms.size()) + " toolchains")
                << "\n";
    }
  }
  std::cout << "\n" << catalog.lesson_count() << " lesson(s) in " << catalog.phases.size()
            << " phase(s)\n";
  return 0;
}

int cmd_start(const Args&) {
  const auto root = find_repo_root();
  if (!root) { std::cerr << "rcpp start: run this from inside the repository\n"; return 2; }
  const Catalog catalog = load_catalog(*root);
  if (catalog.phases.empty() || catalog.phases.front().lessons.empty()) {
    std::cerr << "rcpp start: no lessons found\n";
    return 1;
  }

  const Lesson& first = catalog.phases.front().lessons.front();
  std::cout << "\n" << style::bold("Start here") << "\n\n";
  std::cout << "  " << style::pass(first.id) << "  " << first.title << "\n";
  std::cout << "  " << style::dim(first.rel_path + "/docs/en.md") << "\n\n";
  std::cout << "  1. Read the lesson.\n";
  std::cout << "  2. Open " << first.rel_path << "/exercise and make the tests pass.\n";
  std::cout << "  3. Run: " << style::bold("rcpp verify " + first.id) << "\n\n";

  int blocked = 0;
  for (const auto* lesson : catalog.all())
    if (lesson->hardware_tier >= 2 || lesson->needs_ros2) ++blocked;
  if (blocked > 0) {
    std::cout << style::dim("  " + std::to_string(blocked) +
                            " later lesson(s) want hardware or ROS 2. Every one of them ships a\n"
                            "  software fallback, so nothing here can block you.\n\n");
  }
  return 0;
}

}  // namespace rcpp
