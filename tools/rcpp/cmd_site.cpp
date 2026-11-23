// rcpp site
//
// Writes the course as a website: a home page, the roadmap of ten tracks, a page
// for every phase (its lessons as a checklist), every lesson (its text, the file
// to write, the command that checks it, what breaks first and its quiz), three
// playgrounds that run in the browser, a self-check that says where to start,
// setup, and the Failure Atlas with a search box. The pages are plain files with
// relative links, so they open from disk and can be put on any static host;
// --public leaves this machine's passed lessons out of them. Progress a reader
// ticks is kept in their browser only. The stylesheet and scripts are the files
// in tools/site/.
//
// Like the other commands, everything is read from the file system each time, so
// the pages can never disagree with the lessons; run it again after a pull.

#include <cctype>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "atlas.hpp"
#include "catalog.hpp"
#include "commands.hpp"
#include "json.hpp"
#include "progress.hpp"
#include "util.hpp"

namespace rcpp {
namespace {

std::string html_escape(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out += c;
    }
  }
  return out;
}

bool is_digit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

// What the inline renderer needs to turn names into links.
struct Links {
  std::map<std::string, std::string> lesson_by_prefix;  // "31-09" -> "31-09-keeping-what-the-hand-showed"
  std::map<std::string, bool> atlas_ids;
  std::string up;                                        // "../" from a page in a sub folder, "" from the top
};

// An atlas id (E-ABC-0001) at position i of s, or an empty string.
std::string atlas_id_at(const std::string& s, std::size_t i) {
  if (i + 2 >= s.size() || s[i] != 'E' || s[i + 1] != '-') return "";
  std::size_t j = i + 2;
  while (j < s.size() && std::isupper(static_cast<unsigned char>(s[j]))) ++j;
  if (j == i + 2 || j >= s.size() || s[j] != '-') return "";
  std::size_t k = j + 1;
  while (k < s.size() && is_digit(s[k])) ++k;
  if (k - (j + 1) != 4) return "";
  return s.substr(i, k - i);
}

// Plain text (already escaped) with lesson numbers such as 31-09 and atlas ids turned into links.
std::string link_names(const std::string& t, const Links& links) {
  std::string out;
  for (std::size_t i = 0; i < t.size();) {
    const std::string id = atlas_id_at(t, i);
    if (!id.empty() && links.atlas_ids.count(id)) {
      out += "<a href=\"" + links.up + "e/" + id + ".html\">" + id + "</a>";
      i += id.size();
      continue;
    }
    // NN-NN not touching other digits or another "-NN": a lesson number, if such a lesson exists
    if (i + 5 <= t.size() && is_digit(t[i]) && is_digit(t[i + 1]) && t[i + 2] == '-' && is_digit(t[i + 3]) &&
        is_digit(t[i + 4]) && (i == 0 || (!is_digit(t[i - 1]) && t[i - 1] != '-')) &&
        (i + 5 == t.size() || (!is_digit(t[i + 5]) && !(t[i + 5] == '-' && i + 6 < t.size() && is_digit(t[i + 6]))))) {
      const auto it = links.lesson_by_prefix.find(t.substr(i, 5));
      if (it != links.lesson_by_prefix.end()) {
        out += "<a href=\"" + links.up + "l/" + it->second + ".html\">" + t.substr(i, 5) + "</a>";
        i += 5;
        continue;
      }
    }
    out += t[i++];
  }
  return out;
}

// Bold and emphasis in text that has no code in it.
std::string emphasis(const std::string& t) {
  std::string out;
  bool bold = false;
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] == '*' && i + 1 < t.size() && t[i + 1] == '*') {
      out += bold ? "</strong>" : "<strong>";
      bold = !bold;
      ++i;
    } else {
      out += t[i];
    }
  }
  if (bold) out += "</strong>";
  // *word* (a star touching a letter on the inside of each end) is emphasis; a star with space round it is left alone
  std::string em;
  for (std::size_t i = 0; i < out.size(); ++i) {
    if (out[i] == '*' && i + 1 < out.size() && out[i + 1] != ' ' && out[i + 1] != '*' && (i == 0 || out[i - 1] == ' ' || out[i - 1] == '(')) {
      const std::size_t end = out.find('*', i + 1);
      if (end != std::string::npos && out[end - 1] != ' ' && end - i < 80) {
        em += "<em>" + out.substr(i + 1, end - i - 1) + "</em>";
        i = end;
        continue;
      }
    }
    em += out[i];
  }
  return em;
}

// One line of markdown text: `code`, **bold**, and the names that become links.
std::string inline_md(const std::string& raw, const Links& links) {
  std::string out, text;
  for (std::size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '`') {
      const std::size_t end = raw.find('`', i + 1);
      if (end != std::string::npos) {
        out += emphasis(link_names(html_escape(text), links));
        text.clear();
        const std::string code = raw.substr(i + 1, end - i - 1);
        if (links.atlas_ids.count(code))
          out += "<a href=\"" + links.up + "e/" + code + ".html\"><code>" + code + "</code></a>";
        else
          out += "<code>" + html_escape(code) + "</code>";
        i = end;
        continue;
      }
    }
    text += raw[i];
  }
  out += emphasis(link_names(html_escape(text), links));
  return out;
}

bool is_table_rule(const std::string& line) {
  const std::string t = trim(line);
  if (t.size() < 3 || t[0] != '|') return false;
  for (char c : t)
    if (c != '|' && c != '-' && c != ':' && c != ' ') return false;
  return true;
}

std::vector<std::string> table_cells(const std::string& line) {
  std::string t = trim(line);
  if (!t.empty() && t.front() == '|') t.erase(0, 1);
  if (!t.empty() && t.back() == '|') t.pop_back();
  std::vector<std::string> cells;
  for (const std::string& c : split(t, '|')) cells.push_back(trim(c));
  return cells;
}

bool list_item(const std::string& line, bool& ordered, std::string& text) {
  std::size_t i = 0;
  while (i < line.size() && line[i] == ' ') ++i;
  if (i + 1 < line.size() && (line[i] == '-' || line[i] == '*') && line[i + 1] == ' ') {
    ordered = false;
    text = line.substr(i + 2);
    return true;
  }
  std::size_t j = i;
  while (j < line.size() && is_digit(line[j])) ++j;
  if (j > i && j + 1 < line.size() && line[j] == '.' && line[j + 1] == ' ') {
    ordered = true;
    text = line.substr(j + 2);
    return true;
  }
  return false;
}

