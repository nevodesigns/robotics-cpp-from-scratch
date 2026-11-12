#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <rc/field/sensors.hpp>
#include <rc/field/terrain.hpp>
#include <sstream>
#include <string>
#include <vector>

struct FieldPatch {
  std::string surface;
  double x0 = 0.0;
  double y0 = 0.0;
  double x1 = 0.0;
  double y1 = 0.0;
  double grade_x = 0.0;
  double grade_y = 0.0;
};

struct FieldGoal {
  double x = 0.0;
  double y = 0.0;
  double tolerance = 0.0;
};

// A thing that goes wrong for a while: a kind, the time it starts and ends, and one number (the probability, the fraction, the bias) for the kinds that have one.
struct FieldFault {
  std::string kind;
  double t0 = 0.0;
  double t1 = 0.0;
  double param = 0.0;
  int line = 0;
};

struct FieldScenario {
  std::string name;
  double duration = 0.0;
  unsigned int seed = 1;
  std::string ground = "tarmac";
  std::vector<FieldPatch> patches;
  std::vector<rc::field::Segment> walls;
  std::vector<rc::field::Circle> posts;
  bool has_start = false;
  double start_x = 0.0;
  double start_y = 0.0;
  double start_heading_deg = 0.0;
  std::vector<FieldGoal> goals;
  std::vector<FieldFault> faults;
};

struct ScenarioProblem {
  int line = 0;  // 0 for a problem with the file as a whole
  std::string message;
};

struct ScenarioParse {
  FieldScenario scenario;
  std::vector<ScenarioProblem> problems;
  bool ok() const { return problems.empty(); }
};

inline bool parse_double(const std::string& word, double& out) {
  char* end = nullptr;
  out = std::strtod(word.c_str(), &end);
  return end != word.c_str() && *end == '\0' && std::isfinite(out);
}

// The surfaces a scenario may name.
inline bool surface_by_name(const std::string& name, rc::field::Surface& out) {
  if (name == "tarmac") out = rc::field::tarmac();
  else if (name == "gravel") out = rc::field::gravel();
  else if (name == "grass") out = rc::field::grass();
  else if (name == "sand") out = rc::field::sand();
  else if (name == "ice") out = rc::field::ice();
  else return false;
  return true;
}

// How many numbers a fault kind carries after its two times, or -1 for a kind that does not exist.
inline int fault_params(const std::string& kind) {
  if (kind == "gnss_outage") return 0;
  if (kind == "lidar_dropout" || kind == "encoder_slip" || kind == "imu_bias" || kind == "battery_sag") return 1;
  return -1;
}

inline std::vector<std::string> split_words(const std::string& line) {
  std::vector<std::string> words;
  std::istringstream in(line);
  std::string w;
  while (in >> w) words.push_back(w);
  return words;
}

