// rcpp site
//
// Writes the course as plain web pages, for anyone who would rather read it in a
// browser than in markdown files: a home page with every phase, a page for every
// lesson (its text, its files, the command that checks it and its quiz to try),
// and a page for every entry of the Failure Atlas. Nothing is served and nothing
// is downloaded: the pages are files, opened from disk, and they work offline.
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
  return out;
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

const char* kStyle = R"(:root{--bg:#fbfbf8;--fg:#1d1d1b;--dim:#6b6b66;--line:#e2e1da;--box:#f1f0ea;--link:#1f5fa8;--ok:#2e7d32;--bad:#b3261e}
@media (prefers-color-scheme:dark){:root{--bg:#18181a;--fg:#e8e6e1;--dim:#9a988f;--line:#333336;--box:#222225;--link:#7fb0ef;--ok:#7cc47f;--bad:#f08a80}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font-family:system-ui,-apple-system,"Segoe UI","Noto Sans","DejaVu Sans",Helvetica,Arial,sans-serif;font-size:17px;line-height:1.6;font-style:normal}
main{max-width:820px;margin:0 auto;padding:24px 16px 80px}
a{color:var(--link)}
nav.crumbs{font-size:14px;color:var(--dim);margin-bottom:12px}
nav.crumbs a{color:var(--dim)}
h1{font-size:28px;line-height:1.25;margin:8px 0 12px}
h2{font-size:21px;margin-top:36px;border-top:1px solid var(--line);padding-top:20px}
h3{font-size:18px;margin-top:26px}
code{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:.88em;background:var(--box);padding:1px 4px;border-radius:4px}
pre{background:var(--box);padding:12px 14px;border-radius:6px;overflow-x:auto;line-height:1.45}
pre code{background:none;padding:0}
blockquote{margin:12px 0;padding:4px 16px;border-left:3px solid var(--line);color:var(--dim);font-style:italic}
.table{overflow-x:auto}
table{border-collapse:collapse;margin:12px 0;font-size:15px}
th,td{border:1px solid var(--line);padding:6px 10px;text-align:left;vertical-align:top}
th{background:var(--box)}
.box{background:var(--box);border-radius:8px;padding:14px 16px;margin:18px 0}
.dim{color:var(--dim)}
.done{color:var(--ok);font-weight:600}
ul.lessons{list-style:none;padding:0}
ul.lessons li{padding:7px 0;border-bottom:1px solid var(--line)}
ul.lessons .num{display:inline-block;min-width:3.4em;color:var(--dim);font-variant-numeric:tabular-nums}
details{border-bottom:1px solid var(--line);padding:8px 0}
summary{cursor:pointer;font-weight:600}
input.filter{width:100%;font-size:16px;padding:9px 12px;border:1px solid var(--line);border-radius:6px;background:var(--bg);color:var(--fg);margin:8px 0 16px}
.pager{display:flex;justify-content:space-between;gap:12px;margin-top:40px;font-size:15px}
.q{margin:18px 0;padding:14px 16px;border:1px solid var(--line);border-radius:8px}
.q button{display:block;width:100%;text-align:left;margin:6px 0;padding:8px 12px;font:inherit;font-size:16px;border:1px solid var(--line);border-radius:6px;background:var(--bg);color:var(--fg);cursor:pointer}
.q button.right{border-color:var(--ok);color:var(--ok)}
.q button.wrong{border-color:var(--bad);color:var(--bad)}
.q .why{display:none;margin-top:8px;color:var(--dim)}
.q.answered .why{display:block}
)";

const char* kScript = R"(document.addEventListener('click',function(e){var b=e.target.closest('.q button');if(!b)return;var q=b.closest('.q');if(q.classList.contains('answered'))return;var right=q.getAttribute('data-right');q.querySelectorAll('button').forEach(function(x){if(x.getAttribute('data-i')===right)x.classList.add('right');});if(b.getAttribute('data-i')!==right)b.classList.add('wrong');q.classList.add('answered');});
function filterList(box){var t=box.value.toLowerCase().trim();document.querySelectorAll('[data-text]').forEach(function(li){li.style.display=(!t||li.getAttribute('data-text').indexOf(t)>=0)?'':'none';});document.querySelectorAll('details').forEach(function(d){if(!t){d.open=false;return;}var any=false;d.querySelectorAll('[data-text]').forEach(function(li){if(li.style.display!=='none')any=true;});d.style.display=any?'':'none';d.open=any;});}
)";

std::string page(const std::string& title, const std::string& up, const std::string& crumbs, const std::string& body) {
  std::ostringstream o;
  o << "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
    << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
    << "<title>" << html_escape(title) << "</title>\n"
    << "<link rel=\"stylesheet\" href=\"" << up << "style.css\">\n</head>\n<body>\n<main>\n";
  if (!crumbs.empty()) o << "<nav class=\"crumbs\">" << crumbs << "</nav>\n";
  o << body << "\n</main>\n<script src=\"" << up << "site.js\"></script>\n</body>\n</html>\n";
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
  o << "<h2>Try the quiz</h2>\n<p class=\"dim\">Choose an answer to see whether it is right, and why.</p>\n";
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

std::string lesson_number(const Lesson& l) { return l.id.substr(0, 5); }

}  // namespace

