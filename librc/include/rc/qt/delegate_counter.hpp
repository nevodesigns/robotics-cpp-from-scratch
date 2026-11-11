// rc/qt/delegate_counter.hpp
//
// Counting delegates, from lesson 11-11, graduated.
//
// A Repeater over 10000 rows made 10000 delegates and a ListView of the same rows and a height of 300 made 21; jumping to row 5000 made 11 more and left 12; the number does not grow with the rows; and a model that computes its rows held nothing.

#ifndef RC_QT_DELEGATE_COUNTER_HPP
#define RC_QT_DELEGATE_COUNTER_HPP

#include <QtCore/QAbstractListModel>
#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QModelIndex>
#include <QtCore/QObject>
#include <QtCore/QVariant>

namespace rc {
namespace qt {

// A delegate is the QML item that shows one row. A Repeater makes one for every row of its model: ten thousand rows is ten thousand items, made when the model is set, whether
// anyone sees them or not. A ListView makes them for the rows it shows and a few beyond, and makes and destroys them as the view moves. Which of the two an interface
// uses is a decision about memory and about the time it takes to appear, and it is a decision that no error ever reports.
//
// A test can hold it, if the delegates report themselves: a counter they tell when they are made and when they go, with how many exist and the most there have been.

class DelegateCounter : public QObject {
  Q_OBJECT

 public:
  explicit DelegateCounter(QObject* parent = nullptr) : QObject(parent) {}

  // A delegate was made.
  Q_INVOKABLE void created() {
    ++total_;
    ++alive_;
    if (alive_ > peak_) peak_ = alive_;
  }
  // A delegate went. A report with none alive is not a delegate that went, and is counted as unmatched.
  Q_INVOKABLE void gone() {
    if (alive_ == 0) {
      ++unmatched_;
      return;
    }
    --alive_;
  }

  int total() const { return total_; }          // delegates ever made
  int alive() const { return alive_; }          // delegates that exist now
  int peak() const { return peak_; }            // the most that existed at once
  int unmatched() const { return unmatched_; }  // reports of one going when none existed

 private:
  int total_ = 0;
  int alive_ = 0;
  int peak_ = 0;
  int unmatched_ = 0;
};

// A model of `count` rows that holds nothing: the value of a row is worked out when it is asked for, as twice its number. It can be as long as an int can count.
class RangeModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role { ValueRole = Qt::UserRole + 1 };

  explicit RangeModel(int count, QObject* parent = nullptr) : QAbstractListModel(parent), count_(count < 0 ? 0 : count) {}

  int rowCount(const QModelIndex& parent = QModelIndex()) const override { return parent.isValid() ? 0 : count_; }

  QVariant data(const QModelIndex& index, int role) const override {
    if (!index.isValid() || index.parent().isValid() || index.row() < 0 || index.row() >= count_ || role != ValueRole) return QVariant();
    return index.row() * 2;
  }

  QHash<int, QByteArray> roleNames() const override {
    QHash<int, QByteArray> names;
    names[ValueRole] = "value";
    return names;
  }

  // The number of rows the model was made with.
  int count() const { return count_; }

 private:
  int count_;
};
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_DELEGATE_COUNTER_HPP
