#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <rc/qt/settings_convert.hpp>

// A control station is started from a shortcut, a script or a service file, and what is on its command line is typed once and read for months. `--port 5001` is the easy case. The
// others are `--port 50O1` with a letter O, `--limit 99`, `--hots 10.0.0.5` and `--host` at the end of the line with its value missing.
//
// QCommandLineParser reports the shape of the line: an option it does not know, a value that is missing. It does not report a value that is the wrong kind, because to it every value is
// text, and `--port abc` is accepted and is 0 when asked for as a number. And its usual way of finishing, `process()`, prints the error and ends the program: from a test that is the
// end of the test run. The function to use is `parse()`, which returns, and the checking of the values is the program's.

using rc::qt::setting_double;
using rc::qt::setting_int;

struct StationArgs {
  QString host = QStringLiteral("127.0.0.1");
  int port = 5000;
  double limit_mps = 1.0;
  QString config_path;            // empty if none was given
  bool verbose = false;
  bool help = false;              // --help was given: show usage() and stop
  bool version = false;           // --version was given
};

namespace station_args_detail {
inline QCommandLineOption host_option() { return QCommandLineOption(QStringList() << QStringLiteral("H") << QStringLiteral("host"), QStringLiteral("The address of the robot (default 127.0.0.1)."), QStringLiteral("address"), QStringLiteral("127.0.0.1")); }
inline QCommandLineOption port_option() { return QCommandLineOption(QStringList() << QStringLiteral("p") << QStringLiteral("port"), QStringLiteral("The port of the robot, 1 to 65535 (default 5000)."), QStringLiteral("number"), QStringLiteral("5000")); }
inline QCommandLineOption limit_option() { return QCommandLineOption(QStringList() << QStringLiteral("l") << QStringLiteral("limit"), QStringLiteral("The most the robot may be driven at, in metres a second, 0 to 5 (default 1)."), QStringLiteral("metres"), QStringLiteral("1.0")); }
inline QCommandLineOption config_option() { return QCommandLineOption(QStringList() << QStringLiteral("c") << QStringLiteral("config"), QStringLiteral("The settings file."), QStringLiteral("file")); }
inline QCommandLineOption verbose_option() { return QCommandLineOption(QStringLiteral("verbose"), QStringLiteral("Say more.")); }
inline void add_all(QCommandLineParser& parser) {
  parser.setApplicationDescription(QStringLiteral("A control station for a robot."));
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addOption(host_option());
  parser.addOption(port_option());
  parser.addOption(limit_option());
  parser.addOption(config_option());
  parser.addOption(verbose_option());
}
}  // namespace station_args_detail

// The text of --help, with every option and its default.
inline QString station_usage() {
  QCommandLineParser parser;
  station_args_detail::add_all(parser);
  return parser.helpText();
}

// Reads a command line, whose first item is the name of the program. Returns true and fills `*out` if it is good; otherwise returns false and puts a message that names
// what is wrong in `*error`. Never ends the program. --help and --version are good lines: they set the flag and nothing else is checked. A line is not good if it has an
// option that is not known, one with its value missing, one given twice, an argument that is not an option, a port that is not a whole number from 1 to 65535, a limit that
// is not a number from 0 to 5, or an empty host.
inline bool parse_station_args(const QStringList& args, StationArgs* out, QString* error) {
  using namespace station_args_detail;
  QCommandLineParser parser;
  add_all(parser);
  const QCommandLineOption host = host_option(), port = port_option(), limit = limit_option(), config = config_option();
  const QString none;
  QString message;
  QString* err = error != nullptr ? error : &message;

  if (!parser.parse(args)) {
    *err = parser.errorText();
    return false;
  }
  StationArgs result;
  if (parser.isSet(QStringLiteral("help")) || parser.isSet(QStringLiteral("version"))) {
    result.help = parser.isSet(QStringLiteral("help"));
    result.version = parser.isSet(QStringLiteral("version"));
    if (out != nullptr) *out = result;
    return true;
  }
  if (!parser.positionalArguments().isEmpty()) {
    *err = QStringLiteral("unexpected argument ") + parser.positionalArguments().first();
    return false;
  }
  for (const QCommandLineOption* o : {&host, &port, &limit, &config}) {
    if (parser.values(*o).size() > 1 && parser.isSet(*o)) {
      *err = QStringLiteral("--") + o->names().last() + QStringLiteral(" was given more than once");
      return false;
    }
  }

  result.host = parser.value(host);
  if (result.host.trimmed().isEmpty()) {
    *err = QStringLiteral("--host is empty");
    return false;
  }
  bool ok = false;
  result.port = setting_int(QVariant(parser.value(port)), &ok);
  if (!ok || result.port < 1 || result.port > 65535) {
    *err = QStringLiteral("--port must be a whole number from 1 to 65535, not ") + parser.value(port);
    return false;
  }
  result.limit_mps = setting_double(QVariant(parser.value(limit)), &ok);
  if (!ok || result.limit_mps < 0.0 || result.limit_mps > 5.0) {
    *err = QStringLiteral("--limit must be a number from 0 to 5, not ") + parser.value(limit);
    return false;
  }
  result.config_path = parser.value(config);
  result.verbose = parser.isSet(QStringLiteral("verbose"));
  if (out != nullptr) *out = result;
  return true;
}

#endif  // LESSON_SOLUTION_HPP