int cmd_site(const Args& args) {
  if (!reject_unknown(args, {"--out"}, "site")) return 2;
  const auto root = find_repo_root();
  if (!root) { std::cerr << "rcpp site: run this from inside the repository\n"; return 2; }
  std::string out_text;
  if (!read_option(args, "--out", out_text, "site")) return 2;
  const fs::path out = out_text.empty() ? *root / "site" : fs::path(out_text);

  const Catalog catalog = load_catalog(*root);
  const Atlas atlas = load_atlas(*root);
  const Progress progress = load_progress(*root);

  Links top;
  for (const Lesson* l : catalog.all()) top.lesson_by_prefix[lesson_number(*l)] = l->id;
  for (const AtlasEntry& e : atlas.entries) top.atlas_ids[e.id] = true;
  Links sub = top;
  sub.up = "../";

  std::error_code ec;
  fs::create_directories(out / "l", ec);
  fs::create_directories(out / "e", ec);
  if (ec) { std::cerr << "rcpp site: cannot create " << out.string() << ": " << ec.message() << "\n"; return 1; }
  bool ok = write_file(out / "style.css", kStyle) && write_file(out / "site.js", kScript);

  // the lessons, in course order, so each page can point to the one before and after
  const std::vector<const Lesson*> all = catalog.all();
  std::map<std::string, const Lesson*> by_id;
  for (const Lesson* l : all) by_id[l->id] = l;
  std::map<int, const Phase*> phase_of;
  for (const Phase& p : catalog.phases) phase_of[p.num] = &p;

  int pages = 0;
  for (std::size_t k = 0; k < all.size(); ++k) {
    const Lesson& l = *all[k];
    const Phase* ph = phase_of[l.phase_num];
    std::ostringstream b;
    b << "<p class=\"dim\">Lesson " << lesson_number(l) << " &middot; about " << l.minutes << " minutes";
    if (progress.has(l.id)) b << " &middot; <span class=\"done\">passed</span>";
    b << "</p>\n";
    const auto docs = read_file(l.path / "docs" / "en.md");
    b << (docs ? markdown_to_html(*docs, sub) : "<h1>" + html_escape(l.title) + "</h1>\n<p>This lesson has no page yet.</p>\n");

    b << "<div class=\"box\"><strong>Your files.</strong> Write your code in <code>" << l.rel_path
      << "/exercise/solution.hpp</code>, then check it from the top folder of the course with<pre><code>rcpp verify "
      << lesson_number(l) << "</code></pre>The finished version is in <code>" << l.rel_path
      << "/reference/solution.hpp</code>; look at it after you have tried.";
    if (!l.requires_ids.empty()) {
      b << "<br><br><strong>Before this one:</strong> ";
      for (std::size_t i = 0; i < l.requires_ids.size(); ++i) {
        const std::string& r = l.requires_ids[i];
        if (i) b << ", ";
        if (by_id.count(r))
          b << "<a href=\"" << r << ".html\">" << html_escape(r.substr(0, 5) + " " + by_id[r]->title) << "</a>";
        else
          b << html_escape(r);
      }
    }
    b << "</div>\n";
    b << quiz_html(l.path / "quiz.json", sub);
    b << "<div class=\"pager\"><span>";
    if (k > 0) b << "<a href=\"" << all[k - 1]->id << ".html\">&larr; " << html_escape(all[k - 1]->title) << "</a>";
    b << "</span><span>";
    if (k + 1 < all.size()) b << "<a href=\"" << all[k + 1]->id << ".html\">" << html_escape(all[k + 1]->title) << " &rarr;</a>";
    b << "</span></div>\n";
    const std::string crumbs = "<a href=\"../index.html\">Home</a> &rsaquo; " +
                               (ph ? "<a href=\"../index.html#phase-" + std::to_string(ph->num) + "\">Phase " +
                                         std::to_string(ph->num) + ", " + html_escape(ph->title) + "</a>"
                                   : std::string());
    ok = write_file(out / "l" / (l.id + ".html"), page(l.title, "../", crumbs, b.str())) && ok;
    ++pages;
  }

  // the atlas entries
  for (const AtlasEntry& e : atlas.entries) {
    const auto text = read_file(e.path);
    std::string body_md = text ? *text : "";
    const std::size_t sep = body_md.find("\n---\n");
    body_md = sep == std::string::npos ? "" : body_md.substr(sep + 5);
    std::ostringstream b;
    b << "<p class=\"dim\">Failure Atlas &middot; " << e.id << "</p>\n<h1>" << html_escape(e.title) << "</h1>\n"
      << markdown_to_html(body_md, sub);
    if (by_id.count(e.teaches))
      b << "<div class=\"box\">Taught in <a href=\"../l/" << e.teaches << ".html\">"
        << html_escape(e.teaches.substr(0, 5) + " " + by_id[e.teaches]->title) << "</a>. From a terminal, <code>rcpp explain "
        << e.id << "</code> prints this page, and <code>rcpp explain</code> with an error message finds it.</div>\n";
    ok = write_file(out / "e" / (e.id + ".html"),
                    page(e.id + " " + e.title, "../", "<a href=\"../index.html\">Home</a> &rsaquo; <a href=\"../atlas.html\">Failure Atlas</a>", b.str())) && ok;
    ++pages;
  }

  // the atlas index
  {
    std::ostringstream b;
    b << "<h1>Failure Atlas</h1>\n<p>What goes wrong, why, and how to fix it: " << atlas.entries.size()
      << " entries, each linked to the lesson that teaches it. Type a word from your error or symptom to narrow the list.</p>\n"
      << "<input class=\"filter\" type=\"search\" placeholder=\"Search the atlas\" oninput=\"filterList(this)\">\n<ul class=\"lessons\">\n";
    for (const AtlasEntry& e : atlas.entries) {
      std::string text = e.id + " " + e.title + " " + e.symptom;
      for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      b << "<li data-text=\"" << html_escape(text) << "\"><span class=\"num\">" << e.id << "</span> <a href=\"e/" << e.id
        << ".html\">" << html_escape(e.title) << "</a></li>\n";
    }
    b << "</ul>\n";
    ok = write_file(out / "atlas.html", page("Failure Atlas", "", "<a href=\"index.html\">Home</a>", b.str())) && ok;
  }

  // the home page
  {
    int passed = 0;
    for (const Lesson* l : all) passed += progress.has(l->id) ? 1 : 0;
    std::ostringstream b;
    b << "<h1>Robotics C++ From Scratch</h1>\n"
      << "<p>A course in C++ for robots, from a first program to arms, vehicles and legged robots. " << all.size()
      << " lessons in " << catalog.phases.size() << " phases. Every lesson is a small piece of real code with tests that say whether it works.</p>\n"
      << "<div class=\"box\"><strong>How to use it.</strong><ol>"
      << "<li>Open the first lesson below and read it.</li>"
      << "<li>Write your part in the lesson's <code>exercise/solution.hpp</code>, in any editor.</li>"
      << "<li>Check it with <code>rcpp verify</code> and the lesson's number, from the top folder of the course. When it passes, go to the next lesson.</li>"
      << "<li>Stuck on an error? Search the <a href=\"atlas.html\">Failure Atlas</a>.</li></ol>"
      << "You have passed <strong>" << passed << "</strong> of " << all.size() << " lessons. Run <code>rcpp site</code> again to update these pages.</div>\n"
      << "<input class=\"filter\" type=\"search\" placeholder=\"Search the lessons\" oninput=\"filterList(this)\">\n";
    for (const Phase& p : catalog.phases) {
      int done = 0;
      for (const Lesson& l : p.lessons) done += progress.has(l.id) ? 1 : 0;
      b << "<details id=\"phase-" << p.num << "\"><summary>Phase " << p.num << ", " << html_escape(p.title)
        << " <span class=\"dim\">(" << p.lessons.size() << " lessons" << (done ? ", " + std::to_string(done) + " passed" : "")
        << ")</span></summary>\n";
      if (!p.ends_with.empty()) b << "<p class=\"dim\">" << html_escape(p.ends_with) << ".</p>\n";
      b << "<ul class=\"lessons\">\n";
      for (const Lesson& l : p.lessons) {
        std::string text = lesson_number(l) + " " + l.title;
        for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        b << "<li data-text=\"" << html_escape(text) << "\"><span class=\"num\">" << lesson_number(l) << "</span> <a href=\"l/"
          << l.id << ".html\">" << html_escape(l.title) << "</a>" << (progress.has(l.id) ? " <span class=\"done\">passed</span>" : "")
          << "</li>\n";
      }
      b << "</ul></details>\n";
    }
    b << "<p class=\"dim\" style=\"margin-top:28px\">These pages are made from the lesson files by <code>rcpp site</code>; the files are the course, and these pages are a way to read it.</p>\n";
    ok = write_file(out / "index.html", page("Robotics C++ From Scratch", "", "", b.str())) && ok;
    ++pages;
  }

  if (!ok) { std::cerr << "rcpp site: some pages could not be written under " << out.string() << "\n"; return 1; }
  std::cout << "wrote " << pages + 1 << " pages to " << out.string() << "\n"
            << "open " << (out / "index.html").string() << " in a web browser\n";
  return 0;
}

}  // namespace rcpp
