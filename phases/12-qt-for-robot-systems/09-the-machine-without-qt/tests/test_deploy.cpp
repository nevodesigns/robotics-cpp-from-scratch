// The framework's main is suppressed because Qt wants an application object before it looks for its plugins.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>

#include <iostream>

#include "solution.hpp"

namespace {

void touch(const QString& path) {
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile f(path);
  if (f.open(QIODevice::WriteOnly)) f.write("x");
}

void write(const QString& path, const char* text) {
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile f(path);
  if (f.open(QIODevice::WriteOnly)) f.write(text);
}

const QStringList kRequired = QStringList() << "station*" << "*Qt6Core*" << "platforms/*offscreen*" << "qt.conf";

// A folder that holds everything that kRequired asks for.
void make_complete(const QString& root) {
  touch(root + "/station");
  touch(root + "/libQt6Core.so.6");
  touch(root + "/platforms/libqoffscreen.so");
  write(root + "/qt.conf", "[Paths]\nPlugins=plugins\nPrefix=.\n");
}

}  // namespace

RC_TEST("a folder that holds everything that is asked for passes, and the patterns match the names of both systems") {
  QTemporaryDir dir;
  make_complete(dir.path());
  const DeployReport report = check_deployment(dir.path(), kRequired, QStringList() << "qt.conf");
  std::cout << "  a complete folder: " << report.missing.size() << " missing, " << report.stray_paths.size() << " stray paths\n";
  RC_CHECK(report.ok());
  RC_CHECK_EQ(report.missing.size(), 0);
  RC_CHECK_EQ(report.stray_paths.size(), 0);

  // The same patterns on the names a Windows build has.
  QTemporaryDir win;
  touch(win.path() + "/station.exe");
  touch(win.path() + "/Qt6Cored.dll");
  touch(win.path() + "/platforms/qoffscreen.dll");
  write(win.path() + "/qt.conf", "[Paths]\nPlugins=plugins\n");
  RC_CHECK(check_deployment(win.path(), kRequired, QStringList()).ok());
  RC_CHECK(has_match(dir.path(), "libQt6C?re.so.*"));
  RC_CHECK(!has_match(dir.path(), "*Qt6Gui*"));
}

RC_TEST("what is not in the folder is reported by the pattern that was asked for, in the order it was asked for") {
  QTemporaryDir dir;
  make_complete(dir.path());
  QFile::remove(dir.path() + "/platforms/libqoffscreen.so");
  QDir(dir.path() + "/platforms").removeRecursively();
  QFile::remove(dir.path() + "/libQt6Core.so.6");
  const DeployReport report = check_deployment(dir.path(), kRequired, QStringList());
  std::cout << "  the platform plugin and Qt6Core taken out: missing";
  for (const QString& m : report.missing) std::cout << " \"" << m.toStdString() << "\"";
  std::cout << "\n";
  RC_CHECK(!report.ok());
  RC_CHECK_EQ(report.missing.size(), 2);
  if (report.missing.size() == 2) {
    RC_CHECK(report.missing[0] == "*Qt6Core*");               // the order of the list that was given
    RC_CHECK(report.missing[1] == "platforms/*offscreen*");
  }
  RC_CHECK(!has_match(dir.path(), "platforms/*"));             // a directory that is not there matches nothing, and is not an error
}

RC_TEST("a file that names a path that is not on this machine is a stray path, and a relative one or one that exists is not") {
  QTemporaryDir dir;
  make_complete(dir.path());
  write(dir.path() + "/qt.conf", "[Paths]\nPrefix=/home/builder/Qt/6.2/gcc_64\nPlugins=plugins\nLibraries=C:\\Qt\\6.5.3\\msvc2019_64\\bin\nData=C:/Qt/6.5.3/data\n");
  touch(dir.path() + "/log.txt");
  write(dir.path() + "/station.ini", (QString("[General]\nlog=") + dir.path() + "/log.txt\n").toUtf8().constData());
  const DeployReport report = check_deployment(dir.path(), kRequired, QStringList() << "qt.conf" << "station.ini" << "no_such_file.txt");
  std::cout << "  a qt.conf from a build machine: " << report.stray_paths.size() << " stray paths:";
  for (const QString& s : report.stray_paths) std::cout << "\n    " << s.toStdString();
  std::cout << "\n\n";
  RC_CHECK_EQ(report.missing.size(), 0);
  RC_CHECK_EQ(report.stray_paths.size(), 3);                  // the Linux prefix and the two Windows paths: not the relative Plugins, not the log inside this folder
  RC_CHECK(!report.ok());
  bool prefix = false, win_backslash = false, win_slash = false;
  for (const QString& s : report.stray_paths) {
    if (s.startsWith("qt.conf: ") && s.contains("/home/builder/Qt/6.2/gcc_64")) prefix = true;
    if (s.contains("C:\\Qt\\6.5.3\\msvc2019_64\\bin")) win_backslash = true;
    if (s.contains("C:/Qt/6.5.3/data")) win_slash = true;
  }
  RC_CHECK(prefix);
  RC_CHECK(win_backslash);
  RC_CHECK(win_slash);
}

RC_TEST("the paths in a text are found as written on either system, up to a space, comma, semicolon or quote") {
  const QStringList a = path_tokens("Prefix=/opt/qt/6.2, Data=C:\\Qt\\6.5;rest \"/var/log/x y\" 'D:/tools/bin'");
  RC_CHECK_EQ(a.size(), 4);
  if (a.size() == 4) {
    RC_CHECK(a[0] == "/opt/qt/6.2");
    RC_CHECK(a[1] == "C:\\Qt\\6.5");
    RC_CHECK(a[2] == "/var/log/x");
    RC_CHECK(a[3] == "D:/tools/bin");
  }
  RC_CHECK_EQ(path_tokens("Plugins=plugins\nPrefix=.\n").size(), 0);
  RC_CHECK_EQ(path_tokens("").size(), 0);

  // And the check finds four in a file, none of them being on this machine.
  QTemporaryDir dir;
  write(dir.path() + "/t.txt", "Prefix=/opt/no_such_qt/6.2, Data=C:\\NoSuchQt\\6.5;rest \"/var/no_such_log/x\" 'D:/no_such_tools/bin'");
  const DeployReport report = check_deployment(dir.path(), QStringList(), QStringList() << "t.txt");
  RC_CHECK_EQ(report.stray_paths.size(), 4);
}

RC_TEST("a folder that does not exist has everything missing and no crash, and nothing asked for is nothing missing") {
  const DeployReport none = check_deployment("/this/folder/does/not/exist/at/all", kRequired, QStringList() << "qt.conf");
  std::cout << "  a folder that is not there: " << none.missing.size() << " of " << kRequired.size() << " missing\n";
  RC_CHECK_EQ(none.missing.size(), kRequired.size());
  RC_CHECK_EQ(none.stray_paths.size(), 0);
  QTemporaryDir empty;
  const DeployReport nothing = check_deployment(empty.path(), QStringList(), QStringList());
  RC_CHECK(nothing.ok());
  RC_CHECK(!QCoreApplication::libraryPaths().isEmpty());       // this program knows where it looks for plugins, which is the other half of the question
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