// The markdown the lessons use: headings, paragraphs, lists, tables, quotes and fenced code.
std::string markdown_to_html(const std::string& md, const Links& links) {
  std::vector<std::string> lines = split(md, '\n');
  for (std::string& l : lines)
    if (!l.empty() && l.back() == '\r') l.pop_back();
  std::ostringstream out;
  std::string para;
  auto flush = [&]() {
    if (!trim(para).empty()) {
      std::string p = inline_md(trim(para), links);
      for (std::size_t at = p.find('\x01'); at != std::string::npos; at = p.find('\x01')) p.replace(at, 1, "<br>");
      out << "<p>" << p << "</p>\n";
    }
    para.clear();
  };
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string& line = lines[i];
    const std::string t = trim(line);
    if (starts_with(t, "```")) {
      flush();
      std::string code;
      for (++i; i < lines.size() && !starts_with(trim(lines[i]), "```"); ++i) code += lines[i] + "\n";
      out << "<pre><code>" << html_escape(code) << "</code></pre>\n";
      continue;
    }
    if (t.empty()) { flush(); continue; }
    if (t[0] == '#') {
      flush();
      std::size_t level = 0;
      while (level < t.size() && t[level] == '#') ++level;
      if (level > 4) level = 4;
      out << "<h" << level << ">" << inline_md(trim(t.substr(level)), links) << "</h" << level << ">\n";
      continue;
    }
    if (t[0] == '>') {
      flush();
      std::string q;
      for (; i < lines.size() && starts_with(trim(lines[i]), ">"); ++i) q += trim(trim(lines[i]).substr(1)) + " ";
      --i;
      out << "<blockquote>" << inline_md(trim(q), links) << "</blockquote>\n";
      continue;
    }
    if (t[0] == '|' && i + 1 < lines.size() && is_table_rule(lines[i + 1])) {
      flush();
      out << "<div class=\"table\"><table>\n<tr>";
      for (const std::string& c : table_cells(line)) out << "<th>" << inline_md(c, links) << "</th>";
      out << "</tr>\n";
      for (i += 2; i < lines.size() && starts_with(trim(lines[i]), "|"); ++i) {
        out << "<tr>";
        for (const std::string& c : table_cells(lines[i])) out << "<td>" << inline_md(c, links) << "</td>";
        out << "</tr>\n";
      }
      --i;
      out << "</table></div>\n";
      continue;
    }
    bool ordered = false;
    std::string item;
    if (list_item(line, ordered, item)) {
      flush();
      out << (ordered ? "<ol>\n" : "<ul>\n");
      for (; i < lines.size(); ++i) {
        bool o = false;
        std::string it;
        if (list_item(lines[i], o, it)) {
          out << "<li>" << inline_md(it, links);
          // lines that continue the item, indented and not themselves items
          while (i + 1 < lines.size() && !trim(lines[i + 1]).empty() && lines[i + 1][0] == ' ' &&
                 !list_item(lines[i + 1], o, it)) {
            out << " " << inline_md(trim(lines[++i]), links);
          }
          out << "</li>\n";
        } else {
          break;
        }
      }
      --i;
      out << (ordered ? "</ol>\n" : "</ul>\n");
      continue;
    }
    // the lines of label and value at the top of a page (**Type:** Build) each keep a line of their own
    if (!para.empty() && starts_with(t, "**")) para += "\x01";
    para += t + " ";
  }
  flush();
  return out.str();
}

// ---- the site's structure -------------------------------------------------------------------

// The phases grouped as a learner meets them.
struct Track {
  const char* name;
  int from, to;
  const char* blurb;
  bool optional;
};

const Track kTracks[] = {
    {"C++ a robot can trust", 0, 5,
     "The language, its memory, its build and its tests, taught on robot problems from the first lesson.", false},
    {"Maths, time and the outside world", 6, 8,
     "Frames and rotations, threads and deadlines, ports and bytes: what every robot program stands on.", false},
    {"Interfaces with Qt", 9, 12,
     "The control station a person uses: a sensor on its own thread, live charts, QML, a link that says when it drops. Skip it if you will never build an interface.", true},
    {"The robot itself", 13, 16,
     "Kinematics, control, sensors and estimation: how a robot moves, holds still, senses and knows where it is.", false},
    {"Systems that ship", 17, 21,
     "Middleware and ROS 2, simulation and the bench, safety, deployment, and capstones that run a whole robot with faults injected.", false},
    {"Ground vehicles", 22, 27,
     "Wheeled and tracked vehicles: their models, perception for driving, localisation outdoors, planning per vehicle, and a day of work.", false},
    {"Robot arms", 28, 32,
     "Arms in three dimensions: geometry, dynamics and joint control, collision-free motion, picking and placing, and the other arm families.", false},
    {"Mobile manipulators", 33, 33,
     "An arm on a wheeled base: where to park, the whole body as one, tipping, and a fetch-and-carry job checked before release.", false},
    {"Legged robots", 34, 36,
     "Legs, gaits and balance, and a quadruped controller that does what it is told and keeps itself safe.", false},
    {"Robots that learn", 38, 39,
     "Learning from demonstrations and learning by trial, at a size where every number can be checked, with the guards and release checks a robot needs.", false},
};

// The self-check: what a learner can already do, and the phase that teaches it.
struct Can {
  int phase;
  const char* text;
};
const Can kCan[] = {
    {0, "build and run a C++ program with CMake from a terminal"},
    {1, "explain what a reference, a struct and an enum class are, and when each is the right tool"},
    {2, "say who owns every allocation in a program, and use unique_ptr instead of new and delete"},
    {3, "write a small class template, and use std::optional and std::chrono without guessing"},
    {4, "make a CMake library that another project installs and links against"},
    {5, "test code that talks to hardware by putting a fake in its place, and show the tests catch a bug"},
    {6, "move a point between two frames with a rotation and a translation, composing them in the right order"},
    {7, "share data between two threads without a race, and measure how late a loop runs"},
    {8, "hold a framed, checksummed conversation with a device over a serial port"},
    {13, "compute the forward and inverse kinematics of a two-link arm, and say where it is singular"},
    {14, "tune a PID loop and explain overshoot, steady error and integral wind-up"},
    {15, "filter a noisy sensor and say what the filter costs in delay"},
    {16, "fuse two estimates of a position, and plan a path through a grid map"},
    {17, "explain publish and subscribe, quality of service, and a node's life cycle"},
    {18, "run the same controller against a simulator and against hardware, and measure the difference"},
    {19, "write a risk assessment in which every hazard is tied to a test"},
};

// A playground and the lessons it mirrors.
struct Play {
  const char* slug;
  const char* kicker;
  const char* title;
  const char* blurb;
  const char* script;
  const char* lessons[4];
};
const Play kPlays[] = {
    {"arm", "phases 13 and 32 \xc2\xb7 kinematics", "Drag a robot arm",
     "The two-link arm of 32-01: its closed-form inverse kinematics, both elbows, its reach, and the determinant that goes to zero at a singularity, live as you drag.",
     "play-arm.js", {"13-02", "13-04", "32-01", "28-06"}},
    {"pid", "phase 14 \xc2\xb7 control", "Tune a PID loop",
     "A motor holding an angle. Turn Kp, Ki and Kd, push it, and watch overshoot, settling, saturation and the steady error that only the integral removes.",
     "play-pid.js", {"14-01", "14-03", "14-04", "14-08"}},
    {"rl", "phase 39 \xc2\xb7 learning by trial", "Train a robot by trial",
     "Q-learning on the pit grid of phase 39. Change the slip, the discount and the exploring, and watch which way round the pits it learns to go.",
     "play-q.js", {"39-01", "39-02", "39-03", "39-04"}},
};

std::string lesson_number(const Lesson& l) { return l.id.substr(0, 5); }

std::string lower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::string hours_text(int minutes) {
  const int h = (minutes + 30) / 60;
  return "~" + std::to_string(h) + " h";
}

