// The framework's main is suppressed because a Qt application object is made first.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>

#include <iostream>
#include <string>

#include "solution.hpp"

namespace {

bool bad(const QStringList& args, const char* must_name) {
  StationArgs a;
  QString error;
  const bool good = parse_station_args(args, &a, &error);
  return !good && error.contains(QString::fromLatin1(must_name));
}

}  // namespace

RC_TEST("the parser of Qt accepts a port that is abc and takes the last of a port given twice, and this one does not") {
  QCommandLineParser plain;
  QCommandLineOption port(QStringList() << "p" << "port", "the port", "number", "5000");
  plain.addOption(port);
  const bool accepted = plain.parse(QStringList() << "station" << "--port" << "abc");
  const int as_number = plain.value(port).toInt();
  QCommandLineParser twice;
  twice.addOption(port);
  twice.parse(QStringList() << "station" << "--port" << "1" << "--port" << "2");
  std::cout << "  Qt: --port abc is " << (accepted ? "accepted" : "refused") << " and is " << as_number << " as a number; --port 1 --port 2 gives " << twice.value(port).toStdString() << " (and "
            << twice.values(port).size() << " values)\n";
  RC_CHECK(accepted);
  RC_CHECK_EQ(as_number, 0);
  RC_CHECK(twice.value(port) == "2");

  StationArgs a;
  QString error;
  RC_CHECK(!parse_station_args(QStringList() << "station" << "--port" << "abc", &a, &error));
  std::cout << "  the station says: " << error.toStdString() << "\n";
  RC_CHECK(error.contains("--port"));
  RC_CHECK(error.contains("abc"));
  RC_CHECK(!parse_station_args(QStringList() << "station" << "--port" << "1" << "--port" << "2", &a, &error));
  RC_CHECK(error.contains("more than once"));
  RC_CHECK(error.contains("port"));
}

RC_TEST("a good line is read into typed values, in the long form, the short form and with an equals sign, and the rest are the defaults") {
  StationArgs a;
  QString error;
  RC_CHECK(parse_station_args(QStringList() << "station", &a, &error));
  RC_CHECK(a.host == "127.0.0.1");
  RC_CHECK_EQ(a.port, 5000);
  RC_CHECK_NEAR(a.limit_mps, 1.0, 1e-12);
  RC_CHECK(a.config_path.isEmpty());
  RC_CHECK(!a.verbose);
  RC_CHECK(!a.help && !a.version);

  StationArgs b;
  RC_CHECK(parse_station_args(QStringList() << "station" << "--host" << "10.0.0.5" << "-p" << "5001" << "--limit=2.5" << "-c" << "/etc/station.ini" << "--verbose", &b, &error));
  std::cout << "  --host 10.0.0.5 -p 5001 --limit=2.5 -c /etc/station.ini --verbose: " << b.host.toStdString() << ", " << b.port << ", " << b.limit_mps << ", " << b.config_path.toStdString() << ", "
            << b.verbose << "\n\n";
  RC_CHECK(b.host == "10.0.0.5");
  RC_CHECK_EQ(b.port, 5001);
  RC_CHECK_NEAR(b.limit_mps, 2.5, 1e-12);
  RC_CHECK(b.config_path == "/etc/station.ini");
  RC_CHECK(b.verbose);
  RC_CHECK_EQ(a.port, 5000);                      // the first result was not disturbed by the second
}

RC_TEST("each way a line can be wrong is refused, with the name of what is wrong in the message") {
  struct Case {
    const char* what;
    QStringList args;
    const char* names;
  };
  const Case cases[] = {
      {"a port that is not a number", QStringList() << "station" << "--port" << "50O1", "--port"},
      {"a port of 0", QStringList() << "station" << "--port" << "0", "--port"},
      {"a port above 65535", QStringList() << "station" << "--port" << "65536", "--port"},
      {"a limit of 99", QStringList() << "station" << "--limit" << "99", "--limit"},
      {"a negative limit", QStringList() << "station" << "--limit" << "-1", "--limit"},
      {"a limit that is not a number", QStringList() << "station" << "--limit" << "fast", "--limit"},
      {"an empty host", QStringList() << "station" << "--host" << "", "--host"},
      {"an option that does not exist", QStringList() << "station" << "--hots" << "10.0.0.5", "hots"},
      {"a value missing at the end", QStringList() << "station" << "--host", "--host"},
      {"an argument that is not an option", QStringList() << "station" << "10.0.0.5", "10.0.0.5"},
      {"a value on a flag", QStringList() << "station" << "--verbose=yes", "verbose"},
  };
  int refused = 0;
  int named = 0;
  for (const Case& c : cases) {
    StationArgs a;
    QString error;
    const bool good = parse_station_args(c.args, &a, &error);
    if (!good) ++refused;
    if (!good && error.contains(QString::fromLatin1(c.names))) ++named;
    std::cout << "  " << (good ? "ACCEPTED" : "refused ") << "  " << c.what << (good ? "" : ": ") << (good ? "" : error.toStdString()) << "\n";
    RC_CHECK(!good);
  }
  std::cout << "  " << refused << " of 11 refused, " << named << " with the name in the message\n\n";
  RC_CHECK_EQ(refused, 11);
  RC_CHECK_EQ(named, 11);
}

RC_TEST("--help and --version are good lines that set a flag, and the usage lists every option with its default") {
  StationArgs h, v;
  QString error;
  RC_CHECK(parse_station_args(QStringList() << "station" << "--help", &h, &error));
  RC_CHECK(h.help);
  RC_CHECK(!h.version);
  RC_CHECK(parse_station_args(QStringList() << "station" << "--version", &v, &error));
  RC_CHECK(v.version);
  RC_CHECK(!v.help);
  // Nothing else on the line is checked when help is asked for.
  StationArgs h2;
  RC_CHECK(parse_station_args(QStringList() << "station" << "--help" << "--port" << "abc", &h2, &error));
  RC_CHECK(h2.help);

  const QString usage = station_usage();
  const QString flat = usage.simplified();         // the help of Qt wraps long lines, and a phrase can be split across two
  for (const char* name : {"--host", "--port", "--limit", "--config", "--verbose", "--help", "--version"}) RC_CHECK(usage.contains(QString::fromLatin1(name)));
  RC_CHECK(flat.contains("default 5000"));        // the help of Qt does not print a default value, so the description says it
  RC_CHECK(flat.contains("default 127.0.0.1"));
  RC_CHECK(flat.contains("in metres a second"));
  std::cout << "  the usage is " << usage.count('\n') << " lines and names every option\n\n";
}

RC_TEST("a null for the result or for the error is allowed, and a failed parse leaves the result as it was") {
  QString error;
  RC_CHECK(parse_station_args(QStringList() << "station" << "--port" << "5002", nullptr, &error));
  RC_CHECK(!parse_station_args(QStringList() << "station" << "--port" << "x", nullptr, nullptr));

  StationArgs a;
  a.port = 1234;
  a.host = "kept";
  RC_CHECK(!parse_station_args(QStringList() << "station" << "--port" << "x" << "--host" << "other", &a, &error));
  RC_CHECK_EQ(a.port, 1234);                       // untouched by the refusal
  RC_CHECK(a.host == "kept");
  RC_CHECK(bad(QStringList() << "station" << "--limit" << "nan", "--limit"));
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  app.setApplicationName("station");
  app.setApplicationVersion("1.0");
  return rc::test::run_all();
}
