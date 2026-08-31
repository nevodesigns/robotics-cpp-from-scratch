// rcpp next, rcpp status
//
// The loop that turns a pile of lessons into a course. next answers what should
// I do now, using the prerequisite graph and what has actually been passed.
// status shows how far the learner has got.

#include <algorithm>
#include <iostream>
#include <vector>

#include "catalog.hpp"
#include "commands.hpp"
#include "progress.hpp"
#include "util.hpp"

namespace rcpp {
namespace {

bool prerequisites_met(const Lesson& lesson, const Progress& progress) {
  for (const std::string& needed : lesson.requires_ids)
    if (!progress.has(needed)) return false;
  return true;
}

std::vector<std::string> missing_prerequisites(const Lesson& lesson, const Progress& progress) {
  std::vector<std::string> missing;
  for (const std::string& needed : lesson.requires_ids)
    if (!progress.has(needed)) missing.push_back(needed);
  return missing;
}

// A blocked lesson and the reason it is blocked, on one line. Counting blocked
// lessons tells a learner that something is in the way without telling them
// what, which is the least useful thing a course tool can say.
void print_blocked(const std::vector<const Lesson*>& blocked, const Progress& progress,
                   std::size_t how_many) {
  std::size_t shown = 0;
  for (const Lesson* lesson : blocked) {
    if (shown++ >= how_many) break;
    const std::vector<std::string> missing = missing_prerequisites(*lesson, progress);

    std::string waiting;
    for (std::size_t i = 0; i < missing.size(); ++i) {
      if (i > 0) waiting += ", ";
      waiting += missing[i];
    }

    std::cout << "  " << style::dim(lesson->id) << "  " << lesson->title << "\n"
              << "      " << style::warn("waiting on") << " " << waiting << "\n\n";
  }

  if (blocked.size() > shown)
    std::cout << style::dim("  and " + std::to_string(blocked.size() - shown) +
                            " more, run rcpp next --blocked to see them all\n\n");
}

}  // namespace

int cmd_next(const Args& args) {
  const auto root = find_repo_root();
  if (!root) { std::cerr << "rcpp next: run this from inside the repository\n"; return 2; }

  const Catalog catalog = load_catalog(*root);
  const Progress progress = load_progress(*root);
  const int how_many = std::max(1, std::atoi(flag_value(args, "--count", "3").c_str()));

  std::vector<const Lesson*> ready;
  std::vector<const Lesson*> blocked;
  for (const auto* lesson : catalog.all()) {
    if (progress.has(lesson->id)) continue;
    if (prerequisites_met(*lesson, progress)) ready.push_back(lesson);
    else blocked.push_back(lesson);
  }

  if (ready.empty() && blocked.empty()) {
    std::cout << "\n" << style::pass("Everything on disk is passed.") << "\n"
              << "  " << catalog.lesson_count() << " lessons, all of them green.\n\n";
    return 0;
  }

  if (ready.empty()) {
    std::cout << "\n" << style::warn("Nothing is unlocked.") << "\n\n"
              << "  Every remaining lesson is waiting on a prerequisite, which means the\n"
              << "  graph or your progress file is inconsistent. What each one wants:\n\n";
    print_blocked(blocked, progress, blocked.size());
    return 1;
  }

  // Asked for directly, this is the whole list rather than a count.
  if (has_flag(args, "--blocked")) {
    if (blocked.empty()) {
      std::cout << "\n" << style::pass("Nothing is blocked.") << "\n"
                << "  Every lesson you have not passed is available now.\n\n";
      return 0;
    }
    std::cout << "\n" << style::bold("Blocked") << "\n\n";
    print_blocked(blocked, progress, blocked.size());
    return 0;
  }

  std::cout << "\n" << style::bold("Next") << "\n\n";
  int shown = 0;
  for (const Lesson* lesson : ready) {
    if (shown++ >= how_many) break;
    std::cout << "  " << style::pass(lesson->id) << "  " << lesson->title << "\n";
    std::cout << "      " << style::dim(std::to_string(lesson->minutes) + " min   " +
                                        lesson->rel_path + "/docs/en.md")
              << "\n";
    if (shown == 1) {
      std::cout << "      " << style::bold("rcpp verify " + lesson->id) << "\n";
    }
    std::cout << "\n";
  }

  if (!blocked.empty()) {
    std::cout << style::dim("  " + std::to_string(blocked.size()) +
                            " further lesson(s) unlock as you go, rcpp next --blocked\n"
                            "  shows what each one is waiting on.\n\n");
  }
  return 0;
}

int cmd_status(const Args&) {
  const auto root = find_repo_root();
  if (!root) { std::cerr << "rcpp status: run this from inside the repository\n"; return 2; }

  const Catalog catalog = load_catalog(*root);
  const Progress progress = load_progress(*root);

  std::cout << "\n" << style::bold("Progress") << "\n\n";

  int done_total = 0;
  for (const Phase& phase : catalog.phases) {
    int done = 0;
    for (const Lesson& lesson : phase.lessons)
      if (progress.has(lesson.id)) ++done;
    done_total += done;

    // A twenty character bar, because a number alone does not show shape.
    const std::size_t width = 20;
    const std::size_t filled =
        phase.lessons.empty() ? 0 : (done * width) / phase.lessons.size();
    std::string bar;
    for (std::size_t i = 0; i < width; ++i) bar += (i < filled) ? '#' : '.';

    const std::string counts = std::to_string(done) + "/" + std::to_string(phase.lessons.size());
    std::cout << "  " << (done == static_cast<int>(phase.lessons.size()) ? style::pass(bar)
                                                                        : style::dim(bar))
              << "  " << counts << "  " << phase.title << "\n";
  }

  std::cout << "\n  " << done_total << " of " << catalog.lesson_count() << " lessons passed\n";
  if (done_total < catalog.lesson_count())
    std::cout << "  " << style::bold("rcpp next") << " to see what is unlocked\n";
  std::cout << "\n";
  return 0;
}

}  // namespace rcpp