std::string two_digits(int n) { return (n < 10 ? "0" : "") + std::to_string(n); }

struct Site {
  const Catalog* catalog = nullptr;
  const Atlas* atlas = nullptr;
  const Progress* progress = nullptr;
  std::vector<const Lesson*> all;
  std::map<std::string, const Lesson*> by_id;
  std::map<std::string, const Lesson*> by_number;     // "13-02"
  std::map<std::string, const AtlasEntry*> atlas_by_id;
  std::map<int, const Phase*> phase_of;
  std::string repo_url = "https://github.com/nevodesigns/robotics-cpp-from-scratch";
};

std::string phase_page(const Phase& p) { return "p/" + p.slug + ".html"; }

int phase_minutes(const Phase& p) {
  int m = 0;
  for (const Lesson& l : p.lessons) m += l.minutes;
  return m;
}

std::string lesson_ids(const Phase& p) {
  std::string s;
  for (const Lesson& l : p.lessons) s += l.id + " ";
  return s;
}

// The page frame: the bar along the top, the content, the footer.
std::string page(const Site& site, const std::string& title, const std::string& description, const std::string& up,
                 const std::string& active, const std::string& body, bool narrow) {
  std::ostringstream o;
  o << "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
    << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
    << "<title>" << html_escape(title) << "</title>\n"
    << "<meta name=\"description\" content=\"" << html_escape(description) << "\">\n"
    << "<link rel=\"icon\" href=\"" << up << "assets/icon.svg\">\n"
    << "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">\n"
    << "<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&family=JetBrains+Mono:wght@400;500;600&family=Newsreader:opsz,wght@6..72,400&display=swap\">\n"
    << "<link rel=\"stylesheet\" href=\"" << up << "assets/style.css\">\n"
    << "<script>try{var t=localStorage.getItem('rcpp.theme');if(t)document.documentElement.setAttribute('data-theme',t)}catch(e){}</script>\n"
    << "</head>\n<body>\n<header class=\"top\"><div class=\"in\">"
    << "<a class=\"brand\" href=\"" << up << "index.html\">robotics<b>cpp</b><span class=\"long\"> from scratch</span></a>"
    << "<nav class=\"links\">";
  const char* items[][2] = {{"roadmap", "Roadmap"}, {"playground", "Playground"}, {"atlas", "Failure Atlas"}, {"setup", "Setup"}};
  for (const auto& it : items)
    o << "<a href=\"" << up << it[0] << ".html\"" << (active == it[0] ? " class=\"on\"" : "") << ">" << it[1] << "</a>";
  o << "<a href=\"" << site.repo_url << "\">GitHub</a></nav></div></header>\n"
    << "<main" << (narrow ? " class=\"narrow\"" : "") << ">\n" << body << "\n</main>\n"
    << "<footer class=\"bottom\"><div class=\"in\"><div><p class=\"serif\">Every lesson is code you write, and a test that says whether it works.</p>"
    << "Free and open source. Clone it, build it, and run <code>rcpp start</code>. "
    << "<button class=\"theme-btn\" onclick=\"toggleTheme()\">light / dark</button></div>"
    << "<div class=\"right\"><a href=\"" << site.repo_url << "\">The repository on GitHub</a> \xc2\xb7 <a href=\"" << up << "atlas.html\">Failure Atlas</a> \xc2\xb7 <a href=\"" << up << "setup.html\">Setup</a><br>"
    << "Made from the lesson files by <code>rcpp site</code>, so the pages cannot disagree with the course.<br>"
    << "Your progress is kept in this browser only.</div></div></footer>\n";
  // lessons passed on this machine, when the pages are made locally
  if (site.progress && !site.progress->passed.empty()) {
    o << "<script>window.RCPP_PASSED=[";
    bool first = true;
    for (const std::string& id : site.progress->passed) {
      o << (first ? "" : ",") << "\"" << html_escape(id) << "\"";
      first = false;
    }
    o << "];</script>\n";
  }
  o << "<script src=\"" << up << "assets/site.js\"></script>\n</body>\n</html>\n";
  return o.str();
}

std::string quiz_html(const fs::path& quiz_file, const Links& links) {
  const auto text = read_file(quiz_file);
  if (!text) return "";
  const json::ParseResult parsed = json::parse(*text);
  if (!parsed.ok) return "";
  const json::Array& questions = parsed.value.at("questions").as_array();
  if (questions.empty()) return "";
  std::ostringstream o;
  o << "<p class=\"label\">Check yourself</p>\n<p class=\"dim\">Choose an answer to see whether it is right, and why.</p>\n";
  int n = 0;
  for (const json::Value& q : questions) {
    ++n;
    o << "<div class=\"q\" data-right=\"" << q.at("correct").as_int() << "\">\n<p><strong>" << n << ".</strong> "
      << inline_md(q.at("question").as_string_or(""), links) << "</p>\n";
    int i = 0;
    for (const json::Value& opt : q.at("options").as_array())
      o << "<button type=\"button\" data-i=\"" << i++ << "\">" << inline_md(opt.as_string_or(""), links) << "</button>\n";
    o << "<p class=\"why\">" << inline_md(q.at("explanation").as_string_or(""), links) << "</p>\n</div>\n";
  }
  return o.str();
}

// ---- the pages ------------------------------------------------------------------------------

