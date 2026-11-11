// rc/qt/station_settings.hpp
//
// The settings of a control station, from lesson 12-05, graduated.
//
// A fresh read of an INI file gave every value as a string, "abc" as an integer was 0 with ok false, and a setting was not in the file until sync; a port that was not a whole number was the default with a note; a speed limit of 99 was 5 with a note; a file of the first version was brought to the second once, in metres.

#ifndef RC_QT_STATION_SETTINGS_HPP
#define RC_QT_STATION_SETTINGS_HPP

#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <cmath>
#include <rc/qt/settings_convert.hpp>

namespace rc {
namespace qt {

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
  int port() const { return strict_int("robot/port", 5000, 1, 65535); }
  // Refuses a port outside 1 to 65535 (returns false, and writes nothing).
  bool set_port(int port) {
    if (port < 1 || port > 65535) return false;
    settings_.setValue(QStringLiteral("robot/port"), port);
    return true;
  }

  // The most the robot may be driven at, in metres a second, 0 to 5. A missing value is 1.0. A value that is not a finite number is 1.0 with a note. A number outside 0 to 5 is
  // pulled to the nearest end, with a note.
  double limit_mps() const {
    const QString key = QStringLiteral("limit_mps");
    if (!settings_.contains(key)) return 1.0;
    bool ok = false;
    const double value = rc::qt::setting_double(settings_.value(key), &ok);
    if (!ok) {
      problems_.append(key + QStringLiteral(": not a number, using 1.0"));
      return 1.0;
    }
    if (value < 0.0) {
      problems_.append(key + QStringLiteral(": below 0, using 0"));
      return 0.0;
    }
    if (value > 5.0) {
      problems_.append(key + QStringLiteral(": above 5, using 5"));
      return 5.0;
    }
    return value;
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
    settings_.sync();
    return settings_.status() == QSettings::NoError;
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
    if (version() >= kVersion) return;
    if (settings_.contains(QStringLiteral("robot_ip"))) {
      settings_.setValue(QStringLiteral("robot/host"), settings_.value(QStringLiteral("robot_ip")));
      settings_.remove(QStringLiteral("robot_ip"));
    }
    if (settings_.contains(QStringLiteral("speed_limit_mm"))) {
      bool ok = false;
      const int mm = rc::qt::setting_int(settings_.value(QStringLiteral("speed_limit_mm")), &ok);
      if (ok) settings_.setValue(QStringLiteral("limit_mps"), static_cast<double>(mm) / 1000.0);
      else problems_.append(QStringLiteral("speed_limit_mm: not a whole number, dropped"));
      settings_.remove(QStringLiteral("speed_limit_mm"));
    }
    settings_.setValue(QStringLiteral("version"), kVersion);
  }

  mutable QSettings settings_;
  mutable QStringList problems_;
};
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_STATION_SETTINGS_HPP
