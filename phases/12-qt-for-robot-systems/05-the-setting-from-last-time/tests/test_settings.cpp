// The framework's main is suppressed because QSettings is happier with a QCoreApplication to exist.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>

#include <iostream>

#include "solution.hpp"

namespace {

// A settings file written by hand, as a person or an older version would have, at a path that no QSettings has been given before.
QString write_ini(QTemporaryDir& dir, const char* name, const char* text) {
  const QString path = dir.filePath(QString::fromLatin1(name));
  QFile file(path);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(text);
    file.close();
  }
  return path;
}

QString file_text(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return QString();
  return QString::fromUtf8(file.readAll());
}

}  // namespace

RC_TEST("QSettings reads a file as text, and takes abc for an integer as 0 with nothing said") {
  QTemporaryDir dir;
  const QString path = write_ini(dir, "fresh.ini", "[General]\nflag=no\nlimit_mps=1.5\n[robot]\nport=abc\n");
  QSettings raw(path, QSettings::IniFormat);
  bool ok = true;
  const int naive_port = raw.value("robot/port").toInt(&ok);
  const bool naive_flag = raw.value("flag").toBool();
  std::cout << "  a file read fresh: the port abc is " << naive_port << " (ok " << ok << "), the flag no is " << naive_flag << ", the type of a stored number is "
            << raw.value("limit_mps").typeName() << "\n";
  RC_CHECK_EQ(naive_port, 0);                    // and a plain toInt() does not say it was wrong
  RC_CHECK(!ok);
  RC_CHECK(naive_flag);                          // "no" is true, as lesson 09-10 found
  RC_CHECK(raw.value("limit_mps").typeId() == QMetaType::QString);

  StationSettings s(path);
  RC_CHECK_EQ(s.port(), 5000);                   // the default, and not 0
  RC_CHECK(s.invert_y() == false);
  RC_CHECK_NEAR(s.limit_mps(), 1.5, 1e-12);
  RC_CHECK_EQ(s.problems().size(), 1);
  if (s.problems().size() == 1) RC_CHECK(s.problems()[0].contains("robot/port"));
  std::cout << "  the station says: " << (s.problems().isEmpty() ? "" : s.problems()[0].toStdString()) << "\n\n";
}

RC_TEST("a value that is missing is the default and not a problem, and one that is out of range is the default or the nearest end, with a note") {
  QTemporaryDir dir;
  StationSettings empty(write_ini(dir, "empty.ini", ""));
  RC_CHECK_EQ(empty.port(), 5000);
  RC_CHECK_NEAR(empty.limit_mps(), 1.0, 1e-12);
  RC_CHECK(empty.host() == "127.0.0.1");
  RC_CHECK(!empty.invert_y());
  RC_CHECK_EQ(empty.problems().size(), 0);       // nothing was wrong: nothing was written yet

  StationSettings odd(write_ini(dir, "odd.ini", "[General]\nversion=2\nlimit_mps=99\n[robot]\nport=70000\n[display]\ninvert_y=maybe\n"));
  const double limit = odd.limit_mps();
  const int port = odd.port();
  const bool flip = odd.invert_y();
  std::cout << "  limit 99: " << limit << ", port 70000: " << port << ", invert_y maybe: " << flip << "; " << odd.problems().size() << " notes\n\n";
  RC_CHECK_NEAR(limit, 5.0, 1e-12);              // pulled to the end, not passed on
  RC_CHECK_EQ(port, 5000);
  RC_CHECK(!flip);
  RC_CHECK_EQ(odd.problems().size(), 3);

  StationSettings negative(write_ini(dir, "negative.ini", "[General]\nversion=2\nlimit_mps=-3\n"));
  RC_CHECK_NEAR(negative.limit_mps(), 0.0, 1e-12);
  StationSettings nan_limit(write_ini(dir, "nan.ini", "[General]\nversion=2\nlimit_mps=nan\n"));
  RC_CHECK_NEAR(nan_limit.limit_mps(), 1.0, 1e-12);
  RC_CHECK_EQ(nan_limit.problems().size(), 1);
}

RC_TEST("a setting is in the file when it is saved and not when it is set, and a port that is not a port is refused") {
  QTemporaryDir dir;
  const QString path = dir.filePath("kept.ini");
  StationSettings s(path);
  s.set_host("10.0.0.5");
  RC_CHECK(s.set_port(5001));
  s.set_limit_mps(2.5);
  s.set_invert_y(true);
  const bool exists_before = QFile::exists(path);
  RC_CHECK(!s.set_port(0));                      // refused
  RC_CHECK(!s.set_port(65536));
  RC_CHECK(!s.set_port(-1));
  RC_CHECK_EQ(s.port(), 5001);                   // and the good one is still there
  RC_CHECK(s.save());
  const QString text = file_text(path);
  std::cout << "  before save the file " << (exists_before ? "existed" : "did not exist") << "; after it holds " << text.count('\n') << " lines, including: "
            << (text.contains("host=10.0.0.5") ? "host=10.0.0.5 " : "") << (text.contains("port=5001") ? "port=5001 " : "") << (text.contains("limit_mps=2.5") ? "limit_mps=2.5" : "") << "\n\n";
  RC_CHECK(!exists_before);
  RC_CHECK(text.contains("host=10.0.0.5"));
  RC_CHECK(text.contains("port=5001"));
  RC_CHECK(text.contains("limit_mps=2.5"));
  RC_CHECK(text.contains("version=2"));

  // And a station that starts again finds it.
  QSettings reread(path, QSettings::IniFormat);
  RC_CHECK(reread.value("robot/host").toString() == "10.0.0.5");
}

RC_TEST("a file of the first version is brought to the second once, in the units of the second, and its old names are gone") {
  QTemporaryDir dir;
  const QString path = write_ini(dir, "v1.ini", "[General]\nrobot_ip=192.168.1.20\nspeed_limit_mm=1500\n");
  {
    StationSettings s(path);
    std::cout << "  a version 1 file: host " << s.host().toStdString() << ", limit " << s.limit_mps() << " m/s, version " << s.version() << "\n";
    RC_CHECK(s.host() == "192.168.1.20");
    RC_CHECK_NEAR(s.limit_mps(), 1.5, 1e-12);        // 1500 mm/s is 1.5 m/s
    RC_CHECK_EQ(s.version(), 2);
    RC_CHECK_EQ(s.problems().size(), 0);
    RC_CHECK(s.save());
  }
  const QString after = file_text(path);
  std::cout << "  after saving: the old names " << ((after.contains("robot_ip") || after.contains("speed_limit_mm")) ? "are still in the file" : "are gone") << "\n";
  RC_CHECK(!after.contains("robot_ip"));
  RC_CHECK(!after.contains("speed_limit_mm"));
  RC_CHECK(after.contains("host=192.168.1.20"));
  RC_CHECK(after.contains("limit_mps=1.5"));
  RC_CHECK(after.contains("version=2"));

  // Starting again changes nothing: it is not migrated twice, and 1.5 does not become 0.0015.
  StationSettings again(path);
  RC_CHECK_NEAR(again.limit_mps(), 1.5, 1e-12);
  RC_CHECK_EQ(again.version(), 2);
  RC_CHECK(again.save());
  RC_CHECK_EQ(file_text(path).count("limit_mps"), 1);

  // A first version file with a speed that is not a number drops it, and says so.
  StationSettings bad(write_ini(dir, "v1bad.ini", "[General]\nspeed_limit_mm=fast\n"));
  RC_CHECK_NEAR(bad.limit_mps(), 1.0, 1e-12);
  RC_CHECK_EQ(bad.problems().size(), 1);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
