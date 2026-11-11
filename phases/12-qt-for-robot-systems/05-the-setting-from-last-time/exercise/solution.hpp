#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <cmath>
#include <rc/qt/settings_convert.hpp>

// A station remembers where the robot is and how fast it may be driven, so that the operator does not type them in again. The file is written by the program, and it is read by
// the program, and between the two it is read by people, copied between machines, edited with a text editor, and left behind by a version of the station that is two releases old.
//
// Three things go wrong with that. A value that cannot be read is not an error to QSettings: asked for an integer, "abc" is 0, and a speed limit of 0 is a robot that does not move.
// A setting is written to the file when it is synced and not when it is set, and a station that is killed has not synced. And a file written by an older version has other names
// for things, in other units, and the new version reads none of them and uses its defaults without a word.

class StationSettings {
 public:
  static constexpr int kVersion = 2;

  // Opens the file at `path` (an INI file), and brings what it holds up to the current version.
  explicit StationSettings(const QString& path) : settings_(path, QSettings::IniFormat) { migrate(); }

  int version() const { return strict_int("version", 1, 1, 1000000); }

  QString host() const { return settings_.value(QStringLiteral("robot/host"), QStringLiteral("127.0.0.1")).toString(); }
  void set_host(const QString& host) { settings_.setValue(QStringLiteral("robot/host"), host); }

  // The port of the robot, 1 to 65535. A missing value is the default, quietly. A value that is there and is not a whole number in that range is the default too, with a
  // note of what was wrong in problems().
  int port() const {
    // TODO: strict_int("robot/port", 5000, 1, 65535). strict_int is supplied: it gives the default for a missing key, and the default with a note in problems_ for a value that is
    // there and is not a whole number in the range.
    return -1;
  }
  // Refuses a port outside 1 to 65535 (returns false, and writes nothing).
  bool set_port(int port) {
    if (port < 1 || port > 65535) return false;
    settings_.setValue(QStringLiteral("robot/port"), port);
    return true;
  }

  // The most the robot may be driven at, in metres a second, 0 to 5. A missing value is 1.0. A value that is not a finite number is 1.0 with a note. A number outside 0 to 5 is
  // pulled to the nearest end, with a note.
  double limit_mps() const {
    // TODO: key = "limit_mps". If settings_ does not contain it, 1.0. Otherwise value = rc::qt::setting_double(settings_.value(key), &ok): if !ok, add a note
    // (key + ": not a number, using 1.0") to problems_ and return 1.0. If value < 0, a note and 0.0. If value > 5, a note and 5.0. Otherwise value.
    return -1.0;
  }
  void set_limit_mps(double v) { settings_.setValue(QStringLiteral("limit_mps"), v); }

  // Whether the display is turned over. A word that is not true or false is the default (false) with a note.
  bool invert_y() const {
    const QString key = QStringLiteral("display/invert_y");
    if (!settings_.contains(key)) return false;
    bool ok = false;
    const bool value = rc::qt::setting_bool(settings_.value(key), &ok);
    if (!ok) {
      problems_.append(key + QStringLiteral(": not true or false, using false"));
      return false;
    }
    return value;
  }
  void set_invert_y(bool v) { settings_.setValue(QStringLiteral("display/invert_y"), v); }

  // Writes the settings to the file now, and says whether that worked. Until this is called a setting is in memory only.
  bool save() {
    // TODO: settings_.sync(), and then true if settings_.status() == QSettings::NoError.
    return false;
  }

  // What was wrong with the values read so far, one line each.
  QStringList problems() const { return problems_; }

 private:
  // A whole number from the file between `low` and `high`, or the default (with a note if the value is there and wrong).
  int strict_int(const QString& key, int fallback, int low, int high) const {
    if (!settings_.contains(key)) return fallback;
    bool ok = false;
    const int value = rc::qt::setting_int(settings_.value(key), &ok);
    if (!ok) {
      problems_.append(key + QStringLiteral(": not a whole number, using ") + QString::number(fallback));
      return fallback;
    }
    if (value < low || value > high) {
      problems_.append(key + QStringLiteral(": outside ") + QString::number(low) + QStringLiteral(" to ") + QString::number(high) + QStringLiteral(", using ") + QString::number(fallback));
      return fallback;
    }
    return value;
  }

  // Version 1 kept the address in `robot_ip` and the speed limit in whole millimetres a second in `speed_limit_mm`. Version 2 keeps `robot/host` and `limit_mps`, in metres.
  // Brings a file of the first version up to the second, once: the old names are removed, and a file that is already the second is not touched.
  void migrate() {
    // TODO: return if version() is already kVersion or more. If settings_ contains "robot_ip": set "robot/host" to its value and remove "robot_ip". If it contains
    // "speed_limit_mm": mm = rc::qt::setting_int(value, &ok); if ok, set "limit_mps" to mm / 1000.0 (as a double); if not, add a note to problems_; remove
    // "speed_limit_mm" either way. Last, set "version" to kVersion.
  }

  mutable QSettings settings_;
  mutable QStringList problems_;
};

#endif  // LESSON_SOLUTION_HPP
