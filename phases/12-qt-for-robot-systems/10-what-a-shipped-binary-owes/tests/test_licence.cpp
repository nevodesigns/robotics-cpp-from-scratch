#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

using Names = std::vector<std::string>;

bool has_code(const Assessment& a, const char* code) {
  for (const Finding& f : a.findings)
    if (f.code == code) return true;
  return false;
}

}  // namespace

RC_TEST("a module is known by its name in the lowercase, however it is spelt") {
  RC_CHECK(normalise("Qt6Core") == "core");
  RC_CHECK(normalise("QtCore") == "core");
  RC_CHECK(normalise("Core") == "core");
  RC_CHECK(normalise("  core ") == "core");
  RC_CHECK(normalise("Qt6Widgets") == "widgets");
  RC_CHECK(normalise("Qt6QuickControls2") == "quickcontrols2");
  RC_CHECK(normalise("QT6GUI") == "gui");
  RC_CHECK(normalise("Qt") == "qt");                 // a name that is only the prefix is left alone
  RC_CHECK(normalise("") == "");
  RC_CHECK(normalise("Qt6Qt6Core") == "qt6core");    // the prefix is taken once
}

RC_TEST("the modules of a control station are LGPL, four add-ons are GPL only, and a module that is not in the table is unknown and not guessed") {
  for (const char* m : {"Core", "Qt6Gui", "QtWidgets", "network", "Qml", "Quick", "QuickControls2", "Concurrent", "Sql", "Test", "Svg"}) RC_CHECK(licence_of(m) == Licence::kLgpl);
  for (const char* m : {"Charts", "Qt6DataVisualization", "VirtualKeyboard", "QtWaylandCompositor"}) RC_CHECK(licence_of(m) == Licence::kGplOnly);
  for (const char* m : {"Frobnicate", "Quick3D", "Multimedia", ""}) RC_CHECK(licence_of(m) == Licence::kUnknown);
  std::cout << "  Core is " << (licence_of("Core") == Licence::kLgpl ? "LGPL" : "?") << ", Charts is " << (licence_of("Charts") == Licence::kGplOnly ? "GPL only" : "?") << ", Multimedia is "
            << (licence_of("Multimedia") == Licence::kUnknown ? "not in the table" : "?") << "\n\n";
}

RC_TEST("a program that links the usual modules dynamically and ships the licence text and an offer of source owes nothing more") {
  const Assessment a = assess(Names{"Core", "Gui", "Widgets", "Network"}, Linking::kDynamic, true, true);
  std::cout << "  Core, Gui, Widgets and Network, dynamic, with the licence text and an offer of source: " << a.findings.size() << " findings\n";
  RC_CHECK(a.ok());
  RC_CHECK_EQ(a.findings.size(), 0u);
  RC_CHECK(assess(Names(), Linking::kDynamic, true, true).ok());     // nothing linked, nothing owed
  RC_CHECK(!assess(Names{"Core", "Gui", "Widgets", "Network"}, Linking::kStatic, true, true).ok());     // the same program linked statically is not
}

RC_TEST("each way of getting it wrong is a finding of its own, with the module or the missing thing named") {
  const Assessment gpl = assess(Names{"Core", "Charts"}, Linking::kDynamic, true, true);
  RC_CHECK_EQ(gpl.findings.size(), 1u);
  RC_CHECK(has_code(gpl, "gpl-only"));
  if (!gpl.findings.empty()) RC_CHECK(gpl.findings[0].text.find("charts") != std::string::npos);

  const Assessment statically = assess(Names{"Core", "Gui"}, Linking::kStatic, true, true);
  RC_CHECK_EQ(statically.findings.size(), 1u);
  RC_CHECK(has_code(statically, "static-lgpl"));

  const Assessment no_text = assess(Names{"Core"}, Linking::kDynamic, false, true);
  RC_CHECK_EQ(no_text.findings.size(), 1u);
  RC_CHECK(has_code(no_text, "no-licence-text"));

  const Assessment no_source = assess(Names{"Core"}, Linking::kDynamic, true, false);
  RC_CHECK_EQ(no_source.findings.size(), 1u);
  RC_CHECK(has_code(no_source, "no-source-offer"));

  const Assessment unknown = assess(Names{"Core", "Multimedia"}, Linking::kDynamic, true, true);
  RC_CHECK_EQ(unknown.findings.size(), 1u);
  RC_CHECK(has_code(unknown, "unknown-module"));
  if (!unknown.findings.empty()) RC_CHECK(unknown.findings[0].text.find("multimedia") != std::string::npos);

  // Static linking of only a GPL only module is not also an LGPL finding: there is nothing LGPL to link statically.
  const Assessment only_gpl = assess(Names{"Charts"}, Linking::kStatic, true, true);
  RC_CHECK_EQ(only_gpl.findings.size(), 1u);
  RC_CHECK(has_code(only_gpl, "gpl-only"));
}

RC_TEST("everything wrong at once is every finding, in a fixed order, and a module spelt two ways is one finding") {
  const Assessment a = assess(Names{"Core", "Qt6Charts", "Multimedia", "Charts", "QtCharts", "VirtualKeyboard"}, Linking::kStatic, false, false);
  std::cout << "  every fault at once: " << a.findings.size() << " findings, in order:";
  for (const Finding& f : a.findings) std::cout << " " << f.code;
  std::cout << "\n\n";
  RC_CHECK_EQ(a.findings.size(), 6u);                // charts once, multimedia, virtualkeyboard, static, no text, no source (the modules in the order they were given)
  if (a.findings.size() == 6) {
    RC_CHECK(a.findings[0].code == "gpl-only");
    RC_CHECK(a.findings[0].text.find("charts") != std::string::npos);
    RC_CHECK(a.findings[1].code == "unknown-module");
    RC_CHECK(a.findings[2].code == "gpl-only");
    RC_CHECK(a.findings[2].text.find("virtualkeyboard") != std::string::npos);
    RC_CHECK(a.findings[3].code == "static-lgpl");
    RC_CHECK(a.findings[4].code == "no-licence-text");
    RC_CHECK(a.findings[5].code == "no-source-offer");
  }
}