std::string home_page(const Site& s) {
  int minutes = 0;
  for (const Lesson* l : s.all) minutes += l->minutes;
  std::ostringstream b;
  b << "<p class=\"eyebrow\">Robotics in C++ \xc2\xb7 " << s.all.size() << " lessons \xc2\xb7 $0</p>\n"
    << "<h1>Learn robotics in the language robots actually run on.</h1>\n"
    << "<p class=\"lede\">From your first compiled program to arms, ground vehicles, legged robots and robots that learn. "
    << "Every lesson is real C++ you write yourself, and a test suite that tells you when it works. No GPU, no robot, no fee.</p>\n"
    << "<div class=\"doors\">"
    << "<a class=\"door\" href=\"" << (s.all.empty() ? "roadmap.html" : "l/" + s.all.front()->id + ".html") << "\"><div class=\"small\">start at phase 0</div>"
    << "<div class=\"big\">I'm starting from zero \xe2\x86\x92</div><p>Install a compiler, build, and make a robot move in the terminal in the first hour.</p></a>"
    << "<a class=\"door paper\" href=\"start.html\"><div class=\"small\">2-minute self-check</div>"
    << "<div class=\"big\">I already code \xe2\x86\x92</div><p>Tick what you can already do, and it tells you which phase to start at.</p></a></div>\n";
  int lesson_count = static_cast<int>(s.all.size());
  b << "<div class=\"stats\">"
    << "<div><b>" << lesson_count << "</b><span>lessons, each with tests</span></div>"
    << "<div><b>" << s.catalog->phases.size() << "</b><span>phases in ten tracks</span></div>"
    << "<div><b>" << hours_text(minutes) << "</b><span>of hands-on material</span></div>"
    << "<div><b>" << s.atlas->entries.size() << "</b><span>errors explained</span></div>"
    << "<div><b>4</b><span>toolchains tested on every change</span></div></div>\n";
  b << "<a class=\"banner paper\" href=\"playground.html\"><div><div class=\"eyebrow\" style=\"margin:0 0 6px\">3 playgrounds in your browser</div>"
    << "<div class=\"t\">Drag a robot arm, tune a PID loop, train a robot by trial: the same maths as lessons you will write</div></div><span class=\"go\">Open the playground \xe2\x86\x92</span></a>\n"
    << "<a class=\"banner hot\" href=\"atlas.html\"><div><div class=\"eyebrow\" style=\"margin:0 0 6px\">The Failure Atlas</div>"
    << "<div class=\"t\">" << s.atlas->entries.size() << " things that go wrong, why, and the fix: search it, or pipe a compiler error into rcpp explain</div></div><span class=\"go\">Search the atlas \xe2\x86\x92</span></a>\n";

  // the path
  std::string all_ids;
  for (const Lesson* l : s.all) all_ids += l->id + " ";
  b << "<div class=\"path-head\"><h2 class=\"serif\" style=\"margin:0\">The path</h2>"
    << "<div class=\"prog\" data-progress=\"" << all_ids << "\"><div class=\"progress-head\"><span>your progress</span><span class=\"count\">0/" << lesson_count
    << "</span></div><div class=\"bar\"><i></i></div></div></div>\n<div class=\"track\">\n";
  int k = 0;
  for (const Track& t : kTracks) {
    int lessons = 0, mins = 0;
    std::string ids;
    for (const Phase& p : s.catalog->phases)
      if (p.num >= t.from && p.num <= t.to) {
        lessons += static_cast<int>(p.lessons.size());
        mins += phase_minutes(p);
        ids += lesson_ids(p);
      }
    if (lessons == 0) continue;
    ++k;
    b << "<a class=\"row\" href=\"roadmap.html#track-" << k << "\" data-progress=\"" << ids << "\"><span class=\"no\">T" << two_digits(k) << "</span>"
      << "<span><span class=\"ti\">" << t.name << "</span>" << (t.optional ? "<span class=\"tag\">optional</span>" : "")
      << "<br><span class=\"sub\">" << t.blurb << "</span></span>"
      << "<span class=\"meta\"><span class=\"done-n\"></span>" << (t.from == t.to ? "phase " + two_digits(t.from) : "phases " + two_digits(t.from) + "\xe2\x80\x93" + two_digits(t.to))
      << "<br>" << lessons << " lessons \xc2\xb7 " << hours_text(mins) << "</span></a>\n";
  }
  b << "</div>\n";

  // the argument
  b << "<div class=\"two\"><div><h2 class=\"serif\">Robotics is learned by building it, and the building is in C++.</h2>"
    << "<p class=\"dim\">Robot software is written in C++ where it has to be fast, predictable and close to the hardware: drivers, control loops, planners, the cores of ROS 2 and of the simulators. "
    << "This course teaches the language and the robotics together, on one robot problem after another, and checks every step with tests on four toolchains, Windows included.</p></div>"
    << "<ul class=\"cross\">"
    << "<li>Watch a lecture on kinematics, nod along, and never compute a Jacobian yourself.</li>"
    << "<li>Learn C++ from a book with no robot in it, and robotics from a course with no C++ in it.</li>"
    << "<li>Copy a tutorial that worked for its author, and spend the evening on an error it never mentioned.</li>"
    << "</ul></div>\n";

  // how a lesson works
  b << "<p class=\"label\" style=\"margin-top:72px\">How every lesson works</p>\n"
    << "<div class=\"steps\"><div><b>Read the problem</b><p>Each lesson starts from a robot problem, explains the idea, and quotes the numbers its own tests measured.</p></div>"
    << "<div><b>Write the code</b><p>Your part is marked <code>TODO</code> in the lesson's <code>exercise/solution.hpp</code>. The rest is already there, so you write the part that matters.</p></div>"
    << "<div><b>Let the tests judge it</b><p><code>rcpp verify</code> builds your code and runs the lesson's real tests. Green means it works; red names the test and the numbers.</p></div></div>\n"
    << "<div class=\"term\"><div class=\"bar3\"><i></i><i></i><i></i></div><pre><code>"
    << "<span class=\"c\">$</span> rcpp verify 13-02\n"
    << "  <span class=\"no\">FAIL</span>  the elbow angle comes back from the hand's position\n"
    << "        expected q2 = 1.047, got 0.000\n"
    << "<span class=\"c\">$</span> <span class=\"c\"># ...write the inverse kinematics...</span>\n"
    << "<span class=\"c\">$</span> rcpp verify 13-02\n"
    << "  <span class=\"ok\">pass</span>  the elbow angle comes back from the hand's position\n"
    << "  <span class=\"ok\">pass</span>  a point out of reach is refused, not stretched toward\n"
    << "  <span class=\"ok\">passed</span>  13-02, next: rcpp next</code></pre></div>\n"
    << "<p class=\"dim\" style=\"font-size:14px\">An illustration of the loop; each lesson's page shows its own tests.</p>\n";

  // what is different
  b << "<p class=\"label\" style=\"margin-top:60px\">What you get that a playlist cannot give you</p>\n<div class=\"cards four\">"
    << "<div class=\"card\"><div class=\"k\">tested</div><div class=\"h\">Code that compiles everywhere</div><p>Every lesson builds and passes its tests on GCC, Clang and MSVC, under the sanitizers, on every change.</p></div>"
    << "<div class=\"card\"><div class=\"k\">honest</div><div class=\"h\">Numbers that are measured</div><p>Every figure on a page is printed by the lesson's own test and pinned there, so the text cannot drift from the code.</p></div>"
    << "<div class=\"card\"><div class=\"k\">kind to mistakes</div><div class=\"h\">An atlas of errors</div><p>" << s.atlas->entries.size()
    << " entries: what you see, why it happens, and the fix, each linked to the lesson that teaches it.</p></div>"
    << "<div class=\"card\"><div class=\"k\">yours</div><div class=\"h\">A library you built</div><p>What you write graduates into <code>librc</code>, a robotics library in plain C++ that the later lessons use.</p></div></div>\n";

  b << "<div class=\"box hot\" style=\"margin-top:56px\"><p class=\"k\">Start in five minutes</p>"
    << "<pre><code>git clone " << s.repo_url << ".git\ncd robotics-cpp-from-scratch\ncmake --preset default\ncmake --build build/default --target rcpp\n./build/default/bin/rcpp doctor     # checks your machine, prints the fix for anything missing\n./build/default/bin/rcpp start      # opens lesson 00-01</code></pre>"
    << "<a class=\"btn\" href=\"setup.html\">Setup for Linux, Windows and macOS \xe2\x86\x92</a></div>\n";
  return page(s, "Robotics C++ From Scratch: learn robotics in the language robots run on",
              "A free course in robotics and C++: " + std::to_string(lesson_count) + " lessons of code you write yourself, each checked by tests, from a first program to arms, vehicles, legged robots and robots that learn.",
              "", "", b.str(), false);
}

