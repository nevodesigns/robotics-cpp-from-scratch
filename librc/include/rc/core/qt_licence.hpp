// rc/core/qt_licence.hpp
//
// What a shipped Qt binary owes, from lesson 12-10, graduated. Not legal advice.
//
// A program that links Core, Gui, Widgets and Network dynamically and ships the licence text and an offer of source had no findings; a GPL only module, a static link of the LGPL modules, a missing licence text, a missing offer of source and a module that is not in the table were each a finding, in a fixed order, once for each module however it was spelt.

#ifndef RC_CORE_QT_LICENCE_HPP
#define RC_CORE_QT_LICENCE_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

namespace rc {
namespace core {

// Qt is free to use, and what a program owes for using it depends on which parts it uses and how. This lesson is not legal advice and its table is not the licence: it is a place to
// start from, for the open source licences as Qt describes them for the 6.2 series, and a station whose maker has a lawyer, or a commercial licence from Qt, should ask the lawyer, or
// read the licence of the Qt that is shipped.
//
// The shape of it is this. Most of the parts a control station uses (Core, Gui, Widgets, Network, Qml, Quick) are offered under the LGPL 3.0, which lets a program that is not free use
// them if it links to them dynamically, so that the person who has the program can put in another build of Qt, and if it hands over the licence text, says that it uses Qt, and makes the
// source of the Qt it ships available. A few parts (among them Charts, Data Visualization, Virtual Keyboard and the Wayland compositor) are offered under the GPL 3.0 only, or under
// a commercial licence, and a program that links one of those and is given to anyone else has to be given under the GPL 3.0 as a whole, with its source, or has to be built with the
// commercial licence. The mistake is to find that out after shipping, from a letter.

enum class Licence { kLgpl, kGplOnly, kUnknown };
enum class Linking { kDynamic, kStatic };

struct Finding {
  std::string code;     // "gpl-only", "unknown-module", "static-lgpl", "no-licence-text" or "no-source-offer"
  std::string text;
};

struct Assessment {
  std::vector<Finding> findings;
  bool ok() const { return findings.empty(); }
};

// A module by the name it goes by in the lowercase: Qt6Core, QtCore, Core and " core " are all "core". The prefix is Qt6 or Qt, taken once, and the rest is the name.
inline std::string normalise(const std::string& module) {
  std::size_t begin = 0;
  std::size_t end = module.size();
  while (begin < end && std::isspace(static_cast<unsigned char>(module[begin]))) ++begin;
  while (end > begin && std::isspace(static_cast<unsigned char>(module[end - 1]))) --end;
  std::string name = module.substr(begin, end - begin);
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (name.compare(0, 3, "qt6") == 0) name = name.substr(3);
  else if (name.compare(0, 2, "qt") == 0 && name.size() > 2) name = name.substr(2);
  return name;
}

// The licence a module is offered under for open source use, from the table below, or unknown for a module that is not in it. The table has only what is certain: a module that
// is not in it is not assumed to be anything.
inline Licence licence_of(const std::string& module) {
  static const char* const lgpl[] = {"core", "gui", "widgets", "network", "qml", "quick", "quickcontrols2", "quicklayouts", "concurrent", "sql", "test", "dbus", "svg", "xml", "printsupport"};
  static const char* const gpl_only[] = {"charts", "datavisualization", "virtualkeyboard", "waylandcompositor"};
  const std::string name = normalise(module);
  for (const char* m : lgpl)
    if (name == m) return Licence::kLgpl;
  for (const char* m : gpl_only)
    if (name == m) return Licence::kGplOnly;
  return Licence::kUnknown;
}

// What is owed or not allowed for a program that links `modules` the way `linking` says, and whether it ships the licence text of Qt with a notice that it uses it and offers the
// source of the Qt it ships. The findings come in a fixed order: for each module in the order given (once, however it is spelt), the GPL only and the unknown ones; then, if any module is LGPL
// and the linking is static, the static finding; then a missing licence text; then a missing offer of source.
inline Assessment assess(const std::vector<std::string>& modules, Linking linking, bool ships_licence_text, bool offers_source) {
  Assessment a;
  std::vector<std::string> seen;
  bool any_lgpl = false;
  for (const std::string& m : modules) {
    const std::string name = normalise(m);
    if (std::find(seen.begin(), seen.end(), name) != seen.end()) continue;
    seen.push_back(name);
    const Licence l = licence_of(name);
    if (l == Licence::kLgpl) any_lgpl = true;
    if (l == Licence::kGplOnly) a.findings.push_back(Finding{"gpl-only", name + " is offered under the GPL 3.0 only or a commercial licence: a program that links it and is given to others is the GPL 3.0 as a whole, with its source, or needs the commercial licence"});
    if (l == Licence::kUnknown) a.findings.push_back(Finding{"unknown-module", name + " is not in the table: find out what licence it is offered under before shipping it"});
  }
  if (any_lgpl && linking == Linking::kStatic) a.findings.push_back(Finding{"static-lgpl", "the LGPL modules are linked statically: the person who has the program must be able to replace Qt, which needs the object files of the program, so link dynamically"});
  if (!ships_licence_text) a.findings.push_back(Finding{"no-licence-text", "the licence text of Qt is not shipped, with a notice that the program uses Qt"});
  if (!offers_source) a.findings.push_back(Finding{"no-source-offer", "the source of the Qt that is shipped is not offered"});
  return a;
}
}  // namespace core
}  // namespace rc

#endif  // RC_CORE_QT_LICENCE_HPP
