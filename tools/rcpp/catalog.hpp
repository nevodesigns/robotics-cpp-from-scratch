// tools/rcpp/catalog.hpp
//
// The in memory model of the curriculum. Everything is read from the file
// system and from each lesson.json, never from a hand maintained index, so the
// catalog can never disagree with what is actually on disk.

#ifndef RCPP_CATALOG_HPP
#define RCPP_CATALOG_HPP

#include <string>
#include <vector>

#include "json.hpp"
#include "util.hpp"

namespace rcpp {

struct Lesson {
  std::string id;             // 07-04-lock-free-spsc-queue
  std::string title;
  std::string type;           // learn | build | capstone
  std::string dir_slug;       // 04-lock-free-spsc-queue
  std::string rel_path;       // phases/07-.../04-...
  fs::path path;
  int phase_num = 0;
  int lesson_num = 0;
  int minutes = 0;
  int hardware_tier = 0;
  int cxx_standard = 20;
  std::vector<std::string> platforms;
  std::vector<std::string> requires_ids;
  std::vector<std::string> breaks_first;
  std::vector<std::string> qt_modules;
  bool needs_ros2 = false;
  json::Value raw;

  std::string target_name() const;  // lesson_07_04_lock_free_spsc_queue
};

struct Phase {
  int num = 0;
  std::string slug;      // 07-concurrency-and-real-time
  std::string title;     // Concurrency and Real Time
  std::string ends_with; // what a learner has when the phase is finished
  std::vector<Lesson> lessons;
};

struct Catalog {
  fs::path root;
  std::vector<Phase> phases;
  std::vector<std::string> load_errors;

  int lesson_count() const;
  const Lesson* find(const std::string& id) const;
  std::vector<const Lesson*> all() const;
};

Catalog load_catalog(const fs::path& repo_root);

std::string phase_title_from_slug(const std::string& slug);

}  // namespace rcpp

#endif  // RCPP_CATALOG_HPP