std::string roadmap_page(const Site& s) {
  std::ostringstream b;
  int minutes = 0;
  for (const Lesson* l : s.all) minutes += l->minutes;
  std::string all_ids;
  for (const Lesson* l : s.all) all_ids += l->id + " ";
  b << "<p class=\"eyebrow\">The full curriculum</p>\n<h1 class=\"page\">Ten tracks, one page</h1>\n"
    << "<p class=\"lede\">" << s.all.size() << " lessons in " << s.catalog->phases.size() << " phases, about " << (minutes + 30) / 60
    << " hours of hands-on work. Roughly an hour a day: read a lesson, write its code, run its tests.</p>\n"
    << "<div data-progress=\"" << all_ids << "\" style=\"max-width:420px\"><div class=\"progress-head\"><span>whole path</span><span class=\"count\"></span></div><div class=\"bar\"><i></i></div></div>\n";
  int k = 0;
  for (const Track& t : kTracks) {
    bool any = false;
    for (const Phase& p : s.catalog->phases)
      if (p.num >= t.from && p.num <= t.to && !p.lessons.empty()) any = true;
    if (!any) continue;
    ++k;
    b << "<h2 class=\"serif\" id=\"track-" << k << "\" style=\"margin-top:56px\"><span class=\"mono dim\" style=\"font-size:15px;margin-right:10px\">T" << two_digits(k)
      << "</span>" << t.name << (t.optional ? " <span class=\"tag\">optional</span>" : "") << "</h2>\n<p class=\"dim\" style=\"margin-top:-6px\">" << t.blurb
      << "</p>\n<div class=\"track\">\n";
    for (const Phase& p : s.catalog->phases) {
      if (p.num < t.from || p.num > t.to || p.lessons.empty()) continue;
      b << "<a class=\"row\" href=\"" << phase_page(p) << "\" data-progress=\"" << lesson_ids(p) << "\"><span class=\"no\">P" << two_digits(p.num) << "</span>"
        << "<span><span class=\"ti\">" << html_escape(p.title) << "</span><br><span class=\"sub\">" << html_escape(p.ends_with) << ".</span></span>"
        << "<span class=\"meta\"><span class=\"done-n\"></span>" << p.lessons.size() << " lessons<br>" << hours_text(phase_minutes(p)) << "</span></a>\n";
    }
    b << "</div>\n";
  }
  b << "<p class=\"label\" style=\"margin-top:64px\">How to actually finish this</p>\n<ul class=\"arrow\">"
    << "<li>Never read two lessons without writing the code of one. The tests are the course; the text is support.</li>"
    << "<li>When a test fails, read the failure before the code: it names the test and prints the number it expected beside the one it got.</li>"
    << "<li>Stuck on an error message? Pipe it into <code>rcpp explain</code>, or search the <a href=\"atlas.html\">Failure Atlas</a>.</li>"
    << "<li>Only after your version passes, read <code>reference/solution.hpp</code> beside it. Where the two differ and both pass, the difference is worth understanding.</li>"
    << "<li>Run <code>rcpp next</code> to see what is unlocked by what you have passed, and <code>rcpp status</code> for how far you have come.</li></ul>\n";
  return page(s, "Roadmap: Robotics C++ From Scratch", "The whole curriculum on one page: ten tracks from first C++ to robots that learn.", "", "roadmap", b.str(), false);
}

std::string phase_html(const Site& s, std::size_t index) {
  const Phase& p = s.catalog->phases[index];
  const Track* track = nullptr;
  for (const Track& t : kTracks)
    if (p.num >= t.from && p.num <= t.to) track = &t;
  std::ostringstream b;
  b << "<div class=\"crumbs\"><a href=\"../roadmap.html\">\xe2\x86\x90 all phases</a>" << (track ? std::string(" \xc2\xb7 ") + track->name : "") << "</div>\n"
    << "<p class=\"eyebrow\">Phase " << two_digits(p.num) << " \xc2\xb7 " << p.lessons.size() << " lessons \xc2\xb7 " << hours_text(phase_minutes(p)) << "</p>\n"
    << "<h1 class=\"page\">" << html_escape(p.title) << "</h1>\n";
  if (!p.ends_with.empty())
    b << "<p class=\"lede\"><span class=\"dim\">What you will have at the end:</span> " << html_escape(p.ends_with) << ".</p>\n";
  b << "<div data-progress=\"" << lesson_ids(p) << "\"><div class=\"progress-head\"><span>phase " << two_digits(p.num)
    << "</span><span class=\"count\"></span></div><div class=\"bar\"><i></i></div></div>\n";
  b << "<p class=\"label\">The lessons</p>\n<ul class=\"checks\">\n";
  for (const Lesson& l : p.lessons) {
    b << "<li><input type=\"checkbox\" data-lesson-check=\"" << l.id << "\" aria-label=\"done\"><span class=\"n\">" << lesson_number(l) << "</span>"
      << "<a href=\"../l/" << l.id << ".html\">" << html_escape(l.title) << "</a><span class=\"m\">" << l.minutes << " min</span></li>\n";
  }
  b << "</ul>\n";
  // what graduates into the library
  std::vector<std::string> headers;
  for (const Lesson& l : p.lessons) {
    const std::string h = l.raw.at("artifact").at("header").as_string_or("");
    if (!h.empty()) headers.push_back(h);
  }
  if (!headers.empty()) {
    b << "<p class=\"label\">What you build</p>\n<p class=\"dim\">Each lesson's finished code graduates into the course's library, where the later lessons use it:</p>\n<p>";
    for (std::size_t i = 0; i < headers.size(); ++i) b << (i ? " " : "") << "<code>" << html_escape(headers[i]) << "</code>";
    b << "</p>\n";
  }
  for (const Play& pl : kPlays)
    for (const char* ln : pl.lessons)
      if (std::string(ln).substr(0, 2) == two_digits(p.num)) {
        b << "<a class=\"banner hot\" style=\"margin-top:28px\" href=\"../play/" << pl.slug << ".html\"><div><div class=\"eyebrow\" style=\"margin:0 0 6px\">Warm up in the playground</div>"
          << "<div class=\"t\">" << pl.title << "</div></div><span class=\"go\">Open \xe2\x86\x92</span></a>\n";
        goto played;
      }
played:
  b << "<div class=\"pager\"><span>";
  if (index > 0) {
    const Phase& q = s.catalog->phases[index - 1];
    b << "<a href=\"" << q.slug << ".html\">\xe2\x86\x90 P" << two_digits(q.num) << ": " << html_escape(q.title) << "</a>";
  }
  b << "</span><span>";
  if (index + 1 < s.catalog->phases.size()) {
    const Phase& q = s.catalog->phases[index + 1];
    b << "<a href=\"" << q.slug << ".html\">P" << two_digits(q.num) << ": " << html_escape(q.title) << " \xe2\x86\x92</a>";
  }
  b << "</span></div>\n";
  return page(s, "Phase " + two_digits(p.num) + ": " + p.title + " \xc2\xb7 Robotics C++ From Scratch", p.ends_with, "../", "roadmap", b.str(), true);
}

