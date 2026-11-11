// rc/qt/deploy_check.hpp
//
// A check of a folder before it is shipped, from lesson 12-09, graduated.
//
// A complete folder passed; with the platform plugin removed the pattern for it was reported missing, and with two things gone both were, in the order asked; a qt.conf with the prefix of the build machine, and one with a Windows path, were each a stray path, and a relative path or one that exists was not; a folder that did not exist had everything missing.

#ifndef RC_QT_DEPLOY_CHECK_HPP
#define RC_QT_DEPLOY_CHECK_HPP

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace rc {
namespace qt {

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
  const int slash = pattern.lastIndexOf(QLatin1Char('/'));
  const QString directory = slash >= 0 ? folder + QLatin1Char('/') + pattern.left(slash) : folder;
  const QString name = slash >= 0 ? pattern.mid(slash + 1) : pattern;
  const QDir dir(directory);
  if (!dir.exists()) return false;
  return !dir.entryList(QStringList(name), QDir::Files | QDir::NoDotAndDotDot).isEmpty();
}

// Checks a folder that is to be shipped. Every pattern of `required` must match a file in it, and no file of `text_files` (names relative to the folder; one that is not there is
// skipped) may mention an absolute path that does not exist on this machine. A folder that does not exist has every pattern missing.
inline DeployReport check_deployment(const QString& folder, const QStringList& required, const QStringList& text_files) {
  DeployReport report;
  for (const QString& pattern : required)
    if (!has_match(folder, pattern)) report.missing.append(pattern);
  for (const QString& name : text_files) {
    QFile file(folder + QLatin1Char('/') + name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
    const QString text = QString::fromUtf8(file.readAll());
    for (const QString& path : path_tokens(text))
      if (!QFileInfo::exists(path)) report.stray_paths.append(name + QStringLiteral(": ") + path);
  }
  return report;
}
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_DEPLOY_CHECK_HPP