// Reads one statement (the words of a line, not empty) into the scenario, adding a problem for anything wrong with it.
inline void interpret(FieldScenario& sc, const std::vector<std::string>& w, int line, std::vector<ScenarioProblem>& problems) {
  const std::string& key = w[0];
  const std::size_t n = w.size() - 1;
  auto bad = [&](const std::string& message) {
    ScenarioProblem p;
    p.line = line;
    p.message = message;
    problems.push_back(p);
  };
  auto count = [&](std::size_t wanted) {
    if (n == wanted) return true;
    bad("'" + key + "' needs " + std::to_string(wanted) + " values, found " + std::to_string(n));
    return false;
  };
  auto numbers = [&](std::size_t first, std::size_t how_many, double* out) {
    for (std::size_t i = 0; i < how_many; ++i) {
      if (!parse_double(w[first + i], out[i])) {
        bad("'" + w[first + i] + "' is not a number");
        return false;
      }
    }
    return true;
  };
  double v[6] = {0, 0, 0, 0, 0, 0};
  rc::field::Surface surface = rc::field::tarmac();
  if (key == "scenario") {
    if (count(1)) sc.name = w[1];
  } else if (key == "duration") {
    if (count(1) && numbers(1, 1, v)) sc.duration = v[0];
  } else if (key == "seed") {
    if (count(1) && numbers(1, 1, v)) sc.seed = static_cast<unsigned int>(v[0]);
  } else if (key == "ground") {
    if (count(1)) {
      if (surface_by_name(w[1], surface)) sc.ground = w[1];
      else bad("unknown surface '" + w[1] + "'");
    }
  } else if (key == "patch") {
    if (n != 5 && n != 7) {
      bad("'patch' needs 5 or 7 values, found " + std::to_string(n));
    } else if (!surface_by_name(w[1], surface)) {
      bad("unknown surface '" + w[1] + "'");
    } else if (numbers(2, n - 1, v)) {
      FieldPatch p;
      p.surface = w[1];
      p.x0 = v[0];
      p.y0 = v[1];
      p.x1 = v[2];
      p.y1 = v[3];
      if (n == 7) {
        p.grade_x = v[4];
        p.grade_y = v[5];
      }
      if (p.x1 <= p.x0 || p.y1 <= p.y0) bad("a patch needs x1 > x0 and y1 > y0");
      else sc.patches.push_back(p);
    }
  } else if (key == "wall") {
    if (count(4) && numbers(1, 4, v)) sc.walls.push_back(rc::field::Segment{v[0], v[1], v[2], v[3]});
  } else if (key == "post") {
    if (count(3) && numbers(1, 3, v)) {
      if (v[2] <= 0.0) bad("a post needs a positive radius");
      else sc.posts.push_back(rc::field::Circle{v[0], v[1], v[2]});
    }
  } else if (key == "start") {
    if (count(3) && numbers(1, 3, v)) {
      sc.has_start = true;
      sc.start_x = v[0];
      sc.start_y = v[1];
      sc.start_heading_deg = v[2];
    }
  } else if (key == "goal") {
    if (count(3) && numbers(1, 3, v)) {
      if (v[2] <= 0.0) bad("a goal needs a positive tolerance");
      else sc.goals.push_back(FieldGoal{v[0], v[1], v[2]});
    }
  } else if (key == "fault") {
    if (n < 3) {
      bad("'fault' needs a kind and two times, found " + std::to_string(n) + " values");
    } else if (fault_params(w[1]) < 0) {
      bad("unknown fault '" + w[1] + "'");
    } else if (n != 3 + static_cast<std::size_t>(fault_params(w[1]))) {
      bad("fault '" + w[1] + "' needs " + std::to_string(3 + fault_params(w[1])) + " values, found " + std::to_string(n));
    } else if (numbers(2, n - 1, v)) {
      FieldFault f;
      f.kind = w[1];
      f.t0 = v[0];
      f.t1 = v[1];
      f.param = n == 4 ? v[2] : 0.0;
      f.line = line;
      sc.faults.push_back(f);
    }
  } else {
    bad("unknown keyword '" + key + "'");
  }
}

// What must be true of the file as a whole.
inline void validate(const FieldScenario& sc, std::vector<ScenarioProblem>& problems) {
  auto bad = [&](int line, const std::string& message) {
    ScenarioProblem p;
    p.line = line;
    p.message = message;
    problems.push_back(p);
  };
  if (!(sc.duration > 0.0)) bad(0, "no duration, or it is not positive");
  if (!sc.has_start) bad(0, "no start");
  for (std::size_t i = 0; i < sc.faults.size(); ++i) {
    const FieldFault& f = sc.faults[i];
    if (f.t1 <= f.t0) bad(f.line, "fault '" + f.kind + "' ends before it starts");
    else if (sc.duration > 0.0 && f.t1 > sc.duration) bad(f.line, "fault '" + f.kind + "' ends after the scenario does");
  }
}

