#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <rc/core/version.hpp>

using rc::core::compare;
using rc::core::parse_version;
using rc::core::Version;

// "at least low, and below high": the low end is included and the high end is not, as in 1.0 up to but
// not including 2.0. With no high end, anything from low upwards.
struct Range {
  Version low;
  bool has_high = false;
  Version high;
};

inline Version ver(const char* text) {
  Version v;
  parse_version(text, &v);
  return v;
}

inline Range range(const char* low, const char* high) {
  Range r;
  r.low = ver(low);
  r.has_high = true;
  r.high = ver(high);
  return r;
}

inline Range at_least(const char* low) {
  Range r;
  r.low = ver(low);
  return r;
}

// Is this version inside the range?
inline bool contains(const Range& r, const Version& v) {
  // TODO: false if v is older than r.low (compare(v, r.low) < 0). False if the range has a high end and v
  // is not below it (compare(v, r.high) >= 0). Otherwise true.
  (void)r;
  (void)v;
  return false;
}

// Nothing can be inside it: the high end is not above the low end.
inline bool is_empty(const Range& r) {
  // TODO: only a range with a high end can be empty: the high end is not above the low end,
  // compare(r.high, r.low) <= 0.
  (void)r;
  return false;
}

// What two ranges both allow: the larger of the low ends, and the smaller of the high ends.
inline Range intersect(const Range& a, const Range& b) {
  // TODO: the low end is the larger of the two low ends. The high end is the smaller of the two high ends,
  // when both have one; if only one has a high end, that is the result's; if neither, there is none.
  (void)a;
  (void)b;
  return Range();
}

// One thing that needs another, within a range, and who asked.
struct Dep {
  std::string name;
  Range range;
  std::string from;
};

struct Package {
  std::string name;
  Version version;
  std::vector<Dep> deps;
};

struct Resolution {
  bool ok = false;
  std::vector<std::pair<std::string, std::string> > chosen;   // name and version, in name order
  std::string problem;
};

namespace resolve_detail {

using Constraints = std::map<std::string, std::vector<Dep>>;

inline std::string show(const Version& v) {
  std::string s = std::to_string(v.major) + "." + std::to_string(v.minor) + "." + std::to_string(v.patch);
  if (!v.pre.empty()) s += "-" + v.pre;
  return s;
}

inline std::string show(const Range& r) {
  return "[" + show(r.low) + ", " + (r.has_high ? show(r.high) : std::string("any")) + ")";
}

inline bool allowed(const std::vector<Dep>& wants, const Version& v) {
  for (const Dep& d : wants) {
    if (!contains(d.range, v)) return false;
  }
  return true;
}

// Newest first.
inline std::vector<const Package*> candidates(const std::string& name, const std::vector<Package>& repo,
                                              const std::vector<Dep>& wants) {
  std::vector<const Package*> found;
  for (const Package& p : repo) {
    if (p.name == name && allowed(wants, p.version)) found.push_back(&p);
  }
  for (std::size_t i = 0; i < found.size(); ++i) {
    for (std::size_t j = i + 1; j < found.size(); ++j) {
      if (compare(found[j]->version, found[i]->version) > 0) std::swap(found[i], found[j]);
    }
  }
  return found;
}

inline std::string describe_wants(const std::string& name, const std::vector<Dep>& wants) {
  std::string text = name + ": no version satisfies everyone, since";
  for (std::size_t i = 0; i < wants.size(); ++i) {
    text += (i == 0 ? " " : ", ") + wants[i].from + " needs " + show(wants[i].range);
  }
  return text;
}

inline bool solve(Constraints& wants, const std::vector<Package>& repo, std::map<std::string, const Package*>& chosen,
                  std::string* problem, int* steps) {
  ++*steps;
  // The first package somebody needs and nobody has chosen yet.
  std::string next;
  for (Constraints::const_iterator it = wants.begin(); it != wants.end(); ++it) {
    if (chosen.find(it->first) == chosen.end()) {
      next = it->first;
      break;
    }
  }
  if (next.empty()) return true;

  const std::vector<const Package*> options = candidates(next, repo, wants[next]);
  if (options.empty()) {
    if (problem != nullptr && problem->empty()) {
      bool exists = false;
      for (const Package& p : repo) {
        if (p.name == next) exists = true;
      }
      *problem = exists ? describe_wants(next, wants[next]) : next + ": no version is available at all";
    }
    return false;
  }

  for (const Package* pick : options) {
    chosen[next] = pick;
    Constraints saved = wants;
    for (const Dep& d : pick->deps) wants[d.name].push_back(d);

    // Everything already chosen must still be allowed by what has just been added.
    bool consistent = true;
    for (std::map<std::string, const Package*>::const_iterator c = chosen.begin(); c != chosen.end(); ++c) {
      if (!allowed(wants[c->first], c->second->version)) consistent = false;
    }
    if (consistent && solve(wants, repo, chosen, problem, steps)) return true;

    wants = saved;                       // back up: forget this choice and what it asked for
    chosen.erase(next);
  }
  if (problem != nullptr && problem->empty()) *problem = describe_wants(next, wants[next]);
  return false;
}

}  // namespace resolve_detail

// Choose a version of every package that is needed, so that every requirement holds, preferring the newest.
// If the newest of something cannot be used, back up and try the next, and say who disagreed if nothing works.
inline Resolution resolve(const std::vector<Dep>& roots, const std::vector<Package>& repo, int* steps = nullptr) {
  Resolution result;
  resolve_detail::Constraints wants;
  for (const Dep& d : roots) wants[d.name].push_back(d);
  std::map<std::string, const Package*> chosen;
  int counted = 0;

  result.ok = resolve_detail::solve(wants, repo, chosen, &result.problem, &counted);
  if (steps != nullptr) *steps = counted;
  if (result.ok) {
    result.problem.clear();
    for (std::map<std::string, const Package*>::const_iterator c = chosen.begin(); c != chosen.end(); ++c) {
      result.chosen.push_back(std::make_pair(c->first, resolve_detail::show(c->second->version)));
    }
  }
  return result;
}

#endif  // LESSON_SOLUTION_HPP
