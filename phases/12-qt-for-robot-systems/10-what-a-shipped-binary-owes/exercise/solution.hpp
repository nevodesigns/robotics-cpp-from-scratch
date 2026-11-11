#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

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
  // TODO: trim white space from both ends; make it lowercase (std::tolower on an unsigned char); if it starts with "qt6", take those three letters off, otherwise if it starts
  // with "qt" and has more than two letters, take those two off. Return it.
  return module;
}

// The licence a module is offered under for open source use, from the table below, or unknown for a module that is not in it. The table has only what is certain: a module that
// is not in it is not assumed to be anything.
inline Licence licence_of(const std::string& module) {
  // TODO: name = normalise(module). The LGPL ones are core, gui, widgets, network, qml, quick, quickcontrols2, quicklayouts, concurrent, sql, test, dbus, svg, xml and printsupport;
  // the GPL only ones are charts, datavisualization, virtualkeyboard and waylandcompositor. Anything else is unknown: do not guess.
  (void)module;
  return Licence::kLgpl;
}

// What is owed or not allowed for a program that links `modules` the way `linking` says, and whether it ships the licence text of Qt with a notice that it uses it and offers the
// source of the Qt it ships. The findings come in a fixed order: for each module in the order given (once, however it is spelt), the GPL only and the unknown ones; then, if any module is LGPL
// and the linking is static, the static finding; then a missing licence text; then a missing offer of source.
inline Assessment assess(const std::vector<std::string>& modules, Linking linking, bool ships_licence_text, bool offers_source) {
  // TODO: for each module, by its normalised name and only the first time that name is seen: a GPL only one adds Finding{"gpl-only", ...} and an unknown one Finding{"unknown-module", ...},
  // and an LGPL one sets any_lgpl. Then, if any_lgpl and linking is static, Finding{"static-lgpl", ...}. Then, if not ships_licence_text, Finding{"no-licence-text", ...}. Then, if not
  // offers_source, Finding{"no-source-offer", ...}. The text of each says what is wrong in a sentence.
  (void)modules;
  (void)linking;
  (void)ships_licence_text;
  (void)offers_source;
  return Assessment();
}

#endif  // LESSON_SOLUTION_HPP