std::string lesson_html(const Site& s, std::size_t k, const Links& sub) {
  const Lesson& l = *s.all[k];
  const Phase* ph = s.phase_of.count(l.phase_num) ? s.phase_of.at(l.phase_num) : nullptr;
  std::ostringstream b;
  b << "<div class=\"crumbs\"><a href=\"../roadmap.html\">roadmap</a>";
  if (ph) b << " / <a href=\"../" << phase_page(*ph) << "\">phase " << two_digits(ph->num) << ", " << html_escape(ph->title) << "</a>";
  b << "</div>\n<p class=\"eyebrow\">Lesson " << lesson_number(l) << " \xc2\xb7 about " << l.minutes << " minutes</p>\n"
    << "<h1 class=\"page\">" << html_escape(l.title) << "</h1>\n";
  const auto docs = read_file(l.path / "docs" / "en.md");
  b << "<div class=\"prose\">" << (docs ? markdown_to_html(*docs, sub) : "<p>This lesson has no page yet.</p>\n") << "</div>\n";
  b << "<div class=\"box hot\"><p class=\"k\">Your turn</p>"
    << "Write your part in <code>" << l.rel_path << "/exercise/solution.hpp</code>, then, from the top folder of the course:"
    << "<pre><code>rcpp verify " << lesson_number(l) << "</code></pre>"
    << "<span class=\"dim\">The finished version is in <code>reference/solution.hpp</code>; read it after yours passes. On GitHub: "
    << "<a href=\"" << s.repo_url << "/blob/main/" << l.rel_path << "/exercise/solution.hpp\">the exercise</a>, "
    << "<a href=\"" << s.repo_url << "/tree/main/" << l.rel_path << "/tests\">its tests</a>.</span><br><br>"
    << "<label class=\"done-toggle\"><input type=\"checkbox\" data-lesson-check=\"" << l.id << "\"> I have done this lesson</label>";
  if (s.progress && s.progress->has(l.id)) b << " <span class=\"passed\">passed with rcpp verify</span>";
  b << "</div>\n";
  if (!l.requires_ids.empty()) {
    b << "<p class=\"label\">Before this one</p>\n<ul class=\"plain\">";
    for (const std::string& r : l.requires_ids) {
      if (s.by_id.count(r))
        b << "<li><span class=\"num\">" << r.substr(0, 5) << "</span><a href=\"" << r << ".html\">" << html_escape(s.by_id.at(r)->title) << "</a></li>";
      else
        b << "<li>" << html_escape(r) << "</li>";
    }
    b << "</ul>\n";
  }
  if (!l.breaks_first.empty()) {
    b << "<p class=\"label\">What breaks first here</p>\n<ul class=\"plain\">";
    for (const std::string& id : l.breaks_first) {
      if (!s.atlas_by_id.count(id)) continue;
      b << "<li><span class=\"num\">" << id << "</span><a href=\"../e/" << id << ".html\">" << html_escape(s.atlas_by_id.at(id)->title) << "</a></li>";
    }
    b << "</ul>\n";
  }
  b << quiz_html(l.path / "quiz.json", sub);
  b << "<div class=\"pager\"><span>";
  if (k > 0) b << "<a href=\"" << s.all[k - 1]->id << ".html\">\xe2\x86\x90 " << lesson_number(*s.all[k - 1]) << " " << html_escape(s.all[k - 1]->title) << "</a>";
  b << "</span><span>";
  if (k + 1 < s.all.size()) b << "<a href=\"" << s.all[k + 1]->id << ".html\">" << lesson_number(*s.all[k + 1]) << " " << html_escape(s.all[k + 1]->title) << " \xe2\x86\x92</a>";
  b << "</span></div>\n";
  return page(s, lesson_number(l) + " " + l.title + " \xc2\xb7 Robotics C++ From Scratch", l.title, "../", "roadmap", b.str(), true);
}

std::string start_page(const Site& s) {
  std::ostringstream b;
  b << "<p class=\"eyebrow\">For people who already code</p>\n<h1 class=\"page\">Where should you start?</h1>\n"
    << "<p class=\"lede\">Tick only what you could do today, without looking anything up. Be honest: skipping a phase you are not ready for costs more time than repeating one you know. "
    << "The Qt phases (09 to 12) are left out: they are optional, and you can take them whenever you need an interface.</p>\n<ul class=\"checks\">\n";
  for (const Can& c : kCan)
    b << "<li><input type=\"checkbox\" data-can=\"" << two_digits(c.phase) << "\" aria-label=\"I can\"><span class=\"n\">P" << two_digits(c.phase)
      << "</span><span>I can " << c.text << "</span><span></span></li>\n";
  b << "</ul>\n<p style=\"margin-top:22px\"><button class=\"btn\" onclick=\"selfCheck()\">Show my starting phase</button></p>\n"
    << "<div id=\"verdict\" class=\"box\" style=\"display:none\" data-last=\"22\"></div>\n";
  b << "<p class=\"label\">Or pick a phase yourself</p>\n<ul class=\"plain\">";
  for (const Phase& p : s.catalog->phases)
    b << "<li><span class=\"num\">P" << two_digits(p.num) << "</span><a href=\"" << phase_page(p) << "\" data-phase-link=\"" << two_digits(p.num)
      << "\" data-title=\"" << html_escape(p.title) << "\">" << html_escape(p.title) << "</a></li>";
  b << "</ul>\n";
  return page(s, "Where should you start? \xc2\xb7 Robotics C++ From Scratch", "A two-minute self-check that says which phase to start at.", "", "", b.str(), true);
}

std::string playground_page(const Site& s) {
  std::ostringstream b;
  b << "<p class=\"eyebrow\">Playgrounds \xc2\xb7 free \xc2\xb7 in your browser</p>\n<h1 class=\"page\">Robotics you can touch.</h1>\n"
    << "<p class=\"lede\">Three ideas from the course, small enough to play with in a minute. Each runs the same maths as a lesson you will write in C++, so break it here, then go and build it there.</p>\n"
    << "<div class=\"cards\">";
  int i = 0;
  for (const Play& p : kPlays)
    b << "<a class=\"card\" href=\"play/" << p.slug << ".html\"><div class=\"big-no\">" << two_digits(++i) << "</div><div class=\"k\">" << p.kicker << "</div>"
      << "<div class=\"h\">" << p.title << "</div><p>" << p.blurb << "</p><span class=\"open\">Open \xe2\x86\x92</span></a>";
  b << "</div>\n";
  return page(s, "Playground \xc2\xb7 Robotics C++ From Scratch", "Drag a robot arm, tune a PID loop, train a robot by trial, in your browser.", "", "playground", b.str(), false);
}

