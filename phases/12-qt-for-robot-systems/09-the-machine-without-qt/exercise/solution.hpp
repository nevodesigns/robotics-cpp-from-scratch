#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QStringList>

// A station that runs on the machine it was built on has been run with that machine around it: its Qt is in a directory that is found by a path variable, its plugins are where the
// build put them, and a settings file points at /home/builder/Qt. Copy the program to a robot with nothing on it and it does not start, or starts without a screen, with a message from
// the loader about a library or a plugin called "platforms/xcb" that is nowhere.
//
// What was meant to be shipped can be checked before it is shipped: a list of the files that must be in the folder, by name or pattern, and a look at the text files in it for paths
// that do not exist. The check is only worth anything if it is run on a folder that is a copy of what will be shipped and not on the build tree, and better on a machine that has not
// got the build tree.

struct DeployReport {
  QStringList missing;        // the required patterns that nothing in the folder matches, in the order they were asked for
  QStringList stray_paths;    // "file: path" for each absolute path in a text file that does not exist on this machine

  bool ok() const { return missing.isEmpty() && stray_paths.isEmpty(); }
};

// The absolute paths in a text, as they would be written on Linux (/home/builder/Qt) or Windows (C:\Qt\6.5.3 or C:/Qt/6.5.3): a slash, or a drive letter and a slash, followed by
// characters up to a space, a comma, a semicolon or a quote.
inline QStringList path_tokens(const QString& text) {
  static const QRegularExpression pattern(QStringLiteral("(?:[A-Za-z]:[\\\\/]|/)[^\\s,;\"']+"));
  QStringList found;
  QRegularExpressionMatchIterator it = pattern.globalMatch(text);
  while (it.hasNext()) found.append(it.next().captured(0));
  return found;
}

// Whether something in `folder` matches `pattern`, which is a path relative to the folder whose last part may have wildcards (`platforms/*offscreen*`, `*Qt6Core*`, `qt.conf`).
inline bool has_match(const QString& folder, const QString& pattern) {
  // TODO: split the pattern at its last '/': the part before is a directory inside the folder (none if there is no '/'), the part after is a file name that may have wildcards.
  // If that directory does not exist, false. Otherwise QDir(directory).entryList(QStringList(name), QDir::Files | QDir::NoDotAndDotDot) is not empty.
  (void)folder;
  (void)pattern;
  return true;
}

// Checks a folder that is to be shipped. Every pattern of `required` must match a file in it, and no file of `text_files` (names relative to the folder; one that is not there is
// skipped) may mention an absolute path that does not exist on this machine. A folder that does not exist has every pattern missing.
inline DeployReport check_deployment(const QString& folder, const QStringList& required, const QStringList& text_files) {
  // TODO: for each pattern of `required` that has_match() does not find in the folder, add it to report.missing. For each name of text_files, open folder + "/" + name (skip it if
  // it cannot be opened), read it as text, and for each path of path_tokens(text) that QFileInfo::exists(path) says is not there, add name + ": " + path to report.stray_paths.
  (void)folder;
  (void)required;
  (void)text_files;
  (void)sizeof(QFile);
  return DeployReport();
}

#endif  // LESSON_SOLUTION_HPP