inline ScenarioParse parse_scenario(const std::string& text) {
  ScenarioParse out;
  std::istringstream in(text);
  std::string line;
  int number = 0;
  while (std::getline(in, line)) {
    ++number;
    const std::size_t hash = line.find('#');
    if (hash != std::string::npos) line.erase(hash);
    const std::vector<std::string> words = split_words(line);
    if (words.empty()) continue;
    interpret(out.scenario, words, number, out.problems);
  }
  validate(out.scenario, out.problems);
  // In the order of the file, with the problems about the file as a whole last.
  std::stable_sort(out.problems.begin(), out.problems.end(), [](const ScenarioProblem& a, const ScenarioProblem& b) {
    return (a.line == 0 ? 1000000000 : a.line) < (b.line == 0 ? 1000000000 : b.line);
  });
  return out;
}

inline std::string write_scenario(const FieldScenario& sc) {
  std::ostringstream o;
  o << std::setprecision(12);
  o << "scenario " << sc.name << "\n";
  o << "duration " << sc.duration << "\n";
  o << "seed " << sc.seed << "\n";
  o << "ground " << sc.ground << "\n";
  for (std::size_t i = 0; i < sc.patches.size(); ++i) {
    const FieldPatch& p = sc.patches[i];
    o << "patch " << p.surface << " " << p.x0 << " " << p.y0 << " " << p.x1 << " " << p.y1;
    if (p.grade_x != 0.0 || p.grade_y != 0.0) o << " " << p.grade_x << " " << p.grade_y;
    o << "\n";
  }
  for (std::size_t i = 0; i < sc.walls.size(); ++i) o << "wall " << sc.walls[i].x0 << " " << sc.walls[i].y0 << " " << sc.walls[i].x1 << " " << sc.walls[i].y1 << "\n";
  for (std::size_t i = 0; i < sc.posts.size(); ++i) o << "post " << sc.posts[i].x << " " << sc.posts[i].y << " " << sc.posts[i].r << "\n";
  if (sc.has_start) o << "start " << sc.start_x << " " << sc.start_y << " " << sc.start_heading_deg << "\n";
  for (std::size_t i = 0; i < sc.goals.size(); ++i) o << "goal " << sc.goals[i].x << " " << sc.goals[i].y << " " << sc.goals[i].tolerance << "\n";
  for (std::size_t i = 0; i < sc.faults.size(); ++i) {
    const FieldFault& f = sc.faults[i];
    o << "fault " << f.kind << " " << f.t0 << " " << f.t1;
    if (fault_params(f.kind) == 1) o << " " << f.param;
    o << "\n";
  }
  return o.str();
}

inline rc::field::Terrain terrain_of(const FieldScenario& sc) {
  rc::field::Cell base;
  surface_by_name(sc.ground, base.surface);
  rc::field::Terrain terrain(base);
  for (std::size_t i = 0; i < sc.patches.size(); ++i) {
    const FieldPatch& p = sc.patches[i];
    rc::field::Patch patch;
    patch.x0 = p.x0;
    patch.y0 = p.y0;
    patch.x1 = p.x1;
    patch.y1 = p.y1;
    surface_by_name(p.surface, patch.cell.surface);
    patch.cell.grade_x = p.grade_x;
    patch.cell.grade_y = p.grade_y;
    terrain.add(patch);
  }
  return terrain;
}

inline rc::field::World2 world_of(const FieldScenario& sc) {
  rc::field::World2 w;
  w.walls = sc.walls;
  w.posts = sc.posts;
  return w;
}

// Is a fault of this kind on at this time, and if so with what number.
inline bool fault_active(const FieldScenario& sc, const std::string& kind, double t, double* param) {
  for (std::size_t i = 0; i < sc.faults.size(); ++i) {
    const FieldFault& f = sc.faults[i];
    if (f.kind == kind && t >= f.t0 && t < f.t1) {
      if (param != nullptr) *param = f.param;
      return true;
    }
  }
  return false;
}


#endif