std::string play_html(const Site& s, const Play& p) {
  std::ostringstream b;
  b << "<div class=\"crumbs\"><a href=\"../playground.html\">\xe2\x86\x90 playground</a></div>\n<p class=\"eyebrow\">" << p.kicker << "</p>\n<h1 class=\"page\">" << p.title << "</h1>\n"
    << "<p class=\"lede\">" << p.blurb << "</p>\n";
  const std::string slug = p.slug;
  if (slug == "arm") {
    b << "<div class=\"play\"><canvas id=\"arm\"></canvas><div class=\"panel\"><p class=\"k\">Drag the target</p>"
      << "<div class=\"read\"><div><span>x</span><span id=\"r-x\"></span></div><div><span>y</span><span id=\"r-y\"></span></div>"
      << "<div><span>q1</span><span id=\"r-q1\"></span></div><div><span>q2</span><span id=\"r-q2\"></span></div>"
      << "<div><span>det J = l1 l2 sin q2</span><span id=\"r-det\"></span></div></div>"
      << "<p id=\"r-msg\" style=\"margin:12px 0 4px\"></p><button class=\"btn ghost\" id=\"elbow\">Elbow: right</button><button class=\"btn ghost\" id=\"straight\">Stretch it straight</button>"
      << "<p class=\"k\">The arm</p><p class=\"dim\" style=\"margin:0\">Two links of 0.225 m, joint limits of 2.35 and 2.6 rad, as the SCARA of 32-01. Seen from above; the dashed circle is its reach.</p></div></div>\n";
  } else if (slug == "pid") {
    b << "<div class=\"play\"><canvas id=\"pid\"></canvas><div class=\"panel\"><p class=\"k\">The gains</p>"
      << "<label>Kp <span class=\"mono\" id=\"v-kp\"></span></label><input type=\"range\" id=\"kp\" min=\"0\" max=\"8\" step=\"0.05\" value=\"3\">"
      << "<label>Ki <span class=\"mono\" id=\"v-ki\"></span></label><input type=\"range\" id=\"ki\" min=\"0\" max=\"6\" step=\"0.05\" value=\"0\">"
      << "<label>Kd <span class=\"mono\" id=\"v-kd\"></span></label><input type=\"range\" id=\"kd\" min=\"0\" max=\"0.6\" step=\"0.005\" value=\"0.02\">"
      << "<label class=\"done-toggle\" style=\"margin-top:10px\"><input type=\"checkbox\" id=\"push\"> push it at 2 s</label>"
      << "<p class=\"k\">What the step did</p><div class=\"read\"><div><span>overshoot</span><span id=\"r-os\"></span></div><div><span>settled within 2 %</span><span id=\"r-ts\"></span></div>"
      << "<div><span>time at the torque limit</span><span id=\"r-sat\"></span></div><div><span>error left</span><span id=\"r-ss\"></span></div></div>"
      << "<p id=\"r-msg\" style=\"margin:12px 0 4px\"></p><button class=\"btn ghost\" id=\"preset\">A tuned set</button></div></div>\n";
  } else {
    b << "<div class=\"play\"><canvas id=\"grid\"></canvas><div class=\"panel\"><p class=\"k\">The task</p>"
      << "<label>slip <span class=\"mono\" id=\"v-slip\"></span></label><input type=\"range\" id=\"slip\" min=\"0\" max=\"0.3\" step=\"0.01\" value=\"0.1\">"
      << "<label>discount <span class=\"mono\" id=\"v-gamma\"></span></label><input type=\"range\" id=\"gamma\" min=\"0.5\" max=\"0.99\" step=\"0.01\" value=\"0.95\">"
      << "<p class=\"k\">The learner</p>"
      << "<label>random moves (epsilon) <span class=\"mono\" id=\"v-eps\"></span></label><input type=\"range\" id=\"eps\" min=\"0\" max=\"0.6\" step=\"0.01\" value=\"0.1\">"
      << "<label>starting values <span class=\"mono\" id=\"v-q0\"></span></label><input type=\"range\" id=\"q0\" min=\"0\" max=\"20\" step=\"1\" value=\"0\">"
      << "<div><button class=\"btn ghost\" id=\"t1\">1 episode</button><button class=\"btn ghost\" id=\"t100\">100</button><button class=\"btn ghost\" id=\"t1000\">1000</button><button class=\"btn ghost\" id=\"reset\">Reset</button></div>"
      << "<p class=\"k\">So far</p><div class=\"read\"><div><span>episodes</span><span id=\"r-ep\"></span></div><div><span>reached the goal</span><span id=\"r-goal\"></span></div>"
      << "<div><span>fell into a pit</span><span id=\"r-pit\"></span></div><div><span>from the start it goes</span><span id=\"r-route\"></span></div></div></div></div>\n"
      << "<p class=\"dim\">Every step costs 1, the goal is worth 10 and a pit costs 10. Each move slips to either side with the chance set above. Arrows show the move the learner rates best in each cell; the orange line is its last run. "
      << "Every pit it falls into while learning is a real failure: on a robot, a fall.</p>\n";
  }
  b << "<p class=\"label\">Build it in C++</p>\n<ul class=\"plain\">";
  for (const char* ln : p.lessons) {
    if (!s.by_number.count(ln)) continue;
    const Lesson* l = s.by_number.at(ln);
    b << "<li><span class=\"num\">" << ln << "</span><a href=\"../l/" << l->id << ".html\">" << html_escape(l->title) << "</a></li>";
  }
  b << "</ul>\n<script src=\"../assets/" << p.script << "\"></script>\n";
  return page(s, std::string(p.title) + " \xc2\xb7 Robotics C++ From Scratch", p.blurb, "../", "playground", b.str(), false);
}

std::string atlas_index(const Site& s) {
  std::ostringstream b;
  b << "<p class=\"eyebrow\">The Failure Atlas</p>\n<h1 class=\"page\">What went wrong, why, and the fix.</h1>\n"
    << "<p class=\"lede\">" << s.atlas->entries.size() << " entries, each a mistake that real robot code makes, with what you see, the cause and the fix, and the lesson that teaches it. "
    << "Type a word from your error or your symptom. From a terminal, <code>your-build 2&gt;&amp;1 | rcpp explain</code> finds the entry for you.</p>\n"
    << "<input class=\"filter\" type=\"search\" placeholder=\"Search the atlas: a word from the error, the symptom or the title\" oninput=\"filterList(this)\" autofocus>\n<ul class=\"plain\">\n";
  for (const AtlasEntry& e : s.atlas->entries)
    b << "<li data-text=\"" << html_escape(lower(e.id + " " + e.title + " " + e.symptom)) << "\"><span class=\"num\">" << e.id << "</span><a href=\"e/" << e.id
      << ".html\">" << html_escape(e.title) << "</a></li>\n";
  b << "</ul>\n";
  return page(s, "Failure Atlas \xc2\xb7 Robotics C++ From Scratch", "Mistakes robot code makes, why, and the fix, each linked to the lesson that teaches it.", "", "atlas", b.str(), true);
}

std::string atlas_entry(const Site& s, const AtlasEntry& e, const Links& sub) {
  const auto text = read_file(e.path);
  std::string body_md = text ? *text : "";
  const std::size_t sep = body_md.find("\n---\n");
  body_md = sep == std::string::npos ? "" : body_md.substr(sep + 5);
  std::ostringstream b;
  b << "<div class=\"crumbs\"><a href=\"../atlas.html\">\xe2\x86\x90 failure atlas</a></div>\n<p class=\"eyebrow\">" << e.id << "</p>\n<h1 class=\"page\">"
    << html_escape(e.title) << "</h1>\n<div class=\"prose\">" << markdown_to_html(body_md, sub) << "</div>\n";
  if (s.by_id.count(e.teaches))
    b << "<div class=\"box\"><p class=\"k\">Taught in</p><a href=\"../l/" << e.teaches << ".html\">" << html_escape(e.teaches.substr(0, 5) + " " + s.by_id.at(e.teaches)->title)
      << "</a><br><span class=\"dim\">From a terminal, <code>rcpp explain " << e.id << "</code> prints this entry.</span></div>\n";
  return page(s, e.id + " " + e.title + " \xc2\xb7 Failure Atlas", e.title, "../", "atlas", b.str(), true);
}

