#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QAbstractListModel>
#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QModelIndex>
#include <QtCore/QVariant>
#include <vector>

// A list in QML is shown by a view, and a view is told what changed by a signal. A property that is a list gives QML a copy of it. A JavaScript push onto the copy has
// different results for different list types, and none of them tells a view. A model is the way to say what happened: rows were inserted here, the whole list was reset.
//
// A model that changes its rows without the begin and end calls around the change is the worst case, because it looks right: rowCount() is correct, data() is correct, and
// every view that is already showing the model is still showing the old rows.

class SampleModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role { ValueRole = Qt::UserRole + 1, PositionRole };

  explicit SampleModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}

  int rowCount(const QModelIndex& parent = QModelIndex()) const override {
    return parent.isValid() ? 0 : static_cast<int>(values_.size());
  }

  // The value of a row, or its position, by role. Anything that is not a row of this model, or a role it does not know, is an empty QVariant.
  QVariant data(const QModelIndex& index, int role) const override {
    // TODO: an empty QVariant() if the index is not valid, has a parent, or its row is outside 0 to the number of values. ValueRole is the value of that
    // row, PositionRole is the row itself. Any other role is an empty QVariant.
    (void)index;
    (void)role;
    return QVariant(7777);
  }

  // The names a QML delegate uses for the roles: `value` and `position`.
  QHash<int, QByteArray> roleNames() const override {
    // TODO: a QHash with ValueRole named "value" and PositionRole named "position" (as QByteArray, written "value" and "position").
    return QHash<int, QByteArray>();
  }

  // Adds a row at the end, and tells every view about exactly that row.
  Q_INVOKABLE void append(double value) {
    // TODO: row = the current number of values. beginInsertRows(QModelIndex(), row, row); values_.push_back(value); endInsertRows(). The begin and end
    // calls are what tell the views, and they have to be around the change.
    (void)value;
  }

  // Removes every row, and tells every view that the whole list is new.
  Q_INVOKABLE void clear() {
    // TODO: beginResetModel(); values_.clear(); endResetModel().
  }

 private:
  std::vector<double> values_;
};

#endif  // LESSON_SOLUTION_HPP