std::string setup_page(const Site& s) {
  std::ostringstream b;
  b << "<p class=\"eyebrow\">Setup</p>\n<h1 class=\"page\">From nothing to your first passing test.</h1>\n"
    << "<p class=\"lede\">You need a C++ compiler, CMake and Git. That is all the first phases ask for; later phases say what else they need, and every lesson runs in software first.</p>\n"
    << "<p class=\"label\">Ubuntu 22.04 or 24.04</p>\n<pre><code>sudo apt update && sudo apt install -y build-essential cmake ninja-build git gdb</code></pre>\n"
    << "<p class=\"label\">Windows 11</p>\n<p class=\"dim\">Install Visual Studio 2022 (the \"Desktop development with C++\" workload, which brings MSVC and CMake) and Git for Windows, then use the Developer PowerShell. "
    << "WSL2 with Ubuntu also works, and is the route into ROS 2 on Windows.</p>\n"
    << "<p class=\"label\">macOS</p>\n<p class=\"dim\">Install the Xcode command line tools (<code>xcode-select --install</code>) and CMake. macOS is not one of the four toolchains tested on every change, so expect to fix the odd thing yourself.</p>\n"
    << "<p class=\"label\">Then, everywhere</p>\n<pre><code>git clone " << s.repo_url << ".git\ncd robotics-cpp-from-scratch\ncmake --preset default\ncmake --build build/default --target rcpp\n./build/default/bin/rcpp doctor</code></pre>\n"
    << "<p class=\"dim\"><code>rcpp doctor</code> checks your machine and prints the exact command that fixes anything missing, for your operating system.</p>\n"
    << "<p class=\"label\">The tool you will use every day</p>\n<div class=\"prose\"><div class=\"table\"><table>"
    << "<tr><th>Command</th><th>What it does</th></tr>"
    << "<tr><td><code>rcpp start</code></td><td>Says which lesson to open first</td></tr>"
    << "<tr><td><code>rcpp verify 13-02</code></td><td>Builds your exercise and runs the lesson's real tests</td></tr>"
    << "<tr><td><code>rcpp next</code></td><td>Shows what is unlocked by what you have passed</td></tr>"
    << "<tr><td><code>rcpp status</code></td><td>Shows how far you have got, phase by phase</td></tr>"
    << "<tr><td><code>rcpp explain</code></td><td>Looks an error up in the Failure Atlas: pipe a build into it</td></tr>"
    << "<tr><td><code>rcpp site</code></td><td>Writes these pages to <code>site/</code>, with your own progress in them</td></tr>"
    << "</table></div></div>\n"
    << "<p class=\"label\">Hardware</p>\n<p class=\"dim\">None is needed. Every lesson passes in software or a simulator; about a third have an optional section for cheap hardware or a real robot, always with a fake of the device so the tests still run.</p>\n";
  return page(s, "Setup \xc2\xb7 Robotics C++ From Scratch", "Install a compiler, CMake and Git, clone the course and run your first test.", "", "setup", b.str(), true);
}

const char* kIcon =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 32 32\"><rect width=\"32\" height=\"32\" rx=\"7\" fill=\"#121110\"/>"
    "<path d=\"M8 23 L14 13 L22 10\" stroke=\"#eeeae2\" stroke-width=\"3\" fill=\"none\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>"
    "<circle cx=\"8\" cy=\"23\" r=\"2.6\" fill=\"#f2804a\"/><circle cx=\"14\" cy=\"13\" r=\"2.6\" fill=\"#f2804a\"/></svg>\n";

}  // namespace

int cmd_site(const Args& args) {
  if (!reject_unknown(args, {"--out", "--public"}, "site")) return 2;
  const auto root = find_repo_root();
  if (!root) { std::cerr << "rcpp site: run this from inside the repository\n"; return 2; }
  std::string out_text;
  if (!read_option(args, "--out", out_text, "site")) return 2;
  const fs::path out = out_text.empty() ? *root / "site" : fs::path(out_text);
  // --public leaves this machine's progress out, for pages that will be put online
  const bool is_public = args.has("--public");

  const Catalog catalog = load_catalog(*root);
  const Atlas atlas = load_atlas(*root);
  const Progress progress = is_public ? Progress() : load_progress(*root);

  Site site;
  site.catalog = &catalog;
  site.atlas = &atlas;
  site.progress = &progress;
  site.all = catalog.all();
  for (const Lesson* l : site.all) {
    site.by_id[l->id] = l;
    site.by_number[lesson_number(*l)] = l;
  }
  for (const AtlasEntry& e : atlas.entries) site.atlas_by_id[e.id] = &e;
  for (const Phase& p : catalog.phases) site.phase_of[p.num] = &p;

  Links top;
  for (const Lesson* l : site.all) top.lesson_by_prefix[lesson_number(*l)] = l->id;
  for (const AtlasEntry& e : atlas.entries) top.atlas_ids[e.id] = true;
  Links sub = top;
  sub.up = "../";

  std::error_code ec;
  for (const char* d : {"l", "e", "p", "play", "assets"}) fs::create_directories(out / d, ec);
  if (ec) { std::cerr << "rcpp site: cannot create " << out.string() << ": " << ec.message() << "\n"; return 1; }

  bool ok = true;
  for (const char* asset : {"style.css", "site.js", "play-arm.js", "play-pid.js", "play-q.js"}) {
    const auto text = read_file(*root / "tools" / "site" / asset);
    if (!text) { std::cerr << "rcpp site: missing tools/site/" << asset << "\n"; return 1; }
    ok = write_file(out / "assets" / asset, *text) && ok;
  }
  ok = write_file(out / "assets" / "icon.svg", kIcon) && ok;

  int pages = 0;
  auto put = [&](const fs::path& p, const std::string& html) { ok = write_file(p, html) && ok; ++pages; };
  put(out / "index.html", home_page(site));
  put(out / "roadmap.html", roadmap_page(site));
  put(out / "start.html", start_page(site));
  put(out / "playground.html", playground_page(site));
  put(out / "atlas.html", atlas_index(site));
  put(out / "setup.html", setup_page(site));
  for (const Play& p : kPlays) put(out / "play" / (std::string(p.slug) + ".html"), play_html(site, p));
  for (std::size_t i = 0; i < catalog.phases.size(); ++i)
    if (!catalog.phases[i].lessons.empty()) put(out / "p" / (catalog.phases[i].slug + ".html"), phase_html(site, i));
  for (std::size_t k = 0; k < site.all.size(); ++k) put(out / "l" / (site.all[k]->id + ".html"), lesson_html(site, k, sub));
  for (const AtlasEntry& e : atlas.entries) put(out / "e" / (e.id + ".html"), atlas_entry(site, e, sub));

  if (!ok) { std::cerr << "rcpp site: some pages could not be written under " << out.string() << "\n"; return 1; }
  std::cout << "wrote " << pages << " pages to " << out.string() << (is_public ? " (without this machine's progress)" : "") << "\n"
            << "open " << (out / "index.html").string() << " in a web browser\n";
  return 0;
}

}  // namespace rcpp
